#pragma once
#include "Oscillator.h"
#include <algorithm>
#include <cmath>

class UnisonOscillator
{
public:
    static constexpr int MaxUnison = 7;

    void setSampleRate(double sr);
    void setFrequency(double hz);
    void setWaveform(Waveform wf);
    void setDetuneCents(float baseCents);
    void setUnison(int voices, float spreadCents);
    void setStereoSpread(float spread);   // 0 = centre, 1 = full width
    void reset();
    void getNextSample(float& outL, float& outR);

private:
    Oscillator oscs[MaxUnison];
    int   unisonVoices = 1;
    float spreadCents  = 0.f;
    float baseCents    = 0.f;
    float invNormGain  = 1.f;
    float stereoSpread = 0.5f;
    float lGains[MaxUnison] = {};
    float rGains[MaxUnison] = {};

    void recomputeDetune();
    void recomputePan();
};
