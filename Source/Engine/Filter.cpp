#include "Filter.h"
#include <cmath>

void Filter::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    svf1.prepare(spec);
    svf2.prepare(spec);
    updateCoefficients();
}

void Filter::setType(FilterType type)
{
    filterType = type;
    switch (type)
    {
        case FilterType::LP12:
            svf1.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            svf2.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            break;
        case FilterType::LP24:
            svf1.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            svf2.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            break;
        case FilterType::HP:
            svf1.setType(juce::dsp::StateVariableTPTFilterType::highpass);
            break;
        case FilterType::BP:
            svf1.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
            break;
        case FilterType::Notch:
            svf1.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
            break;
    }
}

void Filter::setCutoff(float freqHz)
{
    cutoff = juce::jlimit(20.f, 20000.f, freqHz);
    updateCoefficients();
}

void Filter::setResonance(float normalised)
{
    resonance = juce::jmap(juce::jlimit(0.f, 1.f, normalised), 0.5f, 12.f);
    updateCoefficients();
}

float Filter::effectiveStageResonance() const noexcept
{
    // LP24 cascades two identical-Q lowpass stages; Q² at cutoff would clip
    // at high resonance. Using √Q per stage keeps the cascade's peak at ≈ Q,
    // matching LP12 loudness and preventing crackling.
    return filterType == FilterType::LP24 ? std::sqrt(resonance) : resonance;
}

void Filter::setDrive(float normalised)
{
    drive = juce::jlimit(0.f, 1.f, normalised);
}

void Filter::reset()
{
    svf1.reset();
    svf2.reset();
}

float Filter::processSample(float input)
{
    // Pre-filter soft clip
    if (drive > 0.f)
    {
        const float gain = 1.f + drive * 9.f;   // 0..1 → 1..10 pre-gain
        input = std::tanh(input * gain);
    }

    switch (filterType)
    {
        case FilterType::LP12:
        case FilterType::HP:
        case FilterType::BP:
            return svf1.processSample(0, input);

        case FilterType::LP24:
            return svf2.processSample(0, svf1.processSample(0, input));

        case FilterType::Notch:
        {
            // JUCE TPT yBP peaks at Q, not 0 dB. Canonical SVF notch identity
            // in TPT form is: notch = input − R2·yBP where R2 = 1/Q.
            const float bp = svf1.processSample(0, input);
            const float R2 = 1.f / juce::jmax(0.1f, resonance);
            return input - R2 * bp;
        }
    }
    return input;
}

void Filter::updateCoefficients()
{
    const float q = effectiveStageResonance();
    svf1.setCutoffFrequency(cutoff);
    svf1.setResonance(q);
    svf2.setCutoffFrequency(cutoff);
    svf2.setResonance(q);
}
