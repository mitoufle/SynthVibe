#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

class Reverb
{
public:
    struct Params
    {
        float room = 0.5f;  // 0–1
        float damp = 0.5f;  // 0–1
        float mix  = 0.f;   // 0 = dry, 1 = wet
    };

    void prepare(double sampleRate, int maxBlockSize);
    void setParams(const Params& p);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    juce::Reverb reverb;
    Params params;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMix;
    juce::AudioBuffer<float> dryBuf;  // pre-allocated dry copy, avoids audio-thread alloc
};
