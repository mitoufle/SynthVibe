#pragma once
#include "Delay.h"
#include "Chorus.h"
#include "Drive.h"
#include "Reverb.h"

class FXChain
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void setParams(const Delay::Params& dp,
                   const Chorus::Params& cp,
                   const Drive::Params& drp,
                   const Reverb::Params& rp);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    Delay  delay;
    Chorus chorus;
    Drive  drive;
    Reverb reverb;
};
