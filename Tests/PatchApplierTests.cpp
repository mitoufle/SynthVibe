#include <juce_audio_processors/juce_audio_processors.h>
#include "AI/PatchApplier.h"
#include "AI/Variation.h"
#include "Parameters/ParameterLayout.h"
#include "DummyProcessor.h"

namespace
{
    // Listener that counts begin/end gesture pairs.
    struct GestureCounter : public juce::AudioProcessorParameter::Listener
    {
        std::atomic<int> begins { 0 };
        std::atomic<int> ends   { 0 };
        void parameterValueChanged(int, float) override {}
        void parameterGestureChanged(int, bool starting) override
        {
            if (starting) ++begins; else ++ends;
        }
    };
}

struct PatchApplierTests : public juce::UnitTest
{
    PatchApplierTests() : juce::UnitTest("PatchApplier", "AISynth") {}

    // Resolve a known float-typed and choice-typed paramId on the dummy.
    static constexpr const char* kFloatId  = "osc1.level";
    static constexpr const char* kChoiceId = "osc1.wave";

    void runTest() override
    {
        beginTest("applies known float value correctly");
        {
            DummyProcessor proc;
            PatchApplier applier(proc.apvts);

            Variation v;
            v.params[kFloatId] = 0.7;
            const auto r = applier.apply(v);
            expectEquals(r.applied, 1);
            expectEquals(r.unknown, 0);

            auto* p = proc.apvts.getRawParameterValue(kFloatId);
            expectWithinAbsoluteError(p->load(), 0.7f, 0.01f);
        }

        beginTest("applies known choice index correctly");
        {
            DummyProcessor proc;
            PatchApplier applier(proc.apvts);

            Variation v;
            v.params[kChoiceId] = 2.0;
            const auto r = applier.apply(v);
            expectEquals(r.applied, 1);

            auto* p = proc.apvts.getRawParameterValue(kChoiceId);
            expect((int) p->load() == 2, "choice should be 2");
        }

        beginTest("unknown IDs are counted, no crash");
        {
            DummyProcessor proc;
            PatchApplier applier(proc.apvts);

            Variation v;
            v.params["bogus.id"] = 0.5;
            const auto r = applier.apply(v);
            expectEquals(r.unknown, 1);
            expectEquals(r.applied, 0);
        }

        beginTest("out-of-range float clamped");
        {
            DummyProcessor proc;
            PatchApplier applier(proc.apvts);

            Variation v;
            v.params[kFloatId] = 1.5;
            const auto r = applier.apply(v);
            expectEquals(r.applied, 1);
            expectEquals(r.clamped, 1);

            auto* p = proc.apvts.getRawParameterValue(kFloatId);
            expectWithinAbsoluteError(p->load(), 1.0f, 0.01f);
        }

        beginTest("gestures fire (1 begin + 1 end per applied param)");
        {
            DummyProcessor proc;
            PatchApplier applier(proc.apvts);

            GestureCounter counter;
            for (auto* p : proc.getParameters())
                p->addListener(&counter);

            Variation v;
            v.params[kFloatId] = 0.4;
            v.params[kChoiceId] = 1.0;
            v.params["bogus"] = 0.0;     // unknown — no gesture
            const auto r = applier.apply(v);

            for (auto* p : proc.getParameters())
                p->removeListener(&counter);

            expectEquals(r.applied, 2);
            expectEquals(counter.begins.load(), 2);
            expectEquals(counter.ends.load(), 2);
        }

        beginTest("empty params is no-op");
        {
            DummyProcessor proc;
            PatchApplier applier(proc.apvts);

            Variation v;   // params empty
            const auto r = applier.apply(v);
            expectEquals(r.applied, 0);
            expectEquals(r.unknown, 0);
            expectEquals(r.clamped, 0);
        }

        beginTest("PatchApplier sets osc1.wave=4 and osc1.table=2 -> params land in APVTS");
        {
            DummyProcessor proc;
            PatchApplier applier(proc.apvts);

            Variation v;
            v.params["osc1.wave"]  = 4.0;   // Wavetable
            v.params["osc1.table"] = 2.0;   // Bell
            const auto r = applier.apply(v);
            expectEquals(r.applied, 2);
            expectEquals(r.unknown, 0);

            expectEquals(static_cast<int>(*proc.apvts.getRawParameterValue("osc1.wave")),  4);
            expectEquals(static_cast<int>(*proc.apvts.getRawParameterValue("osc1.table")), 2);
        }

        beginTest("PatchApplier sets osc2.wave=4 and osc2.table=1 -> params land in APVTS");
        {
            DummyProcessor proc;
            PatchApplier applier(proc.apvts);

            Variation v;
            v.params["osc2.wave"]  = 4.0;
            v.params["osc2.table"] = 1.0;
            const auto r = applier.apply(v);
            expectEquals(r.applied, 2);
            expectEquals(r.unknown, 0);

            expectEquals(static_cast<int>(*proc.apvts.getRawParameterValue("osc2.wave")),  4);
            expectEquals(static_cast<int>(*proc.apvts.getRawParameterValue("osc2.table")), 1);
        }

        beginTest("resetToDefaults restores every APVTS param to its default value");
        {
            DummyProcessor proc;
            PatchApplier applier(proc.apvts);

            // Drift several params away from their defaults.
            Variation drift;
            drift.params["osc1.wave"]   = 4.0;   // Wavetable (not default 1=Saw)
            drift.params["osc1.table"]  = 3.0;   // Vocal (not default 0=Organ)
            drift.params["filt.cutoff"] = 200.0; // not default 8000
            applier.apply(drift);

            // Sanity: drift was applied
            expectEquals(static_cast<int>(*proc.apvts.getRawParameterValue("osc1.wave")), 4);

            // Reset
            applier.resetToDefaults();

            // After reset, the params are back to their declared defaults
            expectEquals(static_cast<int>(*proc.apvts.getRawParameterValue("osc1.wave")),  1);
            expectEquals(static_cast<int>(*proc.apvts.getRawParameterValue("osc1.table")), 0);
            expectWithinAbsoluteError(proc.apvts.getRawParameterValue("filt.cutoff")->load(), 8000.f, 1.f);
        }
    }
};

static PatchApplierTests _patchApplierTests;
