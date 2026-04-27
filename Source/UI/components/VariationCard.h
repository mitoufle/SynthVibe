#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../DesignTokens.h"
#include "../Fonts.h"
#include "../../AI/Variation.h"

namespace SynthVibe
{
    class VariationCard : public juce::Component
    {
    public:
        std::function<void(int)> onClicked;

        VariationCard() = default;

        void setVariation(const Variation& v)
        {
            variation = v;
            empty = false;
            repaint();
        }

        void setEmpty()
        {
            variation = {};
            empty = true;
            repaint();
        }

        void setSelected(bool s)
        {
            if (selected == s) return;
            selected = s;
            repaint();
        }

        void setIndex(int i) { index = i; }
        bool isEmpty()    const noexcept { return empty; }
        bool isSelected() const noexcept { return selected; }

        void paint(juce::Graphics& g) override
        {
            using namespace Tokens;
            auto b = getLocalBounds().toFloat().reduced(2.f);

            // Background
            g.setColour(panel2);
            g.fillRoundedRectangle(b, radiusMd);

            // Border (selected = accent, otherwise edge)
            g.setColour(selected ? accent : edge);
            g.drawRoundedRectangle(b, radiusMd, selected ? 2.0f : 1.0f);

            if (empty)
            {
                g.setColour(ink3);
                g.setFont(Fonts::mono(Font::body));
                g.drawText(juce::String(juce::CharPointer_UTF8("\xE2\x80\x94")),
                           getLocalBounds(), juce::Justification::centred);
                return;
            }

            auto inner = b.reduced(spaceSm);

            // Name
            g.setColour(ink);
            g.setFont(Fonts::sans(Font::body, juce::Font::bold));
            auto nameRow = inner.removeFromTop(18.f);
            g.drawText(variation.name, nameRow.toNearestInt(), juce::Justification::topLeft);

            // Description (single-line truncation handled by drawText)
            g.setColour(ink2);
            g.setFont(Fonts::sans(Font::label));
            auto descRow = inner.removeFromTop(14.f);
            g.drawText(variation.description, descRow.toNearestInt(),
                       juce::Justification::topLeft, true);

            // Tag badges (up to 3, drawn left-to-right at the bottom)
            inner.removeFromTop(spaceSm);
            auto tagRow = inner.removeFromBottom(16.f);
            const int maxTags = juce::jmin(3, variation.tags.size());
            g.setFont(Fonts::mono(Font::tiny));
            for (int i = 0; i < maxTags; ++i)
            {
                const auto tagText = variation.tags[i];
                const auto w = g.getCurrentFont().getStringWidthFloat(tagText) + 10.f;
                auto badge = tagRow.removeFromLeft(w);
                g.setColour(panel);
                g.fillRoundedRectangle(badge, 2.f);
                g.setColour(ink2);
                g.drawText(tagText, badge.toNearestInt(), juce::Justification::centred);
                tagRow.removeFromLeft((float) spaceXs);
            }
        }

        void mouseDown(const juce::MouseEvent&) override
        {
            triggerClick();
        }

        // Test seam — same effect as a mouseDown click.
        void triggerClick()
        {
            if (empty) return;
            if (onClicked) onClicked(index);
        }

        void resized() override {}

    private:
        Variation variation;
        bool empty    = true;
        bool selected = false;
        int  index    = 0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VariationCard)
    };
}
