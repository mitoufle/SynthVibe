#include "FXChain.h"

void FXChain::prepare(double sr, int maxBlockSize)
{
    delay.prepare(sr, maxBlockSize);
    chorus.prepare(sr, maxBlockSize);
    reverb.prepare(sr, maxBlockSize);
}

void FXChain::setParams(const Delay::Params& dp,
                        const Chorus::Params& cp,
                        const Reverb::Params& rp)
{
    delay.setParams(dp);
    chorus.setParams(cp);
    reverb.setParams(rp);
}

void FXChain::process(juce::AudioBuffer<float>& buffer)
{
    delay.process(buffer);
    chorus.process(buffer);
    reverb.process(buffer);
}

void FXChain::reset()
{
    delay.reset();
    chorus.reset();
    reverb.reset();
}
