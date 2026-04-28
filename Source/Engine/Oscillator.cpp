#include "Oscillator.h"
#include "WavetableBank.h"
#include <juce_core/juce_core.h>

double Oscillator::getDetuneRatio() const noexcept
{
    return std::pow(2.0, static_cast<double>(detuneCents) / 1200.0);
}

// PolyBLEP correction term — reduces aliasing at discontinuities
float Oscillator::polyBlep(double t, double dt) const noexcept
{
    if (t < dt) {
        t /= dt;
        return static_cast<float>(2.0 * t - t * t - 1.0);
    }
    if (t > 1.0 - dt) {
        t = (t - 1.0) / dt;
        return static_cast<float>(t * t + 2.0 * t + 1.0);
    }
    return 0.f;
}

float Oscillator::getNextSample()
{
    const double detunedFreq = frequency * getDetuneRatio();
    const double dt          = detunedFreq / sampleRate;

    float sample = 0.f;

    switch (waveform)
    {
        case Waveform::Sine:
            sample = static_cast<float>(std::sin(juce::MathConstants<double>::twoPi * phase));
            break;

        case Waveform::Saw:
            sample  = static_cast<float>(2.0 * phase - 1.0);
            sample -= polyBlep(phase, dt);
            break;

        case Waveform::Square:
        {
            const double pw = static_cast<double>(pulseWidth);
            sample  = phase < pw ? 1.f : -1.f;
            // Rising edge at phase=0
            sample += polyBlep(phase, dt);
            // Falling edge at phase=pulseWidth — wrap into [0,1)
            sample -= polyBlep(std::fmod(phase - pw + 1.0, 1.0), dt);
            break;
        }

        case Waveform::Triangle:
            sample = phase < 0.5 ? static_cast<float>(4.0 * phase - 1.0)
                                 : static_cast<float>(3.0 - 4.0 * phase);
            break;

        case Waveform::Wavetable:
            if (bank != nullptr)
                sample = bank->fetchSample(tableIdx, phase, dt);
            // else: silent fallthrough (sample remains 0.f)
            break;
    }

    phase += dt;
    if (phase >= 1.0) phase -= 1.0;

    return sample;
}
