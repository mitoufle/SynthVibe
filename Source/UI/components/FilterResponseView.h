#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"
#include "../../Engine/FilterCoefficients.h"

namespace SynthVibe
{
    class FilterResponseView : public juce::Component, private juce::Timer
    {
    public:
        FilterResponseView(juce::AudioProcessorValueTreeState& apvts,
                           const juce::String& typeParam,
                           const juce::String& cutoffParam,
                           const juce::String& resParam,
                           juce::Colour lineColour = Tokens::filter)
            : apvtsRef(apvts),
              typeID(typeParam), cutoffID(cutoffParam), resID(resParam),
              colour(lineColour)
        {
            startTimerHz(30);
        }

        void paint(juce::Graphics& g) override
        {
            using Type = FilterCoefficients::Type;
            const int typeIdx = (int) std::round(apvtsRef.getRawParameterValue(typeID)->load());
            const float cutoff = apvtsRef.getRawParameterValue(cutoffID)->load();
            const float resN   = apvtsRef.getRawParameterValue(resID)->load();
            const float q      = juce::jmap(juce::jlimit(0.f, 1.f, resN), 0.5f, 12.f);

            const Type t = typeIdx == 0 ? Type::LowPass      // LP12
                         : typeIdx == 1 ? Type::LP24
                         : typeIdx == 2 ? Type::HighPass
                         : typeIdx == 3 ? Type::BandPass
                         :                Type::Notch;
            const double sr = 48000.0;
            const auto coeffs = FilterCoefficients::compute(t, cutoff, q, sr);

            auto b = getLocalBounds().toFloat().reduced(4.f);
            const int N = 120;
            const float fLow  = 20.f;
            const float fHigh = 20000.f;

            juce::Path path;
            for (int i = 0; i <= N; ++i)
            {
                const float u = (float) i / N;
                const float freq = fLow * std::pow(fHigh / fLow, u);
                float mag  = coeffs.magnitudeAt(freq, sr);
                if (t == Type::LP24) mag = mag * mag;   // cascade = |H|²
                const float db   = juce::Decibels::gainToDecibels(mag + 1e-6f);
                const float y    = juce::jmap(juce::jlimit(-36.f, 12.f, db),
                                              -36.f, 12.f, b.getBottom(), b.getY());
                const float x    = b.getX() + u * b.getWidth();
                if (i == 0) path.startNewSubPath(x, y);
                else        path.lineTo(x, y);
            }

            g.setColour(colour.withAlpha(0.25f));
            g.strokePath(path, juce::PathStrokeType(3.f));
            g.setColour(colour);
            g.strokePath(path, juce::PathStrokeType(1.5f));
        }

    private:
        void timerCallback() override { repaint(); }

        juce::AudioProcessorValueTreeState& apvtsRef;
        juce::String typeID, cutoffID, resID;
        juce::Colour colour;
    };
}
