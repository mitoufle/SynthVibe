#pragma once

// ADSR envelope. Used for both amplitude and filter modulation.
class Envelope
{
public:
    struct Params
    {
        float attack  = 0.01f;  // seconds
        float decay   = 0.1f;
        float sustain = 0.7f;   // 0–1 level
        float release = 0.2f;   // seconds
    };

    void setSampleRate(double sr) { sampleRate = sr; }
    void setParams(const Params& p) { params = p; }

    void noteOn();
    void noteOff();
    void reset();

    float getNextSample();
    bool  isActive() const { return stage != Stage::Idle; }

private:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    double sampleRate    = 44100.0;
    Params params;
    Stage  stage         = Stage::Idle;
    float  currentLevel  = 0.f;
    float  releaseLevel  = 0.f;
};
