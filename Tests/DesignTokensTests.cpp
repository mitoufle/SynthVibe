#include <juce_core/juce_core.h>
#include "UI/DesignTokens.h"

struct DesignTokensTests : public juce::UnitTest
{
    DesignTokensTests() : juce::UnitTest("DesignTokens", "AISynth") {}

    void runTest() override
    {
        beginTest("Night Plum palette values");
        using namespace SynthVibe::Tokens;
        expectEquals(bg.getARGB(),     (juce::uint32) 0xFF0F0D14);
        expectEquals(panel.getARGB(),  (juce::uint32) 0xFF171421);
        expectEquals(accent.getARGB(), (juce::uint32) 0xFFC693E8);
        expectEquals(osc.getARGB(),    (juce::uint32) 0xFF9ABFE8);
        expectEquals(filter.getARGB(), (juce::uint32) 0xFFE8A3C7);

        beginTest("Arc geometry matches spec (225°/270°)");
        expectWithinAbsoluteError(knobArcStartDeg, 225.0f, 0.001f);
        expectWithinAbsoluteError(knobArcSweepDeg, 270.0f, 0.001f);
    }
};

static DesignTokensTests sDesignTokensTests;
