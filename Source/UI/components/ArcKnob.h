#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class ArcKnob : public juce::Component
    {
    public:
        ArcKnob(const juce::String& labelText,
                juce::AudioProcessorValueTreeState& apvts,
                const juce::String& paramID,
                juce::Colour arcColour    = Tokens::accent,
                const juce::String& suffix = "",
                int decimalPlaces          = 2)
        {
            label.setText(labelText.toUpperCase(), juce::dontSendNotification);
            label.setJustificationType(juce::Justification::centredTop);
            label.setFont(Fonts::mono(Tokens::Font::label));
            label.setColour(juce::Label::textColourId, Tokens::ink3);
            addAndMakeVisible(label);

            slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
            slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 14);
            slider.setColour(juce::Slider::rotarySliderFillColourId, arcColour);
            slider.setColour(juce::Slider::textBoxTextColourId, Tokens::ink2);
            slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            slider.setTextValueSuffix(suffix);
            slider.setNumDecimalPlacesToDisplay(decimalPlaces);
            addAndMakeVisible(slider);

            attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, paramID, slider);
        }

        juce::Slider&       getSlider()       { return slider; }
        const juce::Slider& getSlider() const { return slider; }

        void resized() override
        {
            auto b = getLocalBounds();
            label.setBounds(b.removeFromTop(12));
            slider.setBounds(b);
        }

    private:
        juce::Label  label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
}
