#pragma once
#include <algorithm>
#include <cmath>

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
    void setStartingPhase(float deg)  { startingPhase = std::clamp(deg / 360.f, 0.f, 1.f); }
    void setPulseWidth(float duty)    { pulseWidth    = std::clamp(duty, 0.01f, 0.99f); }

    void reset()             { phase = 0.0; }
    void setPhase(double p)  { phase = p; }
    void resetPhaseToStart() { phase = static_cast<double>(startingPhase); }
    double getPhase() const  { return phase; }

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
