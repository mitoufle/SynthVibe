#include "ParameterLayout.h"
#include "ParameterIDs.h"

using namespace juce;

AudioProcessorValueTreeState::ParameterLayout ParameterLayout::create()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    // -----------------------------------------------------------------------
    // Oscillator
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParamIDs::oscWaveform, "Waveform",
        StringArray { "Sine", "Saw", "Square", "Triangle" }, 1));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::oscDetune, "Detune",
        NormalisableRange<float>(-100.f, 100.f, 0.1f), 0.f));

    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::oscOctave, "Octave", -2, 2, 0));

    // -----------------------------------------------------------------------
    // Filter
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParamIDs::filterType, "Filter Type",
        StringArray { "Low Pass", "High Pass", "Band Pass" }, 0));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::filterCutoff, "Cutoff",
        NormalisableRange<float>(20.f, 20000.f, 0.1f, 0.3f), 8000.f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::filterResonance, "Resonance",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 0.1f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::filterEnvAmt, "Filter Env",
        NormalisableRange<float>(-1.f, 1.f, 0.001f), 0.f));

    // -----------------------------------------------------------------------
    // Amplitude envelope
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::ampAttack, "Amp Attack",
        NormalisableRange<float>(0.001f, 10.f, 0.001f, 0.4f), 0.01f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::ampDecay, "Amp Decay",
        NormalisableRange<float>(0.001f, 10.f, 0.001f, 0.4f), 0.1f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::ampSustain, "Amp Sustain",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 0.7f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::ampRelease, "Amp Release",
        NormalisableRange<float>(0.001f, 10.f, 0.001f, 0.4f), 0.2f));

    // -----------------------------------------------------------------------
    // Filter envelope
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::fltAttack, "Flt Attack",
        NormalisableRange<float>(0.001f, 10.f, 0.001f, 0.4f), 0.01f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::fltDecay, "Flt Decay",
        NormalisableRange<float>(0.001f, 10.f, 0.001f, 0.4f), 0.2f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::fltSustain, "Flt Sustain",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 0.5f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::fltRelease, "Flt Release",
        NormalisableRange<float>(0.001f, 10.f, 0.001f, 0.4f), 0.3f));

    // -----------------------------------------------------------------------
    // Master
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::masterVolume, "Master Volume",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 0.8f));

    // -----------------------------------------------------------------------
    // Delay
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::delayTime, "Delay Time",
        NormalisableRange<float>(1.f, 2000.f, 0.1f, 0.4f), 250.f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::delayFeedback, "Delay Feedback",
        NormalisableRange<float>(0.f, 0.95f, 0.001f), 0.35f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::delayMix, "Delay Mix",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 0.0f));

    return { params.begin(), params.end() };
}
