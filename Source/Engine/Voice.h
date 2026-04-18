#pragma once
#include "Oscillator.h"
#include "Envelope.h"
#include "Filter.h"
#include <juce_dsp/juce_dsp.h>

enum class LfoDest { Pitch = 0, Filter, Amp, Detune };

struct OscParams
{
    Waveform waveform = Waveform::Saw;
    int      octave   = 0;
    int      semitone = 0;
    float    detune   = 0.f;
    float    level    = 1.f;
};

struct LfoParams
{
    Waveform shape = Waveform::Sine;
    float    rate  = 1.f;
    float    depth = 0.f;
    LfoDest  dest  = LfoDest::Pitch;
};

struct VoiceParams
{
    OscParams osc1;
    OscParams osc2;   // osc2.level read from APVTS default 0.f — silent until turned up

    LfoParams lfo1;
    LfoParams lfo2;

    FilterType filterType      = FilterType::LowPass;
    float      filterCutoff    = 8000.f;
    float      filterResonance = 0.1f;
    float      filterEnvAmt    = 0.f;

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
    Oscillator osc1;
    Oscillator osc2;
    Oscillator lfo1Osc;
    Oscillator lfo2Osc;
    Envelope   ampEnv;
    Envelope   fltEnv;
    Filter     filter;

    VoiceParams params;
    int    currentNote = -1;
    float  velocity    = 1.f;
    double sampleRate  = 44100.0;

    static double midiNoteToHz(int note, int octaveOffset, int semitoneOffset = 0) noexcept;
};
