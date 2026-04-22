#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"

namespace SynthVibe
{
    class OscilloscopeView : public juce::Component, private juce::Timer
    {
    public:
        OscilloscopeView(juce::AudioProcessorValueTreeState& apvts,
                         const juce::String& waveformParamID,
                         juce::Colour lineColour = Tokens::osc)
            : param(apvts.getParameter(waveformParamID)),
              colour(lineColour)
        {
            waveAttach = std::make_unique<juce::ParameterAttachment>(
                *param, [this](float v) {
                    waveIndex = juce::jlimit(0, 3, (int) std::round(v));
                    repaint();
                });
            waveAttach->sendInitialUpdate();
            startTimerHz(30);
        }

        void paint(juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat().reduced(2.f);
            const float midY = b.getCentreY();
            const float amp  = b.getHeight() * 0.35f;

            juce::Path path;
            const int N = 128;
            for (int i = 0; i <= N; ++i)
            {
                const float phase = (float) i / N;
                const float t = std::fmod(phase + drift, 1.f);
                const float y = midY - amp * sample(t);
                const float x = b.getX() + phase * b.getWidth();
                if (i == 0) path.startNewSubPath(x, y);
                else        path.lineTo(x, y);
            }

            g.setColour(colour.withAlpha(0.25f));
            g.strokePath(path, juce::PathStrokeType(3.f));
            g.setColour(colour);
            g.strokePath(path, juce::PathStrokeType(1.5f));
        }

    private:
        void timerCallback() override
        {
            drift += 1.f / (float) (Tokens::Motion::ambientScopeMs / 33);
            if (drift > 1.f) drift -= 1.f;
            repaint();
        }

        float sample(float t) const
        {
            switch (waveIndex)
            {
                case 0: return std::sin(2.f * 3.14159265358979323846f * t);     // Sine
                case 1: return 2.f * t - 1.f;                                    // Saw
                case 2: return t < 0.5f ? 1.f : -1.f;                            // Square
                case 3: return 4.f * std::abs(t - 0.5f) - 1.f;                   // Triangle
            }
            return 0.f;
        }

        juce::RangedAudioParameter* param;
        juce::Colour  colour;
        int           waveIndex = 0;
        float         drift     = 0.f;
        std::unique_ptr<juce::ParameterAttachment> waveAttach;
    };
}
