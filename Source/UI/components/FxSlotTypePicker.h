#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"
#include "../../FX/FxTypeMap.h"

namespace SynthVibe
{
    class FxSlotTypePicker : public juce::Component
    {
    public:
        FxSlotTypePicker(juce::AudioProcessorValueTreeState& apvts,
                         const juce::String& paramID)
        {
            for (int i = 0; i < kFxTypeCount; ++i)
                combo.addItem(kFxTypeNames[(size_t) i], i + 1);

            attach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                apvts, paramID, combo);

            combo.setColour(juce::ComboBox::backgroundColourId, Tokens::panel2);
            combo.setColour(juce::ComboBox::outlineColourId,    Tokens::edge);
            combo.setColour(juce::ComboBox::textColourId,       Tokens::ink);
            combo.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(combo);
        }

        juce::ComboBox& getCombo() noexcept { return combo; }

        void resized() override { combo.setBounds(getLocalBounds()); }

    private:
        juce::ComboBox combo;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attach;
    };
}
