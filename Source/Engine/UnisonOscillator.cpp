#include "UnisonOscillator.h"
#include <cmath>

void UnisonOscillator::setSampleRate(double sr)
{
    for (auto& o : oscs)
        o.setSampleRate(sr);
}

void UnisonOscillator::setFrequency(double hz)
{
    for (int i = 0; i < unisonVoices; ++i)
        oscs[i].setFrequency(hz);
}

void UnisonOscillator::setWaveform(Waveform wf)
{
    for (auto& o : oscs)
        o.setWaveform(wf);
}

void UnisonOscillator::setDetuneCents(float base)
{
    baseCents = base;
    recomputeDetune();
}

void UnisonOscillator::setUnison(int voices, float spread)
{
    unisonVoices = std::clamp(voices, 1, MaxUnison);
    spreadCents  = spread;
    recomputeDetune();
}

void UnisonOscillator::reset()
{
    for (auto& o : oscs)
        o.reset();
}

float UnisonOscillator::getNextSample()
{
    float sum = 0.f;
    for (int i = 0; i < unisonVoices; ++i)
        sum += oscs[i].getNextSample();
    return sum / std::sqrt(static_cast<float>(unisonVoices));
}

void UnisonOscillator::recomputeDetune()
{
    for (int i = 0; i < unisonVoices; ++i)
    {
        const float offset = (unisonVoices > 1)
            ? spreadCents * (2.0f * i / static_cast<float>(unisonVoices - 1) - 1.0f)
            : 0.f;
        oscs[i].setDetuneCents(baseCents + offset);
    }
}
