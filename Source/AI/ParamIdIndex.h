#pragma once
#include <juce_core/juce_core.h>
#include <vector>

struct ParamEntry
{
    juce::String id;
    juce::String label;
    enum class Type { Float, Choice, Bool, Int } type;
    double minValue     = 0.0;
    double maxValue     = 1.0;
    double defaultValue = 0.0;
    juce::StringArray choices;   // populated only for Type::Choice (one label per index)
};

namespace ParamIdIndex
{
    const std::vector<ParamEntry>& entries();
    const ParamEntry* find(const juce::String& id);
    juce::String renderForPrompt();   // markdown-style table for the system prompt
}
