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
            const float bp = svf1.processSample(0, input);
            return input - 2.f * bp;
        }
    }
    return input;
}

void Filter::updateCoefficients()
{
    svf1.setCutoffFrequency(cutoff);
    svf1.setResonance(resonance);
    svf2.setCutoffFrequency(cutoff);
    svf2.setResonance(resonance);
}
