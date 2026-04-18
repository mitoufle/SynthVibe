#pragma once
#include "Oscillator.h"
#include "Envelope.h"
#include "Filter.h"
#include <juce_dsp/juce_dsp.h>

// All parameters needed to configure one voice.
// This struct is the bridge between the APVTS and the audio engine —
// the AI layer will also build VoiceParams from a prompt interpretation.
struct VoiceParams
{
    // Oscillator
    Waveform waveform     = Waveform::Saw;
    float    detuneCents  = 0.f;
    int      octave       = 0;

    // Filter
    FilterType filterType      = FilterType::LowPass;
    float      filterCutoff    = 8000.f;
    float      filterResonance = 0.1f;
    float      filterEnvAmt    = 0.f;   // -1..1, scales filter envelope depth

    // Envelopes
    Envelope::Params ampEnv;
    Envelope::Params fltEnv;
};

class Voice
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void setParams(const VoiceParams& p);

    void noteOn (int midiNote, float velocity);
    void noteOff();

    float getNextSample();
    bool  isActive()    const { return ampEnv.isActive(); }
    int   getMidiNote() const { return currentNote; }

private:
    Oscillator osc;
    Envelope   ampEnv;
    Envelope   fltEnv;
    Filter     filter;

    VoiceParams params;
    int    currentNote = -1;
    float  velocity    = 1.f;
    double sampleRate  = 44100.0;

    static double midiNoteToHz(int note, int octaveOffset) noexcept;
};
