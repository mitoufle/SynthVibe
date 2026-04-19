#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "LookAndFeel.h"

struct KnobWithLabel : public juce::Component
{
    juce::Slider slider { juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Label  nameLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    KnobWithLabel(const juce::String& name,
                  juce::AudioProcessorValueTreeState& apvts,
                  const juce::String& paramID,
                  const juce::String& valueSuffix   = "",
                  int                 decimalPlaces = 2)
    {
        nameLabel.setText(name, juce::dontSendNotification);
        nameLabel.setJustificationType(juce::Justification::centred);
        nameLabel.setFont(juce::Font(11.f, juce::Font::bold));
        nameLabel.setColour(juce::Label::textColourId, juce::Colour(SynthLookAndFeel::colTextDim));
        addAndMakeVisible(nameLabel);
        slider.setTextValueSuffix(valueSuffix);
        slider.setNumDecimalPlacesToDisplay(decimalPlaces);
        slider.setTooltip("Double-click or Ctrl+click to enter a value");
        addAndMakeVisible(slider);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, paramID, slider);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        nameLabel.setBounds(b.removeFromTop(14));
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, b.getWidth(), 16);
        slider.setBounds(b);
    }
};
