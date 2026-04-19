#include "UnisonOscillator.h"
#include <cmath>

void UnisonOscillator::setSampleRate(double sr)
{
    for (auto& o : oscs)
        o.setSampleRate(sr);
}

// Only active slots need frequency updates; inactive slots will be synced
// when unisonVoices increases (setUnison is always called before setFrequency in setParams).
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
    invNormGain = 1.f / std::sqrt(static_cast<float>(unisonVoices));
}

void UnisonOscillator::reset()
{
    for (int i = 0; i < MaxUnison; ++i)
    {
        oscs[i].reset();
        if (unisonVoices > 1)
            oscs[i].setPhase(static_cast<double>(i) / unisonVoices);
    }
}

float UnisonOscillator::getNextSample()
{
    float sum = 0.f;
    for (int i = 0; i < unisonVoices; ++i)
        sum += oscs[i].getNextSample();
    return sum * invNormGain;
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
