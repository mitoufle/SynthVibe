#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class MasterVolKnob : public juce::Component
    {
    public:
        MasterVolKnob(juce::AudioProcessorValueTreeState& apvts,
                      const juce::String& paramID)
        {
            label.setText("VOL", juce::dontSendNotification);
            label.setFont(Fonts::mono(Tokens::Font::label));
            label.setColour(juce::Label::textColourId, Tokens::ink4);
            label.setJustificationType(juce::Justification::centredRight);
            addAndMakeVisible(label);

            slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
            slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            slider.setColour(juce::Slider::rotarySliderFillColourId, Tokens::accent);
            addAndMakeVisible(slider);

            attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, paramID, slider);
        }

        void resized() override
        {
            auto b = getLocalBounds();
            auto knobBox = b.removeFromRight(Tokens::knobSizeCompact - 12);
            slider.setBounds(knobBox);
            label.setBounds(b);
        }

    private:
        juce::Label  label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
}
