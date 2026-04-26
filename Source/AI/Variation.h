#pragma once
#include <juce_core/juce_core.h>
#include <map>

struct Variation
{
    juce::String name;
    juce::String description;
    juce::StringArray tags;
    std::map<juce::String, double> params;
};
