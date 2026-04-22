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

        beginTest("Phase 2b new parameters are registered with correct defaults");
        {
            juce::AudioProcessorGraph dummyGraph;
            juce::AudioProcessorValueTreeState apvts(
                dummyGraph, nullptr, "AISynthState", ParameterLayout::create());

            auto readFloat = [&](const char* id) {
                auto* p = apvts.getRawParameterValue(id);
                expect(p != nullptr, juce::String("missing param: ") + id);
                return p ? p->load() : 0.f;
            };

            expectWithinAbsoluteError(readFloat(ParamIDs::osc1Phase),       0.f,  0.001f);
            expectWithinAbsoluteError(readFloat(ParamIDs::osc1Pwm),         0.5f, 0.001f);
            expectWithinAbsoluteError(readFloat(ParamIDs::osc2Phase),       0.f,  0.001f);
            expectWithinAbsoluteError(readFloat(ParamIDs::osc2Pwm),         0.5f, 0.001f);
            expectEquals(readFloat(ParamIDs::osc2UnisonVoices),             1.f);
            expectWithinAbsoluteError(readFloat(ParamIDs::osc2UnisonDetune),0.f,  0.001f);
            expectWithinAbsoluteError(readFloat(ParamIDs::osc2UnisonSpread),0.5f, 0.001f);
            expectWithinAbsoluteError(readFloat(ParamIDs::filterDrive),     0.f,  0.001f);
            expectWithinAbsoluteError(readFloat(ParamIDs::filterKeytrack),  0.f,  0.001f);
        }
    }
};

static ParameterIdMigrationTests sParameterIdMigrationTests;
