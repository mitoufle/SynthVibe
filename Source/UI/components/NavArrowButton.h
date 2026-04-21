#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../DesignTokens.h"

namespace SynthVibe
{
    class NavArrowButton : public juce::Component
    {
    public:
        enum class Direction { Prev, Next };

        std::function<void()> onClick;

        explicit NavArrowButton(Direction dir)
        {
            const char* glyph = (dir == Direction::Prev)
                ? "\xE2\x97\x80"   // ◀
                : "\xE2\x96\xB6";  // ▶
            button.setButtonText(juce::String(juce::CharPointer_UTF8(glyph)));
            button.setColour(juce::TextButton::buttonColourId, Tokens::panel2);
            button.setColour(juce::TextButton::textColourOffId, Tokens::ink2);
            button.onClick = [this] { if (onClick) onClick(); };
            addAndMakeVisible(button);
        }

        // juce::Component::setEnabled is non-virtual, so we hook enablementChanged()
        // (which Component::setEnabled triggers) to keep the inner TextButton in sync.
        void enablementChanged() override
        {
            button.setEnabled(isEnabled());
        }

        void resized() override
        {
            button.setBounds(getLocalBounds());
        }

    private:
        juce::TextButton button;
    };
}
