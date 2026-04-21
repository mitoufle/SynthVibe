#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class PresetNameField : public juce::Component
    {
    public:
        void setPresetName (const juce::String& n) { name = n; repaint(); }
        void setMetaText   (const juce::String& m) { meta = m; repaint(); }
        void setDirty      (bool d)                { dirty = d; repaint(); }

        void paint(juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat();
            g.setColour(juce::Colour(0x33000000));
            g.fillRoundedRectangle(b, Tokens::radiusLg);
            g.setColour(Tokens::edge);
            g.drawRoundedRectangle(b, Tokens::radiusLg, 1.f);

            auto inner = getLocalBounds().reduced(14, 4);
            auto labelRow = inner.removeFromTop(12);
            g.setColour(Tokens::ink4);
            g.setFont(Fonts::mono(Tokens::Font::label));
            g.drawText("PRESET", labelRow, juce::Justification::centred);

            auto titleRow = inner.removeFromTop(20);
            g.setColour(Tokens::ink);
            g.setFont(Fonts::serif(Tokens::Font::presetTitle, juce::Font::italic));
            const auto display = dirty ? name + " " + juce::String(juce::CharPointer_UTF8("\xE2\x97\x8F"))
                                       : name;
            g.drawText(display.isEmpty() ? "Init" : display, titleRow, juce::Justification::centred);

            g.setColour(Tokens::ink3);
            g.setFont(Fonts::mono(Tokens::Font::tiny));
            g.drawText(meta, inner, juce::Justification::centred);
        }

    private:
        juce::String name, meta;
        bool dirty { false };
    };
}
