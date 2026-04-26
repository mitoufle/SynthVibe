#include "ArpEngine.h"
#include <algorithm>
#include <iterator>
#include <random>

void ArpEngine::setParams(const Params& p)
{
    if (!p.enabled && params.enabled && noteIsOn && lastNote >= 0)
        pendingNoteOff = true;

    // Transition: enabled → disabled (only fires once; params still holds old state here)
    if (!p.enabled && params.enabled)
    {
        if (!heldNotes.empty())
            pendingNoteOns.assign(heldNotes.begin(), heldNotes.end());
    }

    // Transition: disabled → enabled (clear any stale state from a prior disable)
    if (p.enabled && !params.enabled)
    {
        pendingNoteOns.clear();
        pendingNoteOff = false;
        needsAllNotesOff = true;   // clear any synth voices held while arp was off
    }

    // Transition: Chord → other while playing — arm a flush so the existing
    // pendingNoteOff path emits noteOff for every entry in currentChordNotes,
    // not just lastNote. Without this, voices 2..N of the previous chord step
    // stay stuck on after the mode switch.
    if (params.mode == Mode::Chord && p.mode != Mode::Chord && noteIsOn)
        pendingNoteOff = true;

    const bool latchWasOn = params.latch;
    params = p;

    if (latchWasOn && !p.latch)
    {
        // Latch flipped off — release any held notes immediately.
        heldNotes.clear();
        sequence.clear();
        currentChordNotes.clear();
        freshLatchGroup = true;
    }

    if (!p.enabled)
    {
        heldNotes.clear();
        sequence.clear();
        currentChordNotes.clear();
        stepIndex         = 0;
        sampleCounter     = 0;
        pingDir           = 1;
        swingShiftSamples = 0;
        // Ne pas toucher noteIsOn/lastNote — process() les nettoie après le noteOff
    }
}

void ArpEngine::noteOn(int midiNote, float velocity)
{
    if (params.latch && freshLatchGroup)
    {
        heldNotes.clear();           // replace-on-new-chord
        freshLatchGroup = false;
    }
    for (auto& h : heldNotes)
        if (h.note == midiNote) return;
    heldNotes.push_back({ midiNote, velocity });
    buildSequence();
}

void ArpEngine::noteOff(int midiNote)
{
    if (params.latch)
    {
        // Don't remove from heldNotes. Just arm the next noteOn to start a fresh chord.
        freshLatchGroup = true;
        return;
    }
    heldNotes.erase(std::remove_if(heldNotes.begin(), heldNotes.end(),
        [midiNote](const HeldNote& h) { return h.note == midiNote; }),
        heldNotes.end());
    buildSequence();
    // Note: we do NOT clear currentChordNotes here. The sequence-empty cleanup in process()
    // needs it to emit noteOff for every chord voice (not just lastNote). It will clear it
    // after emitting.
    if (heldNotes.empty()) { stepIndex = 0; sampleCounter = 0; swingShiftSamples = 0; }
}

void ArpEngine::reset()
{
    heldNotes.clear();
    sequence.clear();
    sortedBuf.clear();
    currentChordNotes.clear();
    stepIndex             = 0;
    sampleCounter         = 0;
    samplesUntilNoteOff   = 0;
    swingShiftSamples     = 0;
    freshLatchGroup       = true;
    pingDir               = 1;
    lastNote              = -1;
    noteIsOn              = false;
    pendingNoteOff        = false;
    needsAllNotesOff      = false;
    pendingNoteOns.clear();
}

void ArpEngine::prepare()
{
    heldNotes.reserve(32);
    sequence.reserve(128);
    sortedBuf.reserve(32);
    pendingNoteOns.reserve(32);
    currentChordNotes.reserve(32);
    scratchMidi.ensureSize(512);
    humanizeRng.seed(0xC0FFEEu);
}

