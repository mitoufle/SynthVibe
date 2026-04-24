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
    bool  isActive()     const { return stage != Stage::Idle; }
    // True only while the key is held (Attack/Decay/Sustain). False during
    // Release tails — used by the polyphonic gain compensation so dying
    // tails don't keep the per-voice gain depressed.
    bool  isSustaining() const { return stage != Stage::Idle && stage != Stage::Release; }

private:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    void recomputeCoeffs()
    {
        // -60 dB convention: after `decay`/`release` seconds the envelope has
        // dropped to 10⁻³ of its starting distance to the target. Treating the
        // parameter as a raw time constant (exp(-1/...)) left audible tails
        // of ~11×release because reaching the Idle threshold (1e-5) takes
        // many time constants. 6.908 = -ln(10⁻³).
        constexpr float k = 6.908f;
        const float sr = static_cast<float>(sampleRate);
        decayCoeff   = std::exp(-k / (std::max(0.0001f, params.decay)   * sr));
        releaseCoeff = std::exp(-k / (std::max(0.0001f, params.release) * sr));
    }

    double sampleRate    = 44100.0;
    Params params;
    Stage  stage         = Stage::Idle;
    float  currentLevel  = 0.f;
    float  decayCoeff    = 0.f;
    float  releaseCoeff  = 0.f;
};
