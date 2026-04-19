#pragma once
#include "Oscillator.h"
#include <algorithm>

// Wraps up to MaxUnison Oscillator instances with symmetric detune distribution.
// Normalization is power-preserving (1/sqrt(N)) and assumes spread > 0.
class UnisonOscillator
{
public:
    static constexpr int MaxUnison = 7;

    void setSampleRate(double sr);
    void setFrequency(double hz);
    void setWaveform(Waveform wf);
    void setDetuneCents(float baseCents);
    void setUnison(int voices, float spreadCents);
    void reset();
    float getNextSample();

private:
    Oscillator oscs[MaxUnison];
    int   unisonVoices = 1;
    float spreadCents  = 0.f;
    float baseCents    = 0.f;
    float invNormGain  = 1.f;

    void recomputeDetune();
};
