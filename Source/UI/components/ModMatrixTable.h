#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "ModRow.h"
#include "../DesignTokens.h"
#include "../Fonts.h"
#include "../../Parameters/ParameterIDs.h"

namespace SynthVibe
{
    class ModMatrixTable : public juce::Component
    {
    public:
        static constexpr int kVisibleRows = 8;

        explicit ModMatrixTable(juce::AudioProcessorValueTreeState& apvts)
        {
            struct SlotIds { const char* src; const char* dst; const char* amount; const char* curve; };
            const SlotIds slots[kVisibleRows] = {
                { ParamIDs::mod1Src, ParamIDs::mod1Dst, ParamIDs::mod1Amount, ParamIDs::mod1Curve },
                { ParamIDs::mod2Src, ParamIDs::mod2Dst, ParamIDs::mod2Amount, ParamIDs::mod2Curve },
                { ParamIDs::mod3Src, ParamIDs::mod3Dst, ParamIDs::mod3Amount, ParamIDs::mod3Curve },
                { ParamIDs::mod4Src, ParamIDs::mod4Dst, ParamIDs::mod4Amount, ParamIDs::mod4Curve },
                { ParamIDs::mod5Src, ParamIDs::mod5Dst, ParamIDs::mod5Amount, ParamIDs::mod5Curve },
                { ParamIDs::mod6Src, ParamIDs::mod6Dst, ParamIDs::mod6Amount, ParamIDs::mod6Curve },
                { ParamIDs::mod7Src, ParamIDs::mod7Dst, ParamIDs::mod7Amount, ParamIDs::mod7Curve },
                { ParamIDs::mod8Src, ParamIDs::mod8Dst, ParamIDs::mod8Amount, ParamIDs::mod8Curve },
            };
            for (int i = 0; i < kVisibleRows; ++i)
            {
                auto& slot = slots[i];
                rows[i] = std::make_unique<ModRow>(apvts,
                    slot.src, slot.dst, slot.amount, slot.curve);
                addAndMakeVisible(*rows[i]);
            }
        }

        int getNumRows() const noexcept { return kVisibleRows; }

        void paint(juce::Graphics& g) override
        {
            auto b = getLocalBounds();
            g.setColour(Tokens::panel2);
            g.fillRoundedRectangle(b.toFloat(), Tokens::radiusLg);
            g.setColour(Tokens::edge);
            g.drawRoundedRectangle(b.toFloat().reduced(0.5f), Tokens::radiusLg, 1.f);

            // Header strip
            auto header = b.removeFromTop(kHeaderH);
            g.setColour(Tokens::ink3);
            g.setFont(juce::Font(Tokens::Font::label, juce::Font::bold));
            drawColumn(g, header, 0.00f, 0.20f, "SRC");
            drawColumn(g, header, 0.20f, 0.48f, "DST");
            drawColumn(g, header, 0.48f, 0.86f, "AMOUNT");
            drawColumn(g, header, 0.86f, 1.00f, "CURVE");

            g.setColour(Tokens::edge);
            g.drawHorizontalLine(kHeaderH, 0.f, (float) getWidth());
        }

        void resized() override
        {
            auto b = getLocalBounds();
            b.removeFromTop(kHeaderH);
            const int rowH = b.getHeight() / kVisibleRows;
            for (int i = 0; i < kVisibleRows; ++i)
                rows[i]->setBounds(b.removeFromTop(rowH));
        }

    private:
        static constexpr int kHeaderH = 20;

        static void drawColumn(juce::Graphics& g, juce::Rectangle<int> header,
                               float xFrac0, float xFrac1, const juce::String& label)
        {
            const int x0 = (int) (header.getWidth() * xFrac0);
            const int x1 = (int) (header.getWidth() * xFrac1);
            auto cell = juce::Rectangle<int>(x0, header.getY(), x1 - x0, header.getHeight())
                            .reduced(Tokens::spaceSm, 0);
            g.drawText(label, cell, juce::Justification::centredLeft);
        }

        std::unique_ptr<ModRow> rows[kVisibleRows];
    };
}
