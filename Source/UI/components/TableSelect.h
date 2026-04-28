#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"

namespace SynthVibe
{
    class TableSelect : public juce::Component
    {
    public:
        TableSelect(juce::AudioProcessorValueTreeState& apvts,
                    const juce::String& paramID)
        {
            combo.addItemList({ "Organ", "EP", "Bell", "Vocal", "Noise" }, 1);
            combo.setColour(juce::ComboBox::backgroundColourId, Tokens::panel2);
            combo.setColour(juce::ComboBox::textColourId,        Tokens::ink);
            combo.setColour(juce::ComboBox::outlineColourId,     Tokens::edge);
            combo.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(combo);

            attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                apvts, paramID, combo);
        }

        void resized() override { combo.setBounds(getLocalBounds()); }

        juce::ComboBox& getCombo() { return combo; }

    private:
        juce::ComboBox combo;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    };
}
