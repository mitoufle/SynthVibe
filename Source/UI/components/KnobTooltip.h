#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class KnobTooltip : public juce::Component
    {
    public:
        KnobTooltip()
        {
            setInterceptsMouseClicks(false, false);
            setVisible(false);
        }

        void showAt(juce::Point<int> anchor, const juce::String& text)
        {
            readout = text;
            const int w = juce::jmax(48,
                Fonts::mono(Tokens::Font::value).getStringWidth(text) + Tokens::spaceMd);
            setBounds(anchor.x - w / 2,
                      anchor.y - 22,
                      w, 18);
            setVisible(true);
            toFront(false);
            repaint();
        }

        void hide() { setVisible(false); }

        void paint(juce::Graphics& g) override
        {
            using namespace Tokens;
            auto b = getLocalBounds().toFloat();
            g.setColour(panel2.withAlpha(0.95f));
            g.fillRoundedRectangle(b, radiusSm);
            g.setColour(edge);
            g.drawRoundedRectangle(b, radiusSm, 1.f);
            g.setColour(ink);
            g.setFont(Fonts::mono(Font::value));
            g.drawText(readout, getLocalBounds(), juce::Justification::centred);
        }

    private:
        juce::String readout;
    };
}
