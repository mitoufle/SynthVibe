#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <utility>
#include <random>

class ArpEngine
{
public:
    enum class Mode {
        Up       = 0,
        Down     = 1,
        UpDown   = 2,
        Dnup     = 3,    // NEW — descending then ascending
        Random   = 4,    // moved from index 3 (preset break per Phase 5 design)
        AsPlayed = 5,    // NEW — preserve insertion order
        Chord    = 6     // NEW — emit all held notes simultaneously per step
    };

    struct Params
    {
        bool  enabled     = false;
        Mode  mode        = Mode::Up;
        int   rateIndex   = 0;    // 0=1/16, 1=1/8, 2=1/4, 3=1/2
        int   octaveRange = 1;    // 1..4
    };

    void setParams(const Params& p);
    void noteOn (int midiNote, float velocity);
    void noteOff(int midiNote);
    void reset();
    void prepare();

    // Rewrites midiBuffer in-place. Call before SynthEngine::handleMidiMessage.
    void process(juce::MidiBuffer& midi, int numSamples, double bpm, double sr);

private:
    Params params;

    struct HeldNote { int note; float velocity; };
    std::vector<HeldNote> heldNotes;
    std::vector<HeldNote> sequence;
    std::vector<HeldNote> sortedBuf;  // buffer de travail pour buildSequence(), évite l'alloc per-call
    juce::MidiBuffer scratchMidi;  // pré-alloué pour éviter l'allocation dans process()

    int   stepIndex      = 0;
    int   sampleCounter  = 0;
    int   pingDir        = 1;
    int   lastNote       = -1;
    bool  noteIsOn       = false;
    bool  pendingNoteOff = false;   // set when arp disabled mid-note
    std::vector<HeldNote> pendingNoteOns; // saved held notes to re-emit on disable

    std::mt19937 rng { std::random_device{}() };

    void buildSequence();
    int  samplesPerStep(double bpm, double sr) const noexcept;
};
