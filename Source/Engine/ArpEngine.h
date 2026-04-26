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
        int   rateIndex   = 2;    // index into samplesPerStep table {1/4, 1/8, 1/16, 1/16T, 1/32}
        int   octaveRange = 1;    // 1..4
        float gate        = 1.0f; // 0.05..1.0  fraction of step the note holds before noteOff
        float swing       = 0.0f; // 0..1  fraction of step that odd steps shift later
        float humanize    = 0.0f; // 0..1  per-step velocity jitter (-50% max) + timing jitter (+/-stepLen/8 max)
        bool  latch       = false; // true: noteOff doesn't remove; new noteOn after release replaces
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

    int   stepIndex            = 0;
    int   sampleCounter        = 0;
    int   samplesUntilNoteOff  = 0;  // gate timing: counts down from gate*stepLen at each step trigger
    int   swingShiftSamples   = 0;  // computed once per step trigger; 0 for even steps, swing*stepLen/2 for odd
    int   pingDir              = 1;
    int   lastNote       = -1;
    bool  noteIsOn       = false;
    bool  pendingNoteOff = false;   // set when arp disabled mid-note
    bool  freshLatchGroup = true;  // armed when all keys released; first noteOn after a release clears the held set
    std::vector<HeldNote> pendingNoteOns; // saved held notes to re-emit on disable

    std::mt19937 rng { std::random_device{}() };
    std::mt19937 humanizeRng { 0xC0FFEEu };  // fixed-seed RNG for reproducible humanization

    void buildSequence();
    int  samplesPerStep(double bpm, double sr) const noexcept;
    float applyHumanizeVelocity(float inputVel) noexcept;
};
