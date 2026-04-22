#pragma once
#include <cmath>
#include <juce_core/juce_core.h>

enum class Waveform { Sine = 0, Saw, Square, Triangle };

// Single-voice oscillator with PolyBLEP anti-aliasing for Saw and Square.
// To add waveforms: extend the Waveform enum and add a case in getNextSample().
class Oscillator
{
public:
    void setSampleRate(double sr)     { sampleRate = sr; }
    void setFrequency(double freqHz)  { frequency = freqHz; }
    void setWaveform(Waveform wf)     { waveform = wf; }
    void setDetuneCents(float cents)  { detuneCents = cents; }
    void setStartingPhase(float deg)  { startingPhase = juce::jlimit(0.f, 1.f, deg / 360.f); }
    void setPulseWidth(float duty)    { pulseWidth    = juce::jlimit(0.01f, 0.99f, duty); }

    void reset()             { phase = 0.0; }
    void setPhase(double p)  { phase = p; }
    void resetPhaseToStart() { phase = static_cast<double>(startingPhase); }

    float getNextSample();

private:
    double   sampleRate    = 44100.0;
    double   frequency     = 440.0;
    double   phase         = 0.0;
    float    detuneCents   = 0.f;
    Waveform waveform      = Waveform::Saw;
    float    startingPhase = 0.f;
    float    pulseWidth    = 0.5f;

    double getDetuneRatio() const noexcept;
    float  polyBlep(double t, double dt) const noexcept;
};
