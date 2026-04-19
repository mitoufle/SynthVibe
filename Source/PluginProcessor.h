#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Engine/SynthEngine.h"
#include "FX/FXChain.h"
#include "Engine/ArpEngine.h"
#include "Parameters/ParameterLayout.h"
#include "Presets/PresetManager.h"

class AISynthProcessor : public juce::AudioProcessor
{
public:
    AISynthProcessor();
    ~AISynthProcessor() override = default;

    // -----------------------------------------------------------------------
    // AudioProcessor interface
    // -----------------------------------------------------------------------
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi()  const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int  getNumPrograms()    override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& dest) override;
    void setStateInformation(const void* data, int size) override;

    // Public so the editor and future AI layer can read/write parameters
    juce::AudioProcessorValueTreeState apvts;
    PresetManager presetManager { apvts };
    int getActiveVoiceCount() const noexcept { return synth.getActiveVoiceCount(); }

private:
    SynthEngine synth;
    FXChain     fxChain;
    ArpEngine   arp;

    VoiceParams       buildVoiceParams()  const;
    Delay::Params     buildDelayParams()  const;
    Chorus::Params    buildChorusParams() const;
    Reverb::Params    buildReverbParams() const;
    ArpEngine::Params buildArpParams()    const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AISynthProcessor)
};
