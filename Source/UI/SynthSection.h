#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "LookAndFeel.h"

// Base class for all UI panels. Draws a labelled rounded-rect frame.
// Add new sections by subclassing this and overriding resized().
class SynthSection : public juce::Component
{
public:
    explicit SynthSection(const juce::String& title) : sectionTitle(title) {}

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.f);

        g.setColour(juce::Colour(SynthLookAndFeel::colPanel));
        g.fillRoundedRectangle(bounds, 6.f);

        g.setColour(juce::Colour(SynthLookAndFeel::colAccent));
        g.drawRoundedRectangle(bounds, 6.f, 1.f);

        g.setColour(juce::Colour(SynthLookAndFeel::colHighlight));
        g.setFont(juce::Font(11.f, juce::Font::bold));
        g.drawText(sectionTitle.toUpperCase(), 10, 4, getWidth() - 20, 16,
                   juce::Justification::centredLeft);
    }

protected:
    // Content area below the title bar
    juce::Rectangle<int> contentBounds() const
    {
        return getLocalBounds().withTrimmedTop(22).reduced(6, 4);
    }

private:
    juce::String sectionTitle;
};
