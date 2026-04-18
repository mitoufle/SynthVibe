#pragma once
#include <juce_dsp/juce_dsp.h>

enum class FilterType { LowPass = 0, HighPass, BandPass };

// State-variable filter wrapper. One instance per voice.
// To add new filter modes: extend FilterType and add a case in setType().
class Filter
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void setType(FilterType type);
    void setCutoff(float freqHz);
    void setResonance(float normalised); // 0–1 → mapped to Q range
    void reset();

    float processSample(float input);

private:
    void updateCoefficients();

    juce::dsp::StateVariableTPTFilter<float> svf;
    FilterType filterType = FilterType::LowPass;
    float      cutoff     = 8000.f;
    float      resonance  = 0.7f;
    double     sampleRate = 44100.0;
};
