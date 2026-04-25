#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "ArcKnob.h"
#include "FxSlotTypePicker.h"
#include "../DesignTokens.h"
#include "../Fonts.h"
#include "../../FX/FxTypeMap.h"

namespace SynthVibe
{
    // One FX slot: a small card showing slot number, type-picker, bypass dot,
    // mix knob, and 4 generic p1..p4 knobs. The knob LABELS update when the
    // type changes (subscribed via ParameterAttachment); knobs whose label
    // is empty (e.g. p3 of Drive) are hidden.
    class FxSlotCard : public juce::Component
    {
    public:
        FxSlotCard(juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& typeId,
                   const juce::String& bypassId,
                   const juce::String& mixId,
                   const juce::String& p1Id,
                   const juce::String& p2Id,
                   const juce::String& p3Id,
                   const juce::String& p4Id,
                   int slotIndex)
            : slotIdx(slotIndex),
              typePicker(apvts, typeId),
              mixKnob ("Mix", apvts, mixId, Tokens::fx, "", 2),
              p1Knob  ("",    apvts, p1Id,  Tokens::fx, "", 2),
              p2Knob  ("",    apvts, p2Id,  Tokens::fx, "", 2),
              p3Knob  ("",    apvts, p3Id,  Tokens::fx, "", 2),
              p4Knob  ("",    apvts, p4Id,  Tokens::fx, "", 2)
        {
            addAndMakeVisible(typePicker);
            addAndMakeVisible(mixKnob);
            addAndMakeVisible(p1Knob);
            addAndMakeVisible(p2Knob);
            addAndMakeVisible(p3Knob);
            addAndMakeVisible(p4Knob);

            // Bypass: a dot toggle in the card header
            bypassButton.setClickingTogglesState(true);
            bypassButton.setButtonText("");
            bypassAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                apvts, bypassId, bypassButton);
            addAndMakeVisible(bypassButton);

            // Watch the type param: relabel the knobs (and hide empty ones).
            typeAttach = std::make_unique<juce::ParameterAttachment>(
                *apvts.getParameter(typeId),
                [this](float v) {
                    const int idx = juce::jlimit(0, kFxTypeCount - 1,
                                                 (int) std::round(v));
                    applyTypeLabels(idx);
                });
            typeAttach->sendInitialUpdate();

            // User-driven type change applies that type's musical defaults to
            // mix/p1..p4. Preset loads bypass this path (ComboBoxAttachment uses
            // dontSendNotification on inbound updates → onChange does not fire),
            // so loaded preset values are preserved.
            typePicker.getCombo().onChange = [this, &apvts, mixId, p1Id, p2Id, p3Id, p4Id]
            {
                const int typeIdx = typePicker.getCombo().getSelectedId() - 1;
                if (typeIdx <= 0 || typeIdx >= kFxTypeCount) return; // None / invalid: skip
                const auto& def = kFxTypeDefaults[(size_t) typeIdx];
                auto setNorm = [&apvts](const juce::String& id, float value01)
                {
                    if (auto* p = apvts.getParameter(id))
                        p->setValueNotifyingHost(value01);
                };
                setNorm(mixId, def.mix);
                setNorm(p1Id,  def.p1);
                setNorm(p2Id,  def.p2);
                setNorm(p3Id,  def.p3);
                setNorm(p4Id,  def.p4);
            };
        }

        int  getSlotIndex() const noexcept { return slotIdx; }
        FxSlotTypePicker& getTypePicker() noexcept { return typePicker; }

        void paint(juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat();
            g.setColour(Tokens::panel2);
            g.fillRoundedRectangle(b, Tokens::radiusMd);
            g.setColour(Tokens::edge);
            g.drawRoundedRectangle(b.reduced(0.5f), Tokens::radiusMd, 1.f);

            // Slot index badge in the header
            g.setColour(Tokens::ink3);
            g.setFont(juce::Font(Tokens::Font::tiny));
            g.drawText(juce::String(slotIdx),
                       getLocalBounds().withSize(18, 18).reduced(2),
                       juce::Justification::centred);
        }

        void resized() override
        {
            using namespace Tokens;
            auto area = getLocalBounds().reduced(spaceSm);

            // Top header strip: badge | type combo | bypass dot
            auto header = area.removeFromTop(24);
            header.removeFromLeft(18);                                      // badge
            const int bypassW = 16;
            bypassButton.setBounds(header.removeFromRight(bypassW)
                                         .withSizeKeepingCentre(12, 12));
            header.removeFromRight(spaceXs);
            typePicker.setBounds(header.reduced(0, 0));

            area.removeFromTop(spaceSm);

            // Mix on its own row
            mixKnob.setBounds(area.removeFromTop(48));
            area.removeFromTop(spaceXs);

            // 4 p-knobs in a 2x2 grid
            const int half = area.getHeight() / 2;
            auto rowTop    = area.removeFromTop(half);
            auto rowBottom = area;
            const int kw = rowTop.getWidth() / 2;
            p1Knob.setBounds(rowTop   .removeFromLeft(kw));
            p2Knob.setBounds(rowTop);
            p3Knob.setBounds(rowBottom.removeFromLeft(kw));
            p4Knob.setBounds(rowBottom);
        }

    private:
        void applyTypeLabels(int typeIdx)
        {
            const auto& labels = kFxParamLabels[(size_t) typeIdx];
            auto applyOne = [](ArcKnob& k, const char* lbl)
            {
                const juce::String s(lbl);
                k.setLabelText(s);
                k.setVisible(s.isNotEmpty());
            };
            applyOne(p1Knob, labels[0]);
            applyOne(p2Knob, labels[1]);
            applyOne(p3Knob, labels[2]);
            applyOne(p4Knob, labels[3]);
            // Mix is always visible; type=None just makes the slot a no-op,
            // but the user can still pre-set the mix for later activation.
        }

        int slotIdx;
        FxSlotTypePicker typePicker;
        juce::ToggleButton bypassButton;
        ArcKnob mixKnob, p1Knob, p2Knob, p3Knob, p4Knob;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttach;
        std::unique_ptr<juce::ParameterAttachment> typeAttach;
    };
}
