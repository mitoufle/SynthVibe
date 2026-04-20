#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

// Stereo delay with linear interpolation, feedback, and wet/dry mix.
// Feedback uses tanh soft-clipping to prevent runaway at high feedback values.
class Delay
{
public:
    struct Params
    {
        float timeMs   = 250.f;   // 1 – 2000 ms
        float feedback = 0.35f;   // 0 – 0.95
        float mix      = 0.0f;    // 0 = dry, 1 = fully wet
    };

    void prepare(double sampleRate, int maxBlockSize);
    void setParams(const Params& p);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    static constexpr int MaxDelayMs = 2000;

    std::vector<float> bufL, bufR;
    int    writePos     = 0;
    int    bufSize      = 0;
    float  delaySamples = 0.f;
    double sampleRate   = 44100.0;
    Params params;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMix;

    void  updateDelaySamples();
    float readInterpolated(const std::vector<float>& buf, float delSamples) const noexcept;
};
