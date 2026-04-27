#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters/ParameterLayout.h"

// Lightweight juce::AudioProcessor with the real APVTS layout — used by
// PatchApplier and AiPromptModal tests so we can drive the same param IDs
// the synth uses without spinning up the full plugin machinery.
struct DummyProcessor : public juce::AudioProcessor
{
    DummyProcessor()
        : juce::AudioProcessor(BusesProperties().withOutput("Out", juce::AudioChannelSet::stereo(), true)),
          apvts(*this, nullptr, "PARAMETERS", ParameterLayout::create()) {}

    const juce::String getName() const override { return "Dummy"; }
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    juce::AudioProcessorValueTreeState apvts;
};
