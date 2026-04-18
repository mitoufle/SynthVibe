#include "Filter.h"

void Filter::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    svf.prepare(spec);
    updateCoefficients();
}

void Filter::setType(FilterType type)
{
    filterType = type;
    switch (type)
    {
        case FilterType::LowPass:
            svf.setType(juce::dsp::StateVariableTPTFilterType::lowpass);  break;
        case FilterType::HighPass:
            svf.setType(juce::dsp::StateVariableTPTFilterType::highpass); break;
        case FilterType::BandPass:
            svf.setType(juce::dsp::StateVariableTPTFilterType::bandpass); break;
    }
}

void Filter::setCutoff(float freqHz)
{
    cutoff = juce::jlimit(20.f, 20000.f, freqHz);
    updateCoefficients();
}

void Filter::setResonance(float normalised)
{
    // Maps 0–1 to a musically useful Q range (0.5–12)
    resonance = juce::jmap(juce::jlimit(0.f, 1.f, normalised), 0.5f, 12.f);
    updateCoefficients();
}

void Filter::reset()
{
    svf.reset();
}

float Filter::processSample(float input)
{
    return svf.processSample(0, input);
}

void Filter::updateCoefficients()
{
    svf.setCutoffFrequency(cutoff);
    svf.setResonance(resonance);
}
