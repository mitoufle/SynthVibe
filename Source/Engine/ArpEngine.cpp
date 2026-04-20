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

    // Transition: disabled → enabled (clear any stale saved notes from a prior disable)
    if (p.enabled && !params.enabled)
        pendingNoteOns.clear();

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
    stepIndex      = 0;
    sampleCounter  = 0;
    pingDir        = 1;
    lastNote       = -1;
    noteIsOn       = false;
    pendingNoteOff = false;
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

    sortedBuf.assign(heldNotes.begin(), heldNotes.end());
    std::sort(sortedBuf.begin(), sortedBuf.end(),
              [](const HeldNote& a, const HeldNote& b) { return a.note < b.note; });

    for (int oct = 0; oct < params.octaveRange; ++oct)
        for (auto& h : sortedBuf)
            sequence.push_back({ h.note + oct * 12, h.velocity });

    if (params.mode == Mode::Down)
        std::reverse(sequence.begin(), sequence.end());

    if (params.mode == Mode::Random)
        std::shuffle(sequence.begin(), sequence.end(), rng);

    if (stepIndex >= static_cast<int>(sequence.size()))
        stepIndex = 0;
}

int ArpEngine::samplesPerStep(double bpm, double sr) const noexcept
{
    static constexpr double rateFractions[] = { 0.25, 0.5, 1.0, 2.0 };
    const double safeBpm = std::max(bpm, 20.0);
    const double beats   = rateFractions[juce::jlimit(0, 3, params.rateIndex)];
    return std::max(static_cast<int>((60.0 / safeBpm) * beats * sr), 1);
}

void ArpEngine::process(juce::MidiBuffer& midi, int numSamples, double bpm, double sr)
{
    if (pendingNoteOff || !pendingNoteOns.empty())
    {
        if (pendingNoteOff && lastNote >= 0)
            midi.addEvent(juce::MidiMessage::noteOff(1, lastNote), 0);

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
        if (sampleCounter == 0)
        {
            if (noteIsOn && lastNote >= 0)
            {
                scratchMidi.addEvent(juce::MidiMessage::noteOff(1, lastNote), i);
                noteIsOn = false;
            }

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
        ++sampleCounter;
        if (sampleCounter >= stepLen) sampleCounter = 0;
    }

    midi.swapWith(scratchMidi);
}
