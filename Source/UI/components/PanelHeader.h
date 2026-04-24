#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class PanelHeader : public juce::Component
    {
    public:
        PanelHeader(const juce::String& title, juce::Colour dot)
            : titleText(title.toUpperCase()), dotColour(dot) {}

        void paint(juce::Graphics& g) override
        {
            using namespace Tokens;
            auto b = getLocalBounds().toFloat();

            const float dotR = 3.f;
            const float dotX = b.getX() + spaceSm + dotR;
            const float dotY = b.getCentreY();
            g.setColour(dotColour);
            g.fillEllipse(dotX - dotR, dotY - dotR, dotR * 2.f, dotR * 2.f);

            // Mock uses the brightest ink for the title and letter-spacing 0.22em,
            // so the mono panel title reads as the section's primary label.
            g.setColour(ink);
            g.setFont(Fonts::mono(Font::panelTitle));
            g.drawText(titleText,
                       (int) (dotX + dotR + spaceSm),
                       (int) b.getY(),
                       (int) (b.getWidth() - (dotX + dotR + spaceSm)),
                       (int) b.getHeight(),
                       juce::Justification::centredLeft);
        }

    private:
        juce::String  titleText;
        juce::Colour  dotColour;
    };
}
