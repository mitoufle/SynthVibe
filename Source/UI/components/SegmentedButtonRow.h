#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"

namespace SynthVibe
{
    // Generic N-segment pill row bound to a juce Choice param. Each segment
    // is a flat-styled button; the active segment is filled with `accent`,
    // others outline-only. Click commits selection through a hidden ComboBox
    // that owns the APVTS attachment — reuses JUCE's ComboBoxAttachment
    // plumbing without inventing a new ChoiceAttachment type.
    class SegmentedButtonRow : public juce::Component,
                               public juce::ComboBox::Listener
    {
    public:
        SegmentedButtonRow(juce::AudioProcessorValueTreeState& apvts,
                           const juce::String& choiceParamID,
                           const juce::StringArray& labels,
                           juce::Colour accent)
            : accentColour(accent)
        {
            buttons.reserve((size_t) labels.size());
            for (int i = 0; i < labels.size(); ++i)
            {
                hiddenCombo.addItem(labels[i], i + 1);
                buttons.emplace_back(std::make_unique<juce::TextButton>(labels[i]));
                auto* btn = buttons.back().get();
                btn->setClickingTogglesState(false);
                btn->onClick = [this, i] {
                    hiddenCombo.setSelectedId(i + 1, juce::sendNotificationSync);
                };
                addAndMakeVisible(*btn);
            }

            attach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                apvts, choiceParamID, hiddenCombo);
            hiddenCombo.addListener(this);
            comboBoxChanged(&hiddenCombo);   // sync visual to current parameter state
        }

        ~SegmentedButtonRow() override { hiddenCombo.removeListener(this); }

        int getNumSegments() const noexcept { return (int) buttons.size(); }
        juce::TextButton& getSegmentButton(int i) noexcept { return *buttons[(size_t) i]; }

        void resized() override
        {
            const int n = (int) buttons.size();
            if (n <= 0) return;
            const int w = getWidth() / n;
            for (int i = 0; i < n; ++i)
            {
                const int x  = i * w;
                const int wx = (i == n - 1) ? (getWidth() - x) : w;
                buttons[(size_t) i]->setBounds(x, 0, wx, getHeight());
            }
        }

        void comboBoxChanged(juce::ComboBox* cb) override
        {
            const int sel = cb->getSelectedId(); // 1-based
            for (int i = 0; i < (int) buttons.size(); ++i)
            {
                const bool active = (i + 1) == sel;
                buttons[(size_t) i]->setColour(juce::TextButton::buttonColourId,
                    active ? accentColour : Tokens::panel2);
                buttons[(size_t) i]->setColour(juce::TextButton::textColourOffId, Tokens::ink3);
                buttons[(size_t) i]->setColour(juce::TextButton::textColourOnId,  Tokens::ink);
                buttons[(size_t) i]->setToggleState(active, juce::dontSendNotification);
                buttons[(size_t) i]->repaint();
            }
        }

    private:
        juce::ComboBox hiddenCombo;
        std::vector<std::unique_ptr<juce::TextButton>> buttons;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attach;
        juce::Colour accentColour;
    };
}
