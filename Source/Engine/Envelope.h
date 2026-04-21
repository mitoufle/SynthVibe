#pragma once
#include <algorithm>
#include <cmath>

class Envelope
{
public:
    struct Params
    {
        float attack  = 0.01f;
        float decay   = 0.1f;
        float sustain = 0.7f;
        float release = 0.2f;
    };

    void setSampleRate(double sr)
    {
        sampleRate = sr;
        recomputeCoeffs();
    }

    void setParams(const Params& p)
    {
        params = p;
        recomputeCoeffs();
    }

    void noteOn();
    void noteOff();
    void reset();

    float getNextSample();
    bool  isActive() const { return stage != Stage::Idle; }

private:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    void recomputeCoeffs()
    {
        const float sr = static_cast<float>(sampleRate);
        decayCoeff   = std::exp(-1.f / (std::max(0.0001f, params.decay)   * sr));
        releaseCoeff = std::exp(-1.f / (std::max(0.0001f, params.release) * sr));
    }

    double sampleRate    = 44100.0;
    Params params;
    Stage  stage         = Stage::Idle;
    float  currentLevel  = 0.f;
    float  releaseLevel  = 0.f;
    float  decayCoeff    = 0.f;
    float  releaseCoeff  = 0.f;
};
