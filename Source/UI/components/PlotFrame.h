#pragma once
#include <juce_graphics/juce_graphics.h>
#include "../DesignTokens.h"

namespace SynthVibe
{
    // Draws the bordered rounded inset-gradient container used behind every
    // scope/response/envelope plot in the hi-fi mock. Returns the inner rect
    // the caller should render its curve into so the curve never clips the
    // border.
    inline juce::Rectangle<float> drawPlotFrame(juce::Graphics& g,
                                                juce::Rectangle<float> bounds)
    {
        using namespace Tokens;
        juce::ColourGradient grad(juce::Colours::black.withAlpha(0.40f),
                                  bounds.getX(), bounds.getY(),
                                  juce::Colours::black.withAlpha(0.15f),
                                  bounds.getX(), bounds.getBottom(),
                                  false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(bounds, radiusMd);
        g.setColour(edge);
        g.drawRoundedRectangle(bounds, radiusMd, 1.f);
        return bounds.reduced(4.f);
    }
}
