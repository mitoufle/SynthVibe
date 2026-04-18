#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace ParameterLayout
{
    // Returns the full APVTS layout.
    // Add new parameters in the .cpp — the processor and editor pick them up automatically.
    juce::AudioProcessorValueTreeState::ParameterLayout create();
}
