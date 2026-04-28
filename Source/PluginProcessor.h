#pragma once
#include <array>
#include <juce_audio_processors/juce_audio_processors.h>
#include "AI/ApiKeyStore.h"
#include "AI/JuceHttpTransport.h"
#include "AI/ClaudeClient.h"
#include "AI/PatchApplier.h"
#include "Engine/SynthEngine.h"
#include "FX/FxRunner.h"
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
    double getTailLengthSeconds() const override { return 10.0; }

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
    const WavetableBank& getWavetableBank() const noexcept { return synth.getWavetableBank(); }

    // Collects UI-driven key presses (from the editor's MidiKeyboardComponent).
    // Its events are merged into the MIDI buffer at the top of processBlock so
    // UI clicks and host-sent MIDI travel through the same path.
    juce::MidiKeyboardState keyboardState;

    // -----------------------------------------------------------------------
    // AI services (A.2). Public so the editor and future API layer can use them.
    // Declaration order is load-bearing: ClaudeClient holds refs to apiKeyStore
    // and httpTransport, and PatchApplier holds a ref to apvts.
    // -----------------------------------------------------------------------
    ApiKeyStore       apiKeyStore;
    JuceHttpTransport httpTransport;
    ClaudeClient      claudeClient { apiKeyStore, httpTransport };
    PatchApplier      patchApplier { apvts };

private:
    SynthEngine          synth;
    SynthVibe::FxRunner  fxRunner;
    ArpEngine            arp;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMasterVol;

    VoiceParams       buildVoiceParams()  const;
    ArpEngine::Params buildArpParams()    const;
    std::array<SynthVibe::FxSlotParams, SynthVibe::FxRunner::kNumSlots>
        buildFxSnapshot() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AISynthProcessor)
};
