#include "Drive.h"
#include <cmath>

void Drive::prepare(double /*sampleRate*/, int /*maxBlockSize*/) {}
void Drive::setParams(const Params& p)
{
    params         = p;
    cachedGain     = std::pow(10.f, p.driveDb / 20.f);
    cachedPostGain = 1.f / std::sqrt(cachedGain);
}
void Drive::reset() {}

void Drive::process(juce::AudioBuffer<float>& buffer)
{
    if (params.mix < 0.001f)
        return;

    const int   numChannels = buffer.getNumChannels();
    const int   numSamples  = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            const float dry = data[i];
            const float wet = processSample(dry, params.type, cachedGain) * cachedPostGain;
            data[i] = dry * (1.f - params.mix) + wet * params.mix;
        }
    }
}

float Drive::processSample(float x, Type type, float gain) noexcept
{
    switch (type)
    {
        case Type::Soft:
            return std::tanh(gain * x);

        case Type::Hard:
            return juce::jlimit(-1.f, 1.f, gain * x);

        case Type::Fold:
        {
            float y = juce::jlimit(-1024.f, 1024.f, gain * x);
            while (y > 1.f || y < -1.f)
            {
                if (y > 1.f) y = 2.f - y;
                else         y = -2.f - y;
            }
            return y;
        }
    }
    return x;
}
