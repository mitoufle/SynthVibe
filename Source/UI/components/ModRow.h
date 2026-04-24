#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "ModSourcePicker.h"
#include "ModDestPicker.h"
#include "BipolarAmountBar.h"
#include "CurveSelect.h"
#include "../DesignTokens.h"

namespace SynthVibe
{
    class ModRow : public juce::Component
    {
    public:
        ModRow(juce::AudioProcessorValueTreeState& apvts,
               const juce::String& srcId,
               const juce::String& dstId,
               const juce::String& amountId,
               const juce::String& curveId)
            : src(apvts, srcId),
              dst(apvts, dstId),
              amount(apvts, amountId),
              curve(apvts, curveId)
        {
            addAndMakeVisible(src);
            addAndMakeVisible(dst);
            addAndMakeVisible(amount);
            addAndMakeVisible(curve);
        }

        void paint(juce::Graphics& g) override
        {
            g.setColour(Tokens::edge.withAlpha(0.35f));
            g.drawHorizontalLine(getHeight() - 1, 0.f, (float) getWidth());
        }

        void resized() override
        {
            // Column layout: src 20% · dst 28% · amount 38% · curve 14%
            auto b = getLocalBounds().reduced(Tokens::spaceXs, Tokens::spaceXs / 2);
            const int wSrc    = (int) (b.getWidth() * 0.20f);
            const int wDst    = (int) (b.getWidth() * 0.28f);
            const int wCurve  = (int) (b.getWidth() * 0.14f);
            src   .setBounds(b.removeFromLeft(wSrc   ).reduced(2, 0));
            dst   .setBounds(b.removeFromLeft(wDst   ).reduced(2, 0));
            curve .setBounds(b.removeFromRight(wCurve).reduced(2, 0));
            amount.setBounds(b.reduced(Tokens::spaceSm, 4));
        }

    private:
        ModSourcePicker  src;
        ModDestPicker    dst;
        BipolarAmountBar amount;
        CurveSelect      curve;
    };
}
