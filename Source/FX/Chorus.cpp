#include "Chorus.h"
#include <cmath>

void Chorus::prepare(double sr, int /*maxBlockSize*/)
{
    sampleRate = sr;
    bufSize    = static_cast<int>(sr * MaxDelayMs / 1000.0) + 2;
    bufL.assign(bufSize, 0.f);
    bufR.assign(bufSize, 0.f);
    writePos  = 0;
    lfoPhase  = 0.0;
}

void Chorus::setParams(const Params& p)
{
    params = p;
}

void Chorus::reset()
{
    std::fill(bufL.begin(), bufL.end(), 0.f);
    std::fill(bufR.begin(), bufR.end(), 0.f);
    writePos  = 0;
    lfoPhase  = 0.0;
    lfoPhaseR = 0.25;
}

void Chorus::process(juce::AudioBuffer<float>& buffer)
{
    if (params.mix < 0.001f)
        return;

    const int   numSamples   = buffer.getNumSamples();
    float*      chL          = buffer.getWritePointer(0);
    float*      chR          = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : chL;
    const float lfoIncrement = static_cast<float>(params.rate / sampleRate);

    for (int i = 0; i < numSamples; ++i)
    {
        const float lfoL = 0.5f * (1.f + std::sin(juce::MathConstants<float>::twoPi * static_cast<float>(lfoPhase)));
        lfoPhase += lfoIncrement;
        if (lfoPhase >= 1.0) lfoPhase -= 1.0;

        const float lfoR = 0.5f * (1.f + std::sin(juce::MathConstants<float>::twoPi * static_cast<float>(lfoPhaseR)));
        lfoPhaseR += lfoIncrement;
        if (lfoPhaseR >= 1.0) lfoPhaseR -= 1.0;

        const float delaySamplesL = static_cast<float>((0.007 + params.depth * lfoL) * sampleRate);
        const float delaySamplesR = static_cast<float>((0.007 + params.depth * lfoR) * sampleRate);

        bufL[writePos] = chL[i];
        bufR[writePos] = chR[i];

        const float wetL = readInterpolated(bufL, delaySamplesL);
        const float wetR = readInterpolated(bufR, delaySamplesR);

        chL[i] = chL[i] * (1.f - params.mix) + wetL * params.mix;
        chR[i] = chR[i] * (1.f - params.mix) + wetR * params.mix;

        writePos = (writePos + 1) % bufSize;
    }
}

float Chorus::readInterpolated(const std::vector<float>& buf, float delSamples) const noexcept
{
    float readPosF = static_cast<float>(writePos) - delSamples;
    while (readPosF < 0.f) readPosF += static_cast<float>(bufSize);
    const int   i0   = static_cast<int>(readPosF) % bufSize;
    const int   i1   = (i0 + 1) % bufSize;
    const float frac = readPosF - std::floor(readPosF);
    return buf[i0] * (1.f - frac) + buf[i1] * frac;
}
