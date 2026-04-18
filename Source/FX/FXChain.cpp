#include "FXChain.h"

void FXChain::prepare(double sr, int maxBlockSize)
{
    delay.prepare(sr, maxBlockSize);
    chorus.prepare(sr, maxBlockSize);
}

void FXChain::setParams(const Delay::Params& dp, const Chorus::Params& cp)
{
    delay.setParams(dp);
    chorus.setParams(cp);
}

void FXChain::process(juce::AudioBuffer<float>& buffer)
{
    delay.process(buffer);
    chorus.process(buffer);
}

void FXChain::reset()
{
    delay.reset();
    chorus.reset();
}
