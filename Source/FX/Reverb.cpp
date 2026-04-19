#include "Reverb.h"

void Reverb::prepare(double sampleRate, int /*maxBlockSize*/)
{
    reverb.setSampleRate(sampleRate);
    reverb.reset();
}

void Reverb::setParams(const Params& p)
{
    params = p;
    juce::Reverb::Parameters rp;
    rp.roomSize   = p.room;
    rp.damping    = p.damp;
    rp.wetLevel   = p.mix;
    rp.dryLevel   = 1.f - p.mix;
    rp.width      = 1.f;
    rp.freezeMode = 0.f;
    reverb.setParameters(rp);
}

void Reverb::process(juce::AudioBuffer<float>& buffer)
{
    if (params.mix < 0.001f)
        return;

    const int numSamples = buffer.getNumSamples();
    if (buffer.getNumChannels() >= 2)
        reverb.processStereo(buffer.getWritePointer(0),
                             buffer.getWritePointer(1),
                             numSamples);
    else
        reverb.processMono(buffer.getWritePointer(0), numSamples);
}

void Reverb::reset()
{
    reverb.reset();
}
