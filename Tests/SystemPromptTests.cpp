#include <juce_core/juce_core.h>
#include "AI/SystemPrompt.h"
#include "AI/ParamIdIndex.h"

struct SystemPromptTests : public juce::UnitTest
{
    SystemPromptTests() : juce::UnitTest("SystemPrompt", "AISynth") {}

    void runTest() override
    {
        beginTest("build() returns non-empty string");
        {
            const auto s = SystemPrompt::build();
            expect(s.isNotEmpty(), "system prompt must not be empty");
            expect(s.length() > 1000, "system prompt must include the param table; got length "
                                       + juce::String(s.length()));
        }

        beginTest("build() contains template anchors");
        {
            const auto s = SystemPrompt::build();
            expect(s.contains("set_patch"), "must mention the set_patch tool");
            expect(s.contains("paramId") || s.contains("param id"), "must mention paramId concept");
            expect(s.contains("0..1") || s.contains("0..(N-1)"), "must mention value ranges");
        }

        beginTest("build() contains every ParamIdIndex entry id");
        {
            const auto s = SystemPrompt::build();
            juce::StringArray missing;
            for (auto& e : ParamIdIndex::entries())
                if (! s.contains(e.id))
                    missing.add(e.id);
            expect(missing.isEmpty(),
                   "SystemPrompt missing paramIds: " + missing.joinIntoString(", "));
        }

        beginTest("system prompt warns Claude about Bell single-cycle limitation");
        {
            const auto p = SystemPrompt::build();
            expect(p.contains("Wavetable timbre rules:"),
                   "system prompt must have the Wavetable timbre rules section header");
            expect(p.contains("oscN.table to the desired index"),
                   "system prompt must contain the coupling rule for oscN.wave + oscN.table");
            expect(p.contains("V1 timbre limitations"),
                   "system prompt must contain the V1 caveat sub-block");
            expect(p.contains("Aa"),
                   "system prompt must specify the single 'Aa' vowel");
        }
    }
};

static SystemPromptTests _systemPromptTests;
