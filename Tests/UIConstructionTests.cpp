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
#include "UI/components/FxSlotCard.h"
#include "UI/components/FxChainStrip.h"
#include "UI/components/SegmentedButtonRow.h"
#include "UI/components/ArpOnOffPill.h"
#include "UI/SoundTab.h"
#include "UI/ModTab.h"
#include "UI/FXTab.h"
#include "UI/ArpTab.h"

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

        beginTest("FxSlotCard constructs and binds slot 1");
        {
            SynthVibe::FxSlotCard card(apvts,
                ParamIDs::fx1Type, ParamIDs::fx1Bypass, ParamIDs::fx1Mix,
                ParamIDs::fx1P1,   ParamIDs::fx1P2,    ParamIDs::fx1P3, ParamIDs::fx1P4,
                /*slotIndex=*/1);
            card.setBounds(0, 0, 220, 180);
            expect(card.getWidth() == 220);
            expectEquals(card.getSlotIndex(), 1);
        }

        beginTest("FxChainStrip renders 10 slots");
        {
            SynthVibe::FxChainStrip strip(apvts);
            strip.setBounds(0, 0, 1200, 380);
            expectEquals(strip.getNumSlots(), 10);
        }

        beginTest("FxTab constructs with chain strip attached");
        {
            FxTab tab(apvts);
            tab.setBounds(0, 0, 1280, 560);
            expectEquals(tab.getWidth(), 1280);
            expect(tab.getStrip() != nullptr, "FxTab should own a FxChainStrip");
            expectEquals(tab.getStrip()->getNumSlots(), 10);
        }

        beginTest("FxSlotCard applies per-type defaults on user pick of new type");
        {
            SynthVibe::FxSlotCard card(apvts,
                ParamIDs::fx1Type, ParamIDs::fx1Bypass, ParamIDs::fx1Mix,
                ParamIDs::fx1P1,   ParamIDs::fx1P2,    ParamIDs::fx1P3, ParamIDs::fx1P4,
                /*slotIndex=*/1);
            card.setBounds(0, 0, 220, 180);

            // Reset slot 1 to known initial state (global APVTS defaults).
            auto* tp = apvts.getParameter(ParamIDs::fx1Type);
            tp->setValueNotifyingHost(tp->convertTo0to1(0.f));        // None
            apvts.getParameter(ParamIDs::fx1Mix)->setValueNotifyingHost(1.0f);
            apvts.getParameter(ParamIDs::fx1P1) ->setValueNotifyingHost(0.5f);
            apvts.getParameter(ParamIDs::fx1P2) ->setValueNotifyingHost(0.5f);

            // Simulate user picking "Delay" from the dropdown (item ID 4 = enum 3).
            // Combo onChange fires synchronously and writes Delay's musical defaults
            // to mix/p1..p4 via setValueNotifyingHost.
            auto& combo = card.getTypePicker().getCombo();
            combo.setSelectedId(4, juce::sendNotificationSync);

            // Type updated by the ComboBoxAttachment.
            expectEquals(apvts.getRawParameterValue(ParamIDs::fx1Type)->load(), 3.f);
            // mix/p1/p2 reset to Delay's musical defaults from kFxTypeDefaults.
            expectWithinAbsoluteError(apvts.getRawParameterValue(ParamIDs::fx1Mix)->load(),
                                      0.3f, 0.001f);
            expectWithinAbsoluteError(apvts.getRawParameterValue(ParamIDs::fx1P1)->load(),
                                      0.1246f, 0.005f);
            expectWithinAbsoluteError(apvts.getRawParameterValue(ParamIDs::fx1P2)->load(),
                                      0.3684f, 0.005f);

            // Restore for downstream tests.
            tp->setValueNotifyingHost(tp->convertTo0to1(0.f));
        }

        beginTest("SegmentedButtonRow constructs with N labels and binds to choice param");
        {
            SynthVibe::SegmentedButtonRow row(apvts, ParamIDs::arpMode,
                juce::StringArray { "Up", "Dn", "UpDn", "DnUp", "Rnd", "Played", "Chord" },
                SynthVibe::Tokens::arp);
            row.setBounds(0, 0, 700, 28);
            expectEquals(row.getNumSegments(), 7);
            // arp.pattern default = index 2 (updn) → segment 2 active.
            expect(row.getSegmentButton(2).getToggleState() == true,
                   "segment 2 (updn) should be active by default");
        }

        beginTest("ArpOnOffPill toggles arp.on via attachment");
        {
            SynthVibe::ArpOnOffPill pill(apvts, ParamIDs::arpEnabled);
            pill.setBounds(0, 0, 100, 28);
            // Default arp.on = false → button off.
            expect(pill.getButton().getToggleState() == false, "default off");

            // Programmatically click → APVTS should reflect.
            pill.getButton().setToggleState(true, juce::sendNotificationSync);
            expectWithinAbsoluteError(
                apvts.getRawParameterValue(ParamIDs::arpEnabled)->load(), 1.0f, 0.001f);

            // Restore for downstream tests.
            pill.getButton().setToggleState(false, juce::sendNotificationSync);
        }

        beginTest("ArpTab atomized: owns pill, two segmented rows, latch toggle");
        {
            ArpTab tab(apvts);
            tab.setBounds(0, 0, 800, 240);
            expect(tab.getOnPill()       != nullptr, "missing OnPill");
            expect(tab.getPatternRow()   != nullptr, "missing PatternRow");
            expect(tab.getRateRow()      != nullptr, "missing RateRow");
            expect(tab.getLatchToggle()  != nullptr, "missing LatchToggle");
            expectEquals(tab.getPatternRow()->getNumSegments(), 7);
            expectEquals(tab.getRateRow()->getNumSegments(),    5);
        }
    }
};

static UIConstructionTests sUIConstructionTests;