void ArpEngine::buildSequence()
{
    sequence.clear();
    if (heldNotes.empty()) return;

    // AsPlayed preserves insertion order; everything else sorts ascending.
    if (params.mode == Mode::AsPlayed)
        sortedBuf.assign(heldNotes.begin(), heldNotes.end());
    else
    {
        sortedBuf.assign(heldNotes.begin(), heldNotes.end());
        std::sort(sortedBuf.begin(), sortedBuf.end(),
                  [](const HeldNote& a, const HeldNote& b) { return a.note < b.note; });
    }

    for (int oct = 0; oct < params.octaveRange; ++oct)
        for (auto& h : sortedBuf)
            sequence.push_back({ h.note + oct * 12, h.velocity });

    // Per-mode reordering after the octave expansion.
    switch (params.mode)
    {
        case Mode::Down:
            std::reverse(sequence.begin(), sequence.end());
            break;
        case Mode::Dnup:
            // Build a descending-then-ascending walk. With sequence = [60,64,67]
            // the constructed sequence is [67,64,60,64], which loops as
            // 67->64->60->64->67->64->60->64->... by wrapping stepIndex.
            // We construct it by reversing first, then appending the original
            // interior (minus the endpoints) to avoid double-hitting the bottom
            // note when looping.
            {
                std::vector<HeldNote> dnup;
                dnup.reserve(sequence.size() * 2);
                for (auto it = sequence.rbegin(); it != sequence.rend(); ++it)
                    dnup.push_back(*it);
                if (sequence.size() > 2)
                    for (auto it = sequence.begin() + 1; it != sequence.end() - 1; ++it)
                        dnup.push_back(*it);
                sequence.swap(dnup);
            }
            break;
        case Mode::Random:
            std::shuffle(sequence.begin(), sequence.end(), rng);
            break;
        case Mode::Up:
        case Mode::UpDown:
        case Mode::AsPlayed:
        case Mode::Chord:
        default:
            break;  // Up/AsPlayed/Chord: no reorder; UpDown reorders at runtime in process()
    }

    if (stepIndex >= static_cast<int>(sequence.size()))
        stepIndex = 0;
}

int ArpEngine::samplesPerStep(double bpm, double sr) const noexcept
{
    // Aligned with ParameterLayout's StringArray { "1/4", "1/8", "1/16", "1/16T", "1/32" }.
    // Values are the fraction-of-a-quarter-note duration of one step.
    static constexpr double rateFractions[] = {
        1.0,                  // 1/4   - quarter note
        0.5,                  // 1/8   - eighth
        0.25,                 // 1/16  - sixteenth
        0.25 * (2.0 / 3.0),   // 1/16T - sixteenth-note triplet
        0.125                 // 1/32  - thirty-second
    };
    const double safeBpm = std::max(bpm, 20.0);
    constexpr int kRateMax = static_cast<int>(std::size(rateFractions)) - 1;
    const double beats   = rateFractions[juce::jlimit(0, kRateMax, params.rateIndex)];
    return std::max(static_cast<int>((60.0 / safeBpm) * beats * sr), 1);
}

