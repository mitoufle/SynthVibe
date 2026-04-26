#include <juce_audio_processors/juce_audio_processors.h>
#include "AI/PatchApplier.h"
#include "AI/Variation.h"
#include "Parameters/ParameterLayout.h"

namespace
{
    struct DummyProcessor : public juce::AudioProcessor
    {
        DummyProcessor()
            : juce::AudioProcessor(BusesProperties().withOutput("Out", juce::AudioChannelSet::stereo(), true)),
              apvts(*this, nullptr, "PARAMETERS", ParameterLayout::create()) {}

        const juce::String getName() const override { return "Dummy"; }
        void prepareToPlay(double, int) override {}
        void releaseResources() override {}
        void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
        juce::AudioProcessorEditor* createEditor() override { return nullptr; }
        bool hasEditor() const override { return false; }
        bool acceptsMidi() const override { return true; }
        bool producesMidi() const override { return false; }
        double getTailLengthSeconds() const override { return 0.0; }
        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram(int) override {}
        const juce::String getProgramName(int) override { return {}; }
        void changeProgramName(int, const juce::String&) override {}
        void getStateInformation(juce::MemoryBlock&) override {}
        void setStateInformation(const void*, int) override {}

        juce::AudioProcessorValueTreeState apvts;
    };

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
    }
};

static PatchApplierTests _patchApplierTests;
