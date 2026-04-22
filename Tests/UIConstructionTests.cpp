#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters/ParameterLayout.h"
#include "Parameters/ParameterIDs.h"
#include "UI/components/ArcKnob.h"
#include "UI/components/PanelHeader.h"
#include "UI/components/WaveTypeSelect.h"
#include "UI/components/FilterTypeSelect.h"

struct UIConstructionTests : public juce::UnitTest
{
    UIConstructionTests() : juce::UnitTest("UIConstruction", "AISynth") {}

    void runTest() override
    {
        beginTest("ArcKnob constructs and attaches to APVTS");

        juce::AudioProcessorGraph dummyGraph;
        juce::AudioProcessorValueTreeState apvts(
            dummyGraph, nullptr, "AISynthState", ParameterLayout::create());

        SynthVibe::ArcKnob knob("Cutoff", apvts, ParamIDs::filterCutoff,
                                SynthVibe::Tokens::filter, " Hz", 0);
        knob.setBounds(0, 0, 60, 80);

        expectEquals(knob.getSlider().getValue(), 8000.0);    // default from ParameterLayout

        beginTest("PanelHeader constructs");
        {
            SynthVibe::PanelHeader h("OSC 1", SynthVibe::Tokens::osc);
            h.setBounds(0, 0, 200, 20);
            expect(h.isVisible() == false); // not yet added to a parent
        }

        beginTest("WaveTypeSelect binds to osc1.wave");
        {
            SynthVibe::WaveTypeSelect ws(apvts, ParamIDs::osc1Waveform);
            ws.setBounds(0, 0, 120, 26);
            // osc1.wave default index is 1 ("Saw"); ComboBox IDs are 1-based, so ID = index + 1 = 2.
            expectEquals(ws.getCombo().getSelectedId(), 2);
        }

        beginTest("FilterTypeSelect constructs and binds to filt.type");
        {
            SynthVibe::FilterTypeSelect fts(apvts, ParamIDs::filterType);
            fts.setBounds(0, 0, 150, 28);
        }
    }
};

static UIConstructionTests sUIConstructionTests;
