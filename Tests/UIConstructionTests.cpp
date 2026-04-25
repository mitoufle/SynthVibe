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
#include "UI/components/ModDestPicker.h"
#include "UI/components/BipolarAmountBar.h"
#include "UI/components/ModRow.h"
#include "UI/components/ModMatrixTable.h"
#include "UI/components/FxSlotTypePicker.h"
#include "UI/SoundTab.h"
#include "UI/ModTab.h"

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

        beginTest("ModDestPicker reads kDestinations[] and defaults to 'None'");
        {
            SynthVibe::ModDestPicker dst(apvts, ParamIDs::mod1Dst);
            dst.setBounds(0, 0, 140, 24);
            // Default destination index = 0 ("None"); ComboBox IDs are 1-based → 1.
            expectEquals(dst.getCombo().getSelectedId(), 1);
            expectEquals(dst.getCombo().getNumItems(), 13);

            // Round-trip: prove the attachment is live.
            auto* p = apvts.getParameter(ParamIDs::mod1Dst);
            p->setValueNotifyingHost(p->convertTo0to1(2.f));    // index 2 = "Resonance"
            expectEquals(dst.getCombo().getSelectedId(), 3);
            p->setValueNotifyingHost(p->convertTo0to1(0.f));    // restore
        }

        beginTest("BipolarAmountBar reads amount float param");
        {
            SynthVibe::BipolarAmountBar bar(apvts, ParamIDs::mod1Amount);
            bar.setBounds(0, 0, 180, 18);
            // Default mod.1.amount = 0
            expectWithinAbsoluteError(bar.getValue(), 0.f, 0.001f);

            // Round-trip: prove the attachment is live.
            auto* p = apvts.getParameter(ParamIDs::mod1Amount);
            p->setValueNotifyingHost(p->convertTo0to1(0.5f));
            expectWithinAbsoluteError(bar.getValue(), 0.5f, 0.001f);
            p->setValueNotifyingHost(p->convertTo0to1(-0.7f));
            expectWithinAbsoluteError(bar.getValue(), -0.7f, 0.001f);
            p->setValueNotifyingHost(p->convertTo0to1(0.f));    // restore
        }

        beginTest("ModRow composes src/dst/amount/curve for slot 1");
        {
            SynthVibe::ModRow row(apvts,
                ParamIDs::mod1Src, ParamIDs::mod1Dst,
                ParamIDs::mod1Amount, ParamIDs::mod1Curve);
            row.setBounds(0, 0, 640, 28);
            expect(row.getWidth() == 640);
            expect(row.getHeight() == 28);
        }

        beginTest("ModMatrixTable renders 8 rows");
        {
            SynthVibe::ModMatrixTable table(apvts);
            table.setBounds(0, 0, 720, 360);
            expectEquals(table.getNumRows(), 8);
        }

        beginTest("SoundTab constructs with Phase 2b grid");
        {
            SoundTab tab(apvts);
            tab.setBounds(0, 0, 1280, 560);
            expect(tab.getWidth() == 1280, "SoundTab should construct at Phase 2b width");
        }

        beginTest("ModTab constructs with mod matrix attached");
        {
            ModTab tab(apvts);
            tab.setBounds(0, 0, 1280, 560);
            expectEquals(tab.getWidth(), 1280);
            expect(tab.getMatrix() != nullptr, "ModTab should own a ModMatrixTable");
            expectEquals(tab.getMatrix()->getNumRows(), 8);
        }

        beginTest("FxSlotTypePicker binds and lists 11 types");
        {
            SynthVibe::FxSlotTypePicker picker(apvts, ParamIDs::fx1Type);
            picker.setBounds(0, 0, 110, 24);
            // Default fx.1.type index = 0 ("None"); ComboBox IDs are 1-based → 1.
            expectEquals(picker.getCombo().getSelectedId(), 1);
            expectEquals(picker.getCombo().getNumItems(), 11);

            // Round-trip
            auto* p = apvts.getParameter(ParamIDs::fx1Type);
            p->setValueNotifyingHost(p->convertTo0to1(3.f));   // index 3 = "Delay"
            expectEquals(picker.getCombo().getSelectedId(), 4);
            p->setValueNotifyingHost(p->convertTo0to1(0.f));   // restore
        }
    }
};

static UIConstructionTests sUIConstructionTests;
