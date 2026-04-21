#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters/ParameterLayout.h"
#include "Parameters/ParameterIDs.h"

struct ParameterIdMigrationTests : public juce::UnitTest
{
    ParameterIdMigrationTests() : juce::UnitTest("ParameterIdMigration", "AISynth") {}

    void runTest() override
    {
        beginTest("IDs use dot-separated scheme");
        expectEquals(juce::String(ParamIDs::osc1Waveform),  juce::String("osc1.wave"));
        expectEquals(juce::String(ParamIDs::filterCutoff),  juce::String("filt.cutoff"));
        expectEquals(juce::String(ParamIDs::ampAttack),     juce::String("env.amp.attack"));
        expectEquals(juce::String(ParamIDs::masterVolume),  juce::String("master.vol"));
        expectEquals(juce::String(ParamIDs::delayTime),     juce::String("fx.delay.time"));
    }
};

static ParameterIdMigrationTests sParameterIdMigrationTests;
