#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"

namespace SynthVibe
{
    // Styled ON pill toggle bound to a bool APVTS param. Outline when off,
    // accent-fill when on. Wraps a juce::TextButton + ButtonAttachment.
    class ArpOnOffPill : public juce::Component
    {
    public:
        ArpOnOffPill(juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& boolParamID)
        {
            button.setButtonText("ON");
            button.setClickingTogglesState(true);
            button.setColour(juce::TextButton::buttonColourId,   Tokens::panel2);
            button.setColour(juce::TextButton::buttonOnColourId, Tokens::arp);
            button.setColour(juce::TextButton::textColourOffId,  Tokens::ink3);
            button.setColour(juce::TextButton::textColourOnId,   Tokens::ink);

            attach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                apvts, boolParamID, button);
            addAndMakeVisible(button);
        }

        juce::TextButton& getButton() noexcept { return button; }

        void resized() override { button.setBounds(getLocalBounds()); }

    private:
        juce::TextButton button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attach;
    };
}
