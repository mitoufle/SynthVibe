#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"

namespace SynthVibe
{
    // Horizontal -1..+1 drag bar. Click-drag anywhere inside sets the value
    // proportional to x / width (0 is dead-centre). Double-click resets to 0.
    // Positive fills with Tokens::modPositive (violet); negative fills with
    // Tokens::modNegative (pink).
    class BipolarAmountBar : public juce::Component
    {
    public:
        BipolarAmountBar(juce::AudioProcessorValueTreeState& apvts,
                         const juce::String& paramID)
            : parameter(apvts.getParameter(paramID))
        {
            jassert(parameter != nullptr);
            jassert(parameter->getNormalisableRange().start <= -1.0f
                 && parameter->getNormalisableRange().end   >=  1.0f);
            attach = std::make_unique<juce::ParameterAttachment>(
                *parameter,
                [this](float v) {
                    value = v;
                    repaint();
                });
            attach->sendInitialUpdate();
        }

        float getValue() const noexcept { return value; }

        void paint(juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat();
            g.setColour(Tokens::panel2);
            g.fillRoundedRectangle(b, Tokens::radiusSm);
            g.setColour(Tokens::edge);
            g.drawRoundedRectangle(b.reduced(0.5f), Tokens::radiusSm, 1.f);

            // Center tick
            const float cx = b.getCentreX();
            g.setColour(Tokens::ink4);
            g.drawLine(cx, b.getY() + 3.f, cx, b.getBottom() - 3.f, 1.f);

            // Fill from centre toward current value
            const float halfW = b.getWidth() * 0.5f;
            const float filled = juce::jlimit(-1.f, 1.f, value) * halfW;
            const auto colour = value >= 0.f ? Tokens::modPositive : Tokens::modNegative;
            g.setColour(colour);
            juce::Rectangle<float> fill;
            if (value >= 0.f)
                fill = { cx, b.getY() + 2.f, filled, b.getHeight() - 4.f };
            else
                fill = { cx + filled, b.getY() + 2.f, -filled, b.getHeight() - 4.f };
            g.fillRoundedRectangle(fill, Tokens::radiusSm * 0.5f);
        }

        void mouseDown(const juce::MouseEvent& e) override
        {
            if (attach != nullptr) attach->beginGesture();
            updateFromMouse(e);
        }
        void mouseDrag(const juce::MouseEvent& e) override { updateFromMouse(e); }
        void mouseUp(const juce::MouseEvent&) override
        {
            if (attach != nullptr) attach->endGesture();
        }

        void mouseDoubleClick(const juce::MouseEvent&) override
        {
            if (attach == nullptr) return;
            attach->beginGesture();
            attach->setValueAsPartOfGesture(0.f);
            attach->endGesture();
        }

    private:
        void updateFromMouse(const juce::MouseEvent& e)
        {
            if (attach == nullptr) return;
            const float w = (float) getWidth();
            if (w <= 0.f) return;
            const float norm = juce::jlimit(0.f, 1.f, e.position.x / w);
            const float v    = norm * 2.f - 1.f;  // [0,1] → [-1,+1]
            attach->setValueAsPartOfGesture(v);
        }

        juce::RangedAudioParameter* parameter = nullptr;
        std::unique_ptr<juce::ParameterAttachment> attach;
        float value = 0.f;
    };
}
