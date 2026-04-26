#pragma once
#include <juce_core/juce_core.h>

namespace SystemPrompt
{
    // Returns the assembled Claude system prompt: static template + ParamIdIndex table.
    // Deterministic: same string on every call within a process.
    juce::String build();
}
