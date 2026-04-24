#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    // The Sound / Mod / FX / Arp tab selector as the hi-fi mock draws it:
    // transparent background, ink-3 text when idle, ink-2 on hover, ink when
    // active with an accent-coloured underline under the active one. We use
    // a dedicated component rather than juce::TextButton + LookAndFeel so
    // that OS-level buttons in other parts of the plugin keep their own look.
    class MainTab : public juce::Component
    {
    public:
        std::function<void()> onClick;

        MainTab() = default;

        void setLabel(const juce::String& text) { label = text.toUpperCase(); repaint(); }
        void setActive(bool a)                  { active = a;                 repaint(); }

        void paint(juce::Graphics& g) override
        {
            using namespace Tokens;
            auto b = getLocalBounds().toFloat();

            g.setColour(active ? ink : (hovered ? ink2 : ink3));
            g.setFont(Fonts::sans(Font::body, juce::Font::plain));
            g.drawText(label, b, juce::Justification::centred);

            if (active)
            {
                const float w = b.getWidth() * 0.5f;
                const float x = b.getCentreX() - w * 0.5f;
                const float y = b.getBottom() - 2.f;
                g.setColour(accent);
                g.fillRoundedRectangle(x, y, w, 2.f, 1.f);
            }
        }

        void mouseDown(const juce::MouseEvent&) override { if (onClick) onClick(); }
        void mouseEnter(const juce::MouseEvent&) override { hovered = true; repaint(); }
        void mouseExit (const juce::MouseEvent&) override { hovered = false; repaint(); }

    private:
        juce::String label;
        bool         active  = false;
        bool         hovered = false;
    };
}
