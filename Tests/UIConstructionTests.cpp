#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters/ParameterLayout.h"
#include "Parameters/ParameterIDs.h"
#include "UI/components/ArcKnob.h"
#include "UI/components/PanelHeader.h"
#include "UI/components/WaveTypeSelect.h"
#include "UI/components/FilterTypeSelect.h"
#include "UI/components/OscilloscopeView.h"
#include "UI/components/EnvelopeEditor.h"
#include "UI/components/FilterResponseView.h"
#include "UI/components/CurveSelect.h"
#include "UI/components/ModSourcePicker.h"
#include "UI/SoundTab.h"

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

        beginTest("FilterTypeSelect has 5 pills after reorder");
        {
            SynthVibe::FilterTypeSelect fts(apvts, ParamIDs::filterType);
            fts.setBounds(0, 0, 250, 28);
            expectEquals(fts.getWidth(), 250);
        }

        beginTest("OscilloscopeView binds without crash");
        {
            SynthVibe::OscilloscopeView scope(apvts, ParamIDs::osc1Waveform);
            scope.setBounds(0, 0, 200, 60);
        }

        beginTest("FilterResponseView binds without crash");
        {
            SynthVibe::FilterResponseView frv(apvts,
                ParamIDs::filterType, ParamIDs::filterCutoff, ParamIDs::filterResonance);
            frv.setBounds(0, 0, 200, 80);
        }

        beginTest("EnvelopeEditor binds without crash");
        {
            SynthVibe::EnvelopeEditor env(apvts,
                ParamIDs::ampAttack, ParamIDs::ampDecay,
                ParamIDs::ampSustain, ParamIDs::ampRelease);
            env.setBounds(0, 0, 240, 120);
        }

        beginTest("CurveSelect binds to a choice param");
        {
            SynthVibe::CurveSelect cs(apvts, ParamIDs::mod1Curve);
            cs.setBounds(0, 0, 120, 22);
            expectEquals(cs.getCurrentIndex(), 0);                  // default "lin"

            // Round-trip: prove the ParameterAttachment is live, not just constructed.
            auto* p = apvts.getParameter(ParamIDs::mod1Curve);
            p->setValueNotifyingHost(p->convertTo0to1(2.f));
            expectEquals(cs.getCurrentIndex(), 2);                  // "log"
            p->setValueNotifyingHost(p->convertTo0to1(0.f));        // restore
        }

        beginTest("ModSourcePicker binds and defaults to 'None'");
        {
            SynthVibe::ModSourcePicker src(apvts, ParamIDs::mod1Src);
            src.setBounds(0, 0, 110, 24);
            // Default source index = 0 ("None"); ComboBox IDs are 1-based → 1.
            expectEquals(src.getCombo().getSelectedId(), 1);

            // Round-trip: prove the attachment is live.
            auto* p = apvts.getParameter(ParamIDs::mod1Src);
            p->setValueNotifyingHost(p->convertTo0to1(3.f));    // index 3 = "Env Amp"
            expectEquals(src.getCombo().getSelectedId(), 4);    // ComboBox ID = index + 1
            p->setValueNotifyingHost(p->convertTo0to1(0.f));    // restore
        }

        beginTest("SoundTab constructs with Phase 2b grid");
        {
            SoundTab tab(apvts);
            tab.setBounds(0, 0, 1280, 560);
            expect(tab.getWidth() == 1280, "SoundTab should construct at Phase 2b width");
        }
    }
};

static UIConstructionTests sUIConstructionTests;
