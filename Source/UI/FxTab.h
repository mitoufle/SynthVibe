#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "KnobWithLabel.h"
#include "LookAndFeel.h"
#include "../Parameters/ParameterIDs.h"

class FxTab : public juce::Component
{
public:
    explicit FxTab(juce::AudioProcessorValueTreeState& apvts) : apvts(apvts)
    {
        addAndMakeVisible(knobDelayTime);
        addAndMakeVisible(knobDelayFeedback);
        addAndMakeVisible(knobDelayMix);
        addAndMakeVisible(knobChorusRate);
        addAndMakeVisible(knobChorusDepth);
        addAndMakeVisible(knobChorusMix);
    }

    void paint(juce::Graphics& g) override
    {
        drawPanel(g, delayBounds,  "DELAY",  SynthLookAndFeel::colFxAccent);
        drawPanel(g, chorusBounds, "CHORUS", SynthLookAndFeel::colFxAccent);
    }

    void resized() override
    {
        const int pad    = 8;
        const int titleH = 20;
        auto area = getLocalBounds().reduced(pad);
        delayBounds  = area.removeFromLeft(area.getWidth() / 2).reduced(pad, 0);
        chorusBounds = area.reduced(pad, 0);

        auto layoutFx = [&](juce::Rectangle<int> bounds,
                            KnobWithLabel& k1, KnobWithLabel& k2, KnobWithLabel& k3)
        {
            auto b = bounds.withTrimmedTop(titleH);
            const int w = b.getWidth() / 3;
            k1.setBounds(b.removeFromLeft(w));
            k2.setBounds(b.removeFromLeft(w));
            k3.setBounds(b);
        };

        layoutFx(delayBounds,  knobDelayTime,   knobDelayFeedback, knobDelayMix);
        layoutFx(chorusBounds, knobChorusRate,  knobChorusDepth,   knobChorusMix);
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::Rectangle<int> delayBounds, chorusBounds;

    KnobWithLabel knobDelayTime     { "Time",     apvts, ParamIDs::delayTime,     " ms", 0 };
    KnobWithLabel knobDelayFeedback { "Feedback", apvts, ParamIDs::delayFeedback, "",    2 };
    KnobWithLabel knobDelayMix      { "Mix",      apvts, ParamIDs::delayMix,      "",    2 };
    KnobWithLabel knobChorusRate    { "Rate",     apvts, ParamIDs::chorusRate,    " Hz", 2 };
    KnobWithLabel knobChorusDepth   { "Depth",    apvts, ParamIDs::chorusDepth,   "",    3 };
    KnobWithLabel knobChorusMix     { "Mix",      apvts, ParamIDs::chorusMix,     "",    2 };

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
