#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class BrandBadge : public juce::Component
    {
    public:
        explicit BrandBadge(const juce::String& versionTag = "V1")
            : version(versionTag) {}

        void paint(juce::Graphics& g) override
        {
            auto b = getLocalBounds();
            const int dotSize = 18;
            auto dot = b.removeFromLeft(dotSize).withSizeKeepingCentre(dotSize, dotSize);

            // Conic-gradient-like look: two-tone filled disc.
            juce::ColourGradient grad(Tokens::accent,   dot.getX() + dotSize * 0.3f, dot.getY(),
                                      Tokens::accent2,  dot.getRight(),              dot.getBottom(),
                                      false);
            g.setGradientFill(grad);
            g.fillEllipse(dot.toFloat());
            g.setColour(Tokens::panel);
            g.fillEllipse(dot.toFloat().reduced(3.f));

            b.removeFromLeft(Tokens::spaceSm);

            auto wordmark = b.removeFromLeft(120);
            g.setColour(Tokens::ink);
            g.setFont(Fonts::serif(20.f, juce::Font::plain));
            g.drawText("AI", wordmark.removeFromLeft(22), juce::Justification::centredLeft);
            g.setColour(Tokens::accent);
            g.setFont(Fonts::serif(20.f, juce::Font::italic));
            g.drawText("Synth", wordmark, juce::Justification::centredLeft);

            g.setColour(Tokens::ink3);
            g.setFont(Fonts::mono(Tokens::Font::label));
            g.drawText(version, b, juce::Justification::centredLeft);
        }

    private:
        juce::String version;
    };
}
