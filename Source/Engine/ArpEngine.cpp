#include "ArpEngine.h"
#include <algorithm>
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
    }

    params = p;

    if (!p.enabled)
    {
        heldNotes.clear();
        sequence.clear();
        stepIndex     = 0;
        sampleCounter = 0;
        pingDir       = 1;
        // Ne pas toucher noteIsOn/lastNote — process() les nettoie après le noteOff
    }
}

void ArpEngine::noteOn(int midiNote, float velocity)
{
    for (auto& h : heldNotes)
        if (h.note == midiNote) return;
    heldNotes.push_back({ midiNote, velocity });
    buildSequence();
}

void ArpEngine::noteOff(int midiNote)
{
    heldNotes.erase(std::remove_if(heldNotes.begin(), heldNotes.end(),
        [midiNote](const HeldNote& h) { return h.note == midiNote; }),
        heldNotes.end());
    buildSequence();
    if (heldNotes.empty()) { stepIndex = 0; sampleCounter = 0; }
}

void ArpEngine::reset()
{
    heldNotes.clear();
    sequence.clear();
    sortedBuf.clear();
    stepIndex             = 0;
    sampleCounter         = 0;
    samplesUntilNoteOff   = 0;
    pingDir               = 1;
    lastNote              = -1;
    noteIsOn              = false;
    pendingNoteOff        = false;
    pendingNoteOns.clear();
}

void ArpEngine::prepare()
{
    heldNotes.reserve(32);
    sequence.reserve(128);
    sortedBuf.reserve(32);
    pendingNoteOns.reserve(32);
    scratchMidi.ensureSize(512);
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
    const double beats   = rateFractions[juce::jlimit(0, 4, params.rateIndex)];
    return std::max(static_cast<int>((60.0 / safeBpm) * beats * sr), 1);
}

void ArpEngine::process(juce::MidiBuffer& midi, int numSamples, double bpm, double sr)
{
    if (pendingNoteOff || !pendingNoteOns.empty())
    {
        if (pendingNoteOff && lastNote >= 0)
            midi.addEvent(juce::MidiMessage::noteOff(1, lastNote), 0);

        int samplePos = 1;
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
            scratchMidi.addEvent(juce::MidiMessage::noteOff(1, lastNote), 0);
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
                scratchMidi.addEvent(juce::MidiMessage::noteOff(1, lastNote), i);
                noteIsOn = false;
            }
        }

        if (sampleCounter == 0)
        {
            // Defensive noteOff if the previous note is still on (e.g. gate=1.0
            // means samplesUntilNoteOff hits 0 exactly at this iteration).
            if (noteIsOn && lastNote >= 0)
            {
                scratchMidi.addEvent(juce::MidiMessage::noteOff(1, lastNote), i);
                noteIsOn = false;
            }

            if (params.mode == Mode::Chord)
            {
                // Chord: emit ALL notes in `sequence` simultaneously (sequence is the
                // octave-expanded held set built by buildSequence()). Voice-stealing
                // at the next step boundary handles the noteOffs for v1.
                for (auto& step : sequence)
                    scratchMidi.addEvent(juce::MidiMessage::noteOn(1, step.note, step.velocity), i);
                lastNote = sequence.empty() ? -1 : sequence.front().note;
                noteIsOn = lastNote >= 0;
                stepIndex = 0;   // chord has 1 logical step; keep stepIndex bounded
            }
            else
            {
                const auto& step = sequence[stepIndex];
                scratchMidi.addEvent(juce::MidiMessage::noteOn(1, step.note, step.velocity), i);
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
        if (sampleCounter >= stepLen) sampleCounter = 0;
    }

    midi.swapWith(scratchMidi);
}
