#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include "AI/ParamIdIndex.h"
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
}

struct ParamIdIndexDriftTests : public juce::UnitTest
{
    ParamIdIndexDriftTests() : juce::UnitTest("ParamIdIndexDrift", "AISynth") {}

    void runTest() override
    {
        DummyProcessor proc;

        beginTest("every APVTS paramId has a ParamIdIndex entry");
        {
            juce::StringArray missing;
            juce::StringArray wrongLookup;
            for (auto* p : proc.getParameters())
            {
                auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p);
                if (rp == nullptr) continue;
                auto* hit = ParamIdIndex::find(rp->paramID);
                if (hit == nullptr) { missing.add(rp->paramID); continue; }
                if (hit->id != rp->paramID) wrongLookup.add(rp->paramID);
            }
            expect(missing.isEmpty(),
                   "ParamIdIndex.cpp is missing entries for: " + missing.joinIntoString(", "));
            expect(wrongLookup.isEmpty(),
                   "ParamIdIndex::find() returned an entry whose id != requested id for: "
                   + wrongLookup.joinIntoString(", "));
        }

        beginTest("every ParamIdIndex entry has an APVTS counterpart");
        {
            juce::StringArray apvtsIds;
            for (auto* p : proc.getParameters())
                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
                    apvtsIds.add(rp->paramID);

            juce::StringArray orphans;
            for (auto& e : ParamIdIndex::entries())
                if (! apvtsIds.contains(e.id))
                    orphans.add(e.id);

            expect(orphans.isEmpty(),
                   "ParamIdIndex has orphan entries (no APVTS counterpart): " + orphans.joinIntoString(", "));
        }

        beginTest("Type::Choice entries have choices.size() == int(maxValue + 1)");
        {
            juce::StringArray bad;
            for (auto& e : ParamIdIndex::entries())
            {
                if (e.type != ParamEntry::Type::Choice) continue;
                if ((int) e.choices.size() != (int) (e.maxValue + 1.0))
                    bad.add(e.id);
            }
            expect(bad.isEmpty(),
                   "Choice entries with mismatched choices.size(): " + bad.joinIntoString(", "));
        }
    }
};

static ParamIdIndexDriftTests _paramIdIndexDriftTests;
