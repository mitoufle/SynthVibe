#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "KnobWithLabel.h"
#include "LookAndFeel.h"
#include "../Parameters/ParameterIDs.h"

class ArpTab : public juce::Component
{
public:
    explicit ArpTab(juce::AudioProcessorValueTreeState& apvts) : apvts(apvts)
    {
        arpOnButton.setButtonText("ARP ON");
        arpOnButton.setClickingTogglesState(true);
        arpOnAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, ParamIDs::arpEnabled, arpOnButton);

        arpModeBox.addItemList({ "Up", "Down", "UpDown", "Random" }, 1);
        arpModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, ParamIDs::arpMode, arpModeBox);

        arpRateBox.addItemList({ "1/16", "1/8", "1/4", "1/2" }, 1);
        arpRateAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, ParamIDs::arpRate, arpRateBox);

        addAndMakeVisible(arpOnButton);
        addAndMakeVisible(arpModeBox);
        addAndMakeVisible(arpRateBox);
        addAndMakeVisible(knobArpOct);
    }

    void paint(juce::Graphics& g) override
    {
        drawPanel(g, panelBounds, "ARP", SynthLookAndFeel::colArpAccent);
    }

    void resized() override
    {
        const int pad    = 8;
        const int comboH = 28;
        const int btnH   = 32;

        auto area = getLocalBounds().reduced(pad);
        const int panelW = juce::jmin(area.getWidth(), 500);
        panelBounds = area.withSizeKeepingCentre(panelW, area.getHeight());

        auto inner = panelBounds.reduced(12, 28);
        arpOnButton.setBounds(inner.removeFromTop(btnH));
        inner.removeFromTop(6);
        arpModeBox.setBounds(inner.removeFromTop(comboH));
        inner.removeFromTop(6);
        arpRateBox.setBounds(inner.removeFromTop(comboH));
        inner.removeFromTop(6);
        knobArpOct.setBounds(inner.removeFromTop(80).withSizeKeepingCentre(80, 80));
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::Rectangle<int> panelBounds;

    juce::ToggleButton arpOnButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> arpOnAttach;

    juce::ComboBox arpModeBox, arpRateBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        arpModeAttach, arpRateAttach;

    KnobWithLabel knobArpOct { "Octaves", apvts, ParamIDs::arpOctaveRange, "", 0 };

    static void drawPanel(juce::Graphics& g, juce::Rectangle<int> bounds,
                          const juce::String& title, juce::uint32 accentColour)
    {
        auto f = bounds.toFloat().reduced(2.f);
        g.setColour(juce::Colour(SynthLookAndFeel::colPanel));
        g.fillRoundedRectangle(f, 6.f);
        g.setColour(juce::Colour(accentColour));
        g.drawRoundedRectangle(f, 6.f, 1.f);
        g.setColour(juce::Colour(SynthLookAndFeel::colHighlight));
        g.setFont(juce::Font(11.f, juce::Font::bold));
        g.drawText(title, bounds.getX() + 10, bounds.getY() + 4,
                   bounds.getWidth() - 20, 16, juce::Justification::centredLeft);
    }
};
