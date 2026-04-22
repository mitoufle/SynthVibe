#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class WaveTypeSelect : public juce::Component
    {
    public:
        WaveTypeSelect(juce::AudioProcessorValueTreeState& apvts,
                       const juce::String& paramID)
        {
            combo.addItemList({ "Sine", "Saw", "Square", "Triangle" }, 1);
            combo.setColour(juce::ComboBox::backgroundColourId, Tokens::panel2);
            combo.setColour(juce::ComboBox::textColourId, Tokens::ink);
            combo.setColour(juce::ComboBox::outlineColourId, Tokens::edge);
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
