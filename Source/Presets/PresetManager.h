#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Manages saving and loading named presets to/from disk.
// The AI layer will use this to persist and recall sounds described by prompts.
class PresetManager
{
public:
    explicit PresetManager(juce::AudioProcessorValueTreeState& apvts);

    void savePreset(const juce::String& name);
    bool loadPreset(const juce::String& name);
    juce::StringArray getPresetNames() const;

    juce::File getPresetsDirectory() const;

private:
    juce::AudioProcessorValueTreeState& apvts;
};
