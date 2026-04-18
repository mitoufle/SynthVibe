#pragma once
#include "Delay.h"
#include "Chorus.h"

class FXChain
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void setParams(const Delay::Params& dp, const Chorus::Params& cp);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    Delay  delay;
    Chorus chorus;
};
