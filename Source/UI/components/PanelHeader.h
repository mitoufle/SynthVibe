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

            g.setColour(ink2);
            g.setFont(Fonts::mono(Font::panelTitle));
            g.drawText(titleText,
                       (int) (dotX + dotR + spaceSm),
                       (int) b.getY(),
                       (int) (b.getWidth() - (dotX + dotR + spaceSm)),
                       (int) b.getHeight(),
                       juce::Justification::centredLeft);

            g.setColour(edge);
            g.drawHorizontalLine((int) b.getBottom() - 1,
                                 b.getX(), b.getRight());
        }

    private:
        juce::String  titleText;
        juce::Colour  dotColour;
    };
}
