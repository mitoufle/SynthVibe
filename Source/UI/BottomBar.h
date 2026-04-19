#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "LookAndFeel.h"
#include <functional>

class BottomBar : public juce::Component,
                  private juce::Timer
{
public:
    explicit BottomBar(std::function<int()> voiceCountGetter)
        : getVoiceCount(std::move(voiceCountGetter))
    {
        voiceLabel.setColour(juce::Label::textColourId,
                             juce::Colour(SynthLookAndFeel::colTextDim));
        voiceLabel.setFont(juce::Font(10.f));
        addAndMakeVisible(voiceLabel);

        cpuLabel.setColour(juce::Label::textColourId,
                           juce::Colour(SynthLookAndFeel::colTextDim));
        cpuLabel.setFont(juce::Font(10.f));
        cpuLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(cpuLabel);

        startTimerHz(10);
        updateLabels();
    }

    ~BottomBar() override { stopTimer(); }

    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colour(SynthLookAndFeel::colAccent));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(8, 2);
        voiceLabel.setBounds(area.removeFromLeft(area.getWidth() / 2));
        cpuLabel.setBounds(area);
    }

private:
    std::function<int()> getVoiceCount;
    juce::Label voiceLabel, cpuLabel;

    void timerCallback() override { updateLabels(); }

    void updateLabels()
    {
        const int active = getVoiceCount ? getVoiceCount() : 0;
        juce::String bar;
        for (int i = 0; i < 8; ++i)
            bar += (i < active ? juce::String(L"\u2588") : juce::String(L"\u2591"));
        voiceLabel.setText("Voices: " + bar + "  " + juce::String(active) + "/8",
                           juce::dontSendNotification);
        cpuLabel.setText("CPU: --", juce::dontSendNotification);
    }
};
