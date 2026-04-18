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
    writePos = 0;
    lfoPhase = 0.0;
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
        const float lfo = 0.5f * (1.f + std::sin(juce::MathConstants<float>::twoPi * static_cast<float>(lfoPhase)));
        lfoPhase += lfoIncrement;
        if (lfoPhase >= 1.0) lfoPhase -= 1.0;

        const float delaySamples = static_cast<float>((0.007 + params.depth * lfo) * sampleRate);

        bufL[writePos] = chL[i];
        bufR[writePos] = chR[i];

        const float wetL = readInterpolated(bufL, delaySamples);
        const float wetR = readInterpolated(bufR, delaySamples);

        chL[i] = chL[i] * (1.f - params.mix) + wetL * params.mix;
        chR[i] = chR[i] * (1.f - params.mix) + wetR * params.mix;

        writePos = (writePos + 1) % bufSize;
    }
}

float Chorus::readInterpolated(const std::vector<float>& buf, float delSamples) const noexcept
{
    const float readPosF = static_cast<float>(writePos) - delSamples;
    int   readPosI       = static_cast<int>(readPosF);
    const float frac     = readPosF - static_cast<float>(readPosI);
    while (readPosI < 0) readPosI += bufSize;
    readPosI %= bufSize;
    const int next = (readPosI + 1) % bufSize;
    return buf[readPosI] * (1.f - frac) + buf[next] * frac;
}
