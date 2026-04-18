#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

class Chorus
{
public:
    struct Params
    {
        float rate  = 0.5f;    // Hz,  0.1 – 5.0
        float depth = 0.003f;  // sec, 0.001 – 0.015
        float mix   = 0.f;     // 0 = dry, 1 = wet
    };

    void prepare(double sampleRate, int maxBlockSize);
    void setParams(const Params& p);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    static constexpr int MaxDelayMs = 25;

    std::vector<float> bufL, bufR;
    int    writePos   = 0;
    int    bufSize    = 0;
    double sampleRate = 44100.0;
    double lfoPhase   = 0.0;
    Params params;

    float readInterpolated(const std::vector<float>& buf, float delSamples) const noexcept;
};
