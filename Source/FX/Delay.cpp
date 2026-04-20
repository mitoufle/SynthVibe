#include "Delay.h"
#include <cmath>

void Delay::prepare(double sr, int /*maxBlockSize*/)
{
    sampleRate = sr;
    bufSize    = static_cast<int>(sr * MaxDelayMs / 1000.0) + 4;
    bufL.assign(bufSize, 0.f);
    bufR.assign(bufSize, 0.f);
    writePos = 0;
    updateDelaySamples();
    smoothMix.reset(sr, 0.005);
    smoothMix.setCurrentAndTargetValue(0.f);
}

void Delay::setParams(const Params& p)
{
    params = p;
    updateDelaySamples();
    smoothMix.setTargetValue(p.mix);
}

void Delay::reset()
{
    std::fill(bufL.begin(), bufL.end(), 0.f);
    std::fill(bufR.begin(), bufR.end(), 0.f);
    writePos = 0;
}

void Delay::updateDelaySamples()
{
    delaySamples = juce::jlimit(1.f, static_cast<float>(bufSize - 2),
                                 params.timeMs * static_cast<float>(sampleRate) / 1000.f);
}

float Delay::readInterpolated(const std::vector<float>& buf, float delSamples) const noexcept
{
    float readPosF = static_cast<float>(writePos) - delSamples;
    while (readPosF < 0.f) readPosF += static_cast<float>(bufSize);

    const int   i0   = static_cast<int>(readPosF) % bufSize;
    const int   i1   = (i0 + 1) % bufSize;
    const float frac = readPosF - std::floor(readPosF);

    return buf[i0] * (1.f - frac) + buf[i1] * frac;
}

void Delay::process(juce::AudioBuffer<float>& buffer)
{
    if (!smoothMix.isSmoothing() && smoothMix.getTargetValue() < 0.0001f)
    {
        smoothMix.skip(buffer.getNumSamples());
        return;
    }

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    float* chL = buffer.getWritePointer(0);
    float* chR = numChannels > 1 ? buffer.getWritePointer(1) : chL;

    const float fb = params.feedback;

    for (int i = 0; i < numSamples; ++i)
    {
        const float wet  = smoothMix.getNextValue();
        const float dry  = 1.f - wet;
        const float dryL = chL[i];
        const float dryR = chR[i];

        const float wetL = readInterpolated(bufL, delaySamples);
        const float wetR = readInterpolated(bufR, delaySamples);

        bufL[writePos] = std::tanh(dryL + wetL * fb);
        bufR[writePos] = std::tanh(dryR + wetR * fb);

        chL[i] = dry * dryL + wet * wetL;
        if (chR != chL)
            chR[i] = dry * dryR + wet * wetR;

        writePos = (writePos + 1) % bufSize;
    }
}
