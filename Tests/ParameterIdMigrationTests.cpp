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
        expectEquals(juce::String(ParamIDs::fx1Mix),        juce::String("fx.1.mix"));

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

        beginTest("FX slot IDs follow fx.N.suffix scheme");
        expectEquals(juce::String(ParamIDs::fx1Type),   juce::String("fx.1.type"));
        expectEquals(juce::String(ParamIDs::fx1Bypass), juce::String("fx.1.bypass"));
        expectEquals(juce::String(ParamIDs::fx1Mix),    juce::String("fx.1.mix"));
        expectEquals(juce::String(ParamIDs::fx1P1),     juce::String("fx.1.p1"));
        expectEquals(juce::String(ParamIDs::fx1P4),     juce::String("fx.1.p4"));
        expectEquals(juce::String(ParamIDs::fx10Type),  juce::String("fx.10.type"));
        expectEquals(juce::String(ParamIDs::fx10P4),    juce::String("fx.10.p4"));

        beginTest("Legacy hardcoded fx.* IDs are removed (preset break is intentional)");
        {
            juce::AudioProcessorGraph dummyGraph;
            juce::AudioProcessorValueTreeState apvts(
                dummyGraph, nullptr, "AISynthState", ParameterLayout::create());

            // Each of these used to exist in Phase 1; Phase 4 deletes them
            // as part of the slot migration. APVTS should report nullptr.
            for (auto* legacyId : {
                "fx.delay.time", "fx.delay.feedback", "fx.delay.mix",
                "fx.chorus.rate", "fx.chorus.depth", "fx.chorus.mix",
                "fx.reverb.room", "fx.reverb.damp", "fx.reverb.mix",
                "fx.drive.type", "fx.drive.amount", "fx.drive.mix" })
            {
                expect(apvts.getParameter(legacyId) == nullptr,
                       juce::String("legacy id should be gone: ") + legacyId);
            }
        }

        beginTest("FX slots register with correct defaults");
        {
            juce::AudioProcessorGraph dummyGraph;
            juce::AudioProcessorValueTreeState apvts(
                dummyGraph, nullptr, "AISynthState", ParameterLayout::create());

            auto readFloat = [&](const char* id) {
                auto* p = apvts.getRawParameterValue(id);
                expect(p != nullptr, juce::String("missing param: ") + id);
                return p ? p->load() : 0.f;
            };

            // Type default = 0 ("None")
            expectEquals(readFloat(ParamIDs::fx1Type), 0.f);
            // Bypass default = false (0)
            expectEquals(readFloat(ParamIDs::fx1Bypass), 0.f);
            // Mix default = 1.0 (full wet — slot is bypassed via type=None or bypass flag)
            expectWithinAbsoluteError(readFloat(ParamIDs::fx1Mix), 1.f, 0.001f);
            // p1..p4 default = 0.5 (mid range)
            expectWithinAbsoluteError(readFloat(ParamIDs::fx1P1), 0.5f, 0.001f);
            expectWithinAbsoluteError(readFloat(ParamIDs::fx1P4), 0.5f, 0.001f);

            // Slot 10 also present
            expect(apvts.getRawParameterValue(ParamIDs::fx10P4) != nullptr,
                   "slot 10 p4 must be registered");

            // 11 type choices: None + 10 effect types
            auto* typeParam = dynamic_cast<juce::AudioParameterChoice*>(
                apvts.getParameter(ParamIDs::fx1Type));
            expect(typeParam != nullptr, "fx.1.type must be a choice param");
            if (typeParam != nullptr)
                expectEquals(typeParam->choices.size(), 11);
        }

        beginTest("Arp params (existing + new) register with audit defaults");
        {
            juce::AudioProcessorGraph dummyGraph;
            juce::AudioProcessorValueTreeState apvts(
                dummyGraph, nullptr, "AISynthState", ParameterLayout::create());

            auto readFloat = [&](const char* id) {
                auto* p = apvts.getRawParameterValue(id);
                expect(p != nullptr, juce::String("missing param: ") + id);
                return p ? p->load() : 0.f;
            };

            // Existing arp params: defaults must match audit
            expectEquals(readFloat(ParamIDs::arpEnabled),     0.f);   // false
            expectEquals(readFloat(ParamIDs::arpMode),        2.f);   // index 2 = "updn"
            expectEquals(readFloat(ParamIDs::arpRate),        2.f);   // index 2 = "1/16"
            expectEquals(readFloat(ParamIDs::arpOctaveRange), 2.f);   // 2 octaves

            // New arp params: defaults from audit
            expectWithinAbsoluteError(readFloat(ParamIDs::arpGate),     0.58f, 0.001f);
            expectWithinAbsoluteError(readFloat(ParamIDs::arpSwing),    0.22f, 0.001f);
            expectWithinAbsoluteError(readFloat(ParamIDs::arpHumanize), 0.4f,  0.001f);
            expectEquals(readFloat(ParamIDs::arpLatch),                 0.f);  // false

            // Choice arity check (locks the new enum sizes — fails loudly if anyone
            // adds/removes patterns or rates without updating the engine)
            auto* pattern = dynamic_cast<juce::AudioParameterChoice*>(
                apvts.getParameter(ParamIDs::arpMode));
            expect(pattern != nullptr, "arp.pattern must be a choice param");
            if (pattern != nullptr) expectEquals(pattern->choices.size(), 7);

            auto* rate = dynamic_cast<juce::AudioParameterChoice*>(
                apvts.getParameter(ParamIDs::arpRate));
            expect(rate != nullptr, "arp.rate must be a choice param");
            if (rate != nullptr) expectEquals(rate->choices.size(), 5);

            // Negative test — documents the 1.A break: pattern index 3 is now "dnup",
            // not the legacy "Random". This locks the choice ordering.
            if (pattern != nullptr) expectEquals(pattern->choices[3], juce::String("dnup"));
            if (rate    != nullptr) expectEquals(rate->choices[3],    juce::String("1/16T"));
        }
    }
};

static ParameterIdMigrationTests sParameterIdMigrationTests;
