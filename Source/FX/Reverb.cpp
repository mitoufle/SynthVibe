#include "Reverb.h"

void Reverb::prepare(double sampleRate, int maxBlockSize)
{
    reverb.setSampleRate(sampleRate);
    reverb.reset();
    smoothMix.reset(sampleRate, 0.005);
    smoothMix.setCurrentAndTargetValue(0.f);
    dryBuf.setSize(2, maxBlockSize, false, true, false);
}

void Reverb::setParams(const Params& p)
{
    params = p;
    smoothMix.setTargetValue(p.mix);
    juce::Reverb::Parameters rp;
    rp.roomSize   = p.room;
    rp.damping    = p.damp;
    rp.wetLevel   = 1.f;   // dry/wet blended manually in process() for smooth mix
    rp.dryLevel   = 0.f;
    rp.width      = 1.f;
    rp.freezeMode = 0.f;
    reverb.setParameters(rp);
}

void Reverb::process(juce::AudioBuffer<float>& buffer)
{
    if (!smoothMix.isSmoothing() && smoothMix.getTargetValue() < 0.001f)
    {
        smoothMix.skip(buffer.getNumSamples());
        return;
    }

    const int numSamples = buffer.getNumSamples();

    // Copy dry signal before reverb overwrites the buffer
    dryBuf.copyFrom(0, 0, buffer, 0, 0, numSamples);
    if (buffer.getNumChannels() >= 2)
        dryBuf.copyFrom(1, 0, buffer, 1, 0, numSamples);

    // Process at full wet
    if (buffer.getNumChannels() >= 2)
        reverb.processStereo(buffer.getWritePointer(0),
                             buffer.getWritePointer(1),
                             numSamples);
    else
        reverb.processMono(buffer.getWritePointer(0), numSamples);

    // Blend dry + wet per-sample using smoothed mix
    float*       outL = buffer.getWritePointer(0);
    const float* dryL = dryBuf.getReadPointer(0);
    float*       outR = buffer.getNumChannels() >= 2 ? buffer.getWritePointer(1) : nullptr;
    const float* dryR = buffer.getNumChannels() >= 2 ? dryBuf.getReadPointer(1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        const float mix = smoothMix.getNextValue();
        outL[i] = dryL[i] * (1.f - mix) + outL[i] * mix;
        if (outR) outR[i] = dryR[i] * (1.f - mix) + outR[i] * mix;
    }
}

void Reverb::reset()
{
    reverb.reset();
}