void ArpEngine::process(juce::MidiBuffer& midi, int numSamples, double bpm, double sr)
{
    if (pendingNoteOff || !pendingNoteOns.empty())
    {
        if (pendingNoteOff && lastNote >= 0)
        {
            if (!currentChordNotes.empty())
            {
                for (int cn : currentChordNotes)
                    midi.addEvent(juce::MidiMessage::noteOff(1, cn), 0);
                currentChordNotes.clear();
            }
            else
            {
                midi.addEvent(juce::MidiMessage::noteOff(1, lastNote), 0);
            }
        }

        int samplePos = 1; // after the noteOff
        for (const auto& h : pendingNoteOns)
            midi.addEvent(juce::MidiMessage::noteOn(1, h.note, h.velocity), samplePos++);

        pendingNoteOns.clear();
        pendingNoteOff = false;
        noteIsOn       = false;
        lastNote       = -1;
        return;
    }

    if (!params.enabled)
        return;

    scratchMidi.clear();

    // First enabled block after disable→enable: silence any synth voices that were
    // playing while the arp was off. Otherwise the user's noteOff (consumed below)
    // never reaches the synth and the voice stays stuck.
    if (needsAllNotesOff)
    {
        scratchMidi.addEvent(juce::MidiMessage::allNotesOff(1), 0);
        needsAllNotesOff = false;
    }
    const int stepLen = samplesPerStep(bpm, sr);

    for (auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOff())
            noteOff(msg.getNoteNumber());
        else if (msg.isNoteOn())
            noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
        else
            scratchMidi.addEvent(msg, meta.samplePosition);
    }

    if (sequence.empty())
    {
        if (noteIsOn && lastNote >= 0)
        {
            if (!currentChordNotes.empty())
            {
                for (int cn : currentChordNotes)
                    scratchMidi.addEvent(juce::MidiMessage::noteOff(1, cn), 0);
                currentChordNotes.clear();
            }
            else
            {
                scratchMidi.addEvent(juce::MidiMessage::noteOff(1, lastNote), 0);
            }
            noteIsOn = false;
            lastNote = -1;
        }
        midi.swapWith(scratchMidi);
        return;
    }

    for (int i = 0; i < numSamples; ++i)
    {
        // Gate-driven noteOff: when timer hits 0, kill the current note.
        if (noteIsOn && samplesUntilNoteOff > 0)
        {
            --samplesUntilNoteOff;
            if (samplesUntilNoteOff == 0 && lastNote >= 0)
            {
                if (params.mode == Mode::Chord && !currentChordNotes.empty())
                {
                    for (int cn : currentChordNotes)
                        scratchMidi.addEvent(juce::MidiMessage::noteOff(1, cn), i);
                    currentChordNotes.clear();
                }
                else
                {
                    scratchMidi.addEvent(juce::MidiMessage::noteOff(1, lastNote), i);
                }
                noteIsOn = false;
            }
        }

        if (sampleCounter == swingShiftSamples)
        {
            // Defensive noteOff if the previous note is still on (e.g. gate=1.0
            // means samplesUntilNoteOff hits 0 exactly at this iteration).
            if (noteIsOn && lastNote >= 0)
            {
                if (params.mode == Mode::Chord && !currentChordNotes.empty())
                {
                    for (int cn : currentChordNotes)
                        scratchMidi.addEvent(juce::MidiMessage::noteOff(1, cn), i);
                    currentChordNotes.clear();
                }
                else
                {
                    scratchMidi.addEvent(juce::MidiMessage::noteOff(1, lastNote), i);
                }
                noteIsOn = false;
            }

            if (params.mode == Mode::Chord)
            {
                // Chord: emit ALL notes in `sequence` simultaneously (sequence is the
                // octave-expanded held set built by buildSequence()). Populate
                // currentChordNotes so gate-driven noteOffs kill the whole chord.
                // Note: stale currentChordNotes from a Chord→other mode switch are
                // handled by the mode==Chord guards above; v1 limitation.
                currentChordNotes.clear();
                for (auto& step : sequence)
                {
                    const float vel = applyHumanizeVelocity(step.velocity);
                    scratchMidi.addEvent(juce::MidiMessage::noteOn(1, step.note, vel), i);
                    currentChordNotes.push_back(step.note);
                }
                lastNote = sequence.empty() ? -1 : sequence.front().note;
                noteIsOn = lastNote >= 0;
                stepIndex = 0;   // chord has 1 logical step; keep stepIndex bounded
            }
            else
            {
                const auto& step = sequence[stepIndex];
                const float vel = applyHumanizeVelocity(step.velocity);
                scratchMidi.addEvent(juce::MidiMessage::noteOn(1, step.note, vel), i);
                lastNote = step.note;
                noteIsOn = true;

                if (params.mode == Mode::UpDown)
                {
                    stepIndex += pingDir;
                    const int last = static_cast<int>(sequence.size()) - 1;
                    if (stepIndex > last) { stepIndex = last; pingDir = -1; }
                    if (stepIndex < 0)    { stepIndex = 0;    pingDir =  1; }
                }
                else
                {
                    stepIndex = (stepIndex + 1) % static_cast<int>(sequence.size());
                }
            }

            // Schedule the gate-driven noteOff: between 1 and stepLen-1 samples
            // from now. gate=1.0 → fires exactly at next step boundary, where
            // the defensive noteOff above will have already cleaned up if needed.
            samplesUntilNoteOff = std::max(1, static_cast<int>(params.gate * stepLen));
        }
        ++sampleCounter;
        if (sampleCounter >= stepLen)
        {
            sampleCounter = 0;
            // Compute swing offset for the NEW step (about to trigger next iteration).
            // Note: swing is keyed off stepIndex (sequence-array index), so odd/even
            // refers to position within the sequence, not absolute song-step count.
            const bool oddStep = (stepIndex & 1) != 0;
            swingShiftSamples = oddStep ? static_cast<int>(params.swing * stepLen * 0.5f) : 0;

            if (params.humanize > 0.f)
            {
                std::uniform_real_distribution<float> jit(-1.f, 1.f);
                swingShiftSamples += static_cast<int>(
                    jit(humanizeRng) * params.humanize * (stepLen / 8.0f));
            }
            swingShiftSamples = juce::jlimit(0, stepLen - 1, swingShiftSamples);
        }
    }

    midi.swapWith(scratchMidi);
}

float ArpEngine::applyHumanizeVelocity(float inputVel) noexcept
{
    if (params.humanize <= 0.f) return inputVel;
    std::uniform_real_distribution<float> reduce(0.f, 0.5f);
    return inputVel * (1.f - params.humanize * reduce(humanizeRng));
}
