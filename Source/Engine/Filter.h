#pragma once
#include <juce_dsp/juce_dsp.h>

enum class FilterType { LP12 = 0, LP24, HP, BP, Notch };

// State-variable filter with LP24 cascade, Notch identity, and a pre-filter
// tanh drive stage. One instance per voice channel.
class Filter
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void setType(FilterType type);
    void setCutoff(float freqHz);
    void setResonance(float normalised); // 0–1 → mapped to Q range
    void setDrive(float normalised);     // 0–1 → pre-gain factor 1..10
    void reset();

    float processSample(float input);

private:
    void updateCoefficients();
    float effectiveStageResonance() const noexcept;

    juce::dsp::StateVariableTPTFilter<float> svf1;
    juce::dsp::StateVariableTPTFilter<float> svf2;  // cascaded second stage for LP24
    FilterType filterType = FilterType::LP12;
    float      cutoff     = 8000.f;
    float      resonance  = 0.7f;
    float      drive      = 0.f;
    double     sampleRate = 44100.0;
};
