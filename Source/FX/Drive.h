#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

class Drive
{
public:
    enum class Type { Soft = 0, Hard, Fold };

    struct Params
    {
        Type  type    = Type::Soft;
        float driveDb = 0.f;   // 0–24 dB pre-gain
        float mix     = 0.f;   // 0 = dry, 1 = wet
    };

    void prepare(double sampleRate, int maxBlockSize);
    void setParams(const Params& p);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    Params params;

    static float processSample(float x, Type type, float gain) noexcept;
};
