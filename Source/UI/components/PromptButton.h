#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class PromptButton : public juce::Button
    {
    public:
        PromptButton() : juce::Button("prompt") {}

        void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override
        {
            auto b = getLocalBounds().toFloat().reduced(1.f);
            auto fill = isButtonDown ? Tokens::accentHi.darker(0.1f)
                      : isMouseOver  ? Tokens::accentHi
                      :                Tokens::accent;
            g.setColour(fill);
            g.fillRoundedRectangle(b, Tokens::radiusSm);

            g.setColour(juce::Colour(0xFF1A1A1A));
            g.setFont(Fonts::mono(Tokens::Font::label).withStyle(juce::Font::bold));
            const juce::String text = juce::String(juce::CharPointer_UTF8("\xE2\x9C\xA6")) + "  PROMPT";
            g.drawText(text, getLocalBounds(), juce::Justification::centred);
        }
    };
}
