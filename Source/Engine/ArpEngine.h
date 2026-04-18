#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <utility>

class ArpEngine
{
public:
    enum class Mode { Up = 0, Down, UpDown, Random };

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

    // Rewrites midiBuffer in-place. Call before SynthEngine::handleMidiMessage.
    void process(juce::MidiBuffer& midi, int numSamples, double bpm, double sr);

private:
    Params params;

    struct HeldNote { int note; float velocity; };
    std::vector<HeldNote> heldNotes;
    std::vector<HeldNote> sequence;

    int   stepIndex      = 0;
    int   sampleCounter  = 0;
    int   pingDir        = 1;
    int   lastNote       = -1;
    bool  noteIsOn       = false;
    bool  pendingNoteOff = false;   // set when arp disabled mid-note

    void buildSequence();
    int  samplesPerStep(double bpm, double sr) const noexcept;
};
