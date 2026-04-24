#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters/ParameterLayout.h"
#include "Parameters/ParameterIDs.h"
#include "Parameters/ModDestinations.h"

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

        beginTest("Mod matrix IDs follow mod.N.suffix scheme");
        expectEquals(juce::String(ParamIDs::mod1Src),    juce::String("mod.1.src"));
        expectEquals(juce::String(ParamIDs::mod1Dst),    juce::String("mod.1.dst"));
        expectEquals(juce::String(ParamIDs::mod1Amount), juce::String("mod.1.amount"));
        expectEquals(juce::String(ParamIDs::mod1Curve),  juce::String("mod.1.curve"));
        expectEquals(juce::String(ParamIDs::mod8Src),    juce::String("mod.8.src"));
        expectEquals(juce::String(ParamIDs::mod16Curve), juce::String("mod.16.curve"));

        beginTest("kDestinations[] maps curated 13 entries (none + 12 targets)");
        expectEquals((int) SynthVibe::kDestinations.size(), 13);
        expect(juce::String(SynthVibe::kDestinations[0].paramId).isEmpty(),
               "slot 0 is the 'none' sentinel (empty paramId)");
        expectEquals(juce::String(SynthVibe::kDestinations[0].label), juce::String("None"));
        expectEquals(juce::String(SynthVibe::kDestinations[1].paramId), juce::String(ParamIDs::filterCutoff));
        expectEquals(juce::String(SynthVibe::kDestinations[1].label),   juce::String("Cutoff"));
        expectEquals(juce::String(SynthVibe::kDestinations[12].paramId), juce::String(ParamIDs::ampRelease));
        expectEquals(juce::String(SynthVibe::kDestinations[12].label),   juce::String("Amp Release"));

        beginTest("Mod slots register with correct defaults");
        {
            juce::AudioProcessorGraph dummyGraph;
            juce::AudioProcessorValueTreeState apvts(
                dummyGraph, nullptr, "AISynthState", ParameterLayout::create());

            auto readFloat = [&](const char* id) {
                auto* p = apvts.getRawParameterValue(id);
                expect(p != nullptr, juce::String("missing param: ") + id);
                return p ? p->load() : 0.f;
            };

            // Source default = 0 ("none")
            expectEquals(readFloat(ParamIDs::mod1Src), 0.f);
            // Destination default = 0 ("none")
            expectEquals(readFloat(ParamIDs::mod1Dst), 0.f);
            // Amount default = 0 (centre of -1..+1)
            expectWithinAbsoluteError(readFloat(ParamIDs::mod1Amount), 0.f, 0.001f);
            // Curve default = 0 ("lin")
            expectEquals(readFloat(ParamIDs::mod1Curve), 0.f);

            // Reserved slots 9..16 also present
            expect(apvts.getRawParameterValue(ParamIDs::mod16Amount) != nullptr,
                   "reserved slot 16 amount must be registered");
        }

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
