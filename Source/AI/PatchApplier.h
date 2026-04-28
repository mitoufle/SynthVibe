#pragma once
#include "Variation.h"
#include <juce_audio_processors/juce_audio_processors.h>

struct ApplyReport
{
    int applied = 0;
    int unknown = 0;
    int clamped = 0;
};

class PatchApplier
{
public:
    explicit PatchApplier(juce::AudioProcessorValueTreeState& apvts);

    // Must run on the message thread.
    ApplyReport apply(const Variation& variation);

    // Resets every APVTS parameter to its default value, firing change
    // gestures so host automation tracks the reset.
    void resetToDefaults();

private:
    juce::AudioProcessorValueTreeState& apvts;
};
