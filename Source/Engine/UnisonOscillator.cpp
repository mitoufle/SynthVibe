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
    const int prevVoices = unisonVoices;
    unisonVoices = std::clamp(voices, 1, MaxUnison);
    spreadCents  = spread;
    invNormGain  = 1.f / std::sqrt(static_cast<float>(unisonVoices));
    recomputeDetune();
    recomputePan();
    // Stagger phases for newly activated slots so they enter spread rather than
    // all starting at phase 0 and causing a transient click.
    if (unisonVoices > prevVoices && unisonVoices > 1)
        for (int i = prevVoices; i < unisonVoices; ++i)
            oscs[i].setPhase(static_cast<double>(i) / unisonVoices);
}

void UnisonOscillator::setStereoSpread(float spread)
{
    stereoSpread = spread;
    recomputePan();
}

void UnisonOscillator::reset()
{
    for (int i = 0; i < MaxUnison; ++i)
        oscs[i].reset();

    if (unisonVoices > 1)
        for (int i = 0; i < unisonVoices; ++i)
            oscs[i].setPhase(static_cast<double>(i) / unisonVoices);
}

void UnisonOscillator::getNextSample(float& outL, float& outR)
{
    outL = 0.f;
    outR = 0.f;
    for (int i = 0; i < unisonVoices; ++i)
    {
        const float s = oscs[i].getNextSample() * invNormGain;
        outL += s * lGains[i];
        outR += s * rGains[i];
    }
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

void UnisonOscillator::recomputePan()
{
    constexpr float pi = 3.14159265358979f;
    for (int i = 0; i < unisonVoices; ++i)
    {
        const float pan = (unisonVoices > 1)
            ? stereoSpread * (2.f * i / static_cast<float>(unisonVoices - 1) - 1.f)
            : 0.f;
        const float angle = (pan + 1.f) * pi / 4.f;
        lGains[i] = std::cos(angle);
        rGains[i] = std::sin(angle);
    }
}
