#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "KnobWithLabel.h"
#include "LookAndFeel.h"
#include "DesignTokens.h"
#include "components/ModMatrixTable.h"
#include "../Parameters/ParameterIDs.h"

class ModTab : public juce::Component
{
public:
    explicit ModTab(juce::AudioProcessorValueTreeState& apvts) : apvts(apvts)
    {
        lfo1ShapeBox.addItemList({ "Sine", "Saw", "Square", "Triangle" }, 1);
        lfo1ShapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, ParamIDs::lfo1Shape, lfo1ShapeBox);
        lfo1DestBox.addItemList({ "Pitch", "Filter", "Amp", "Detune" }, 1);
        lfo1DestAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, ParamIDs::lfo1Dest, lfo1DestBox);

        lfo2ShapeBox.addItemList({ "Sine", "Saw", "Square", "Triangle" }, 1);
        lfo2ShapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, ParamIDs::lfo2Shape, lfo2ShapeBox);
        lfo2DestBox.addItemList({ "Pitch", "Filter", "Amp", "Detune" }, 1);
        lfo2DestAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, ParamIDs::lfo2Dest, lfo2DestBox);

        for (auto* c : { &lfo1ShapeBox, &lfo1DestBox, &lfo2ShapeBox, &lfo2DestBox })
            addAndMakeVisible(c);
        addAndMakeVisible(knobLfo1Rate);
        addAndMakeVisible(knobLfo1Depth);
        addAndMakeVisible(knobLfo2Rate);
        addAndMakeVisible(knobLfo2Depth);

        matrix = std::make_unique<SynthVibe::ModMatrixTable>(apvts);
        addAndMakeVisible(*matrix);
    }

    SynthVibe::ModMatrixTable* getMatrix() noexcept { return matrix.get(); }

    void paint(juce::Graphics& g) override
    {
        drawPanel(g, lfo1Bounds, "LFO 1", SynthLookAndFeel::colLfoAccent);
        drawPanel(g, lfo2Bounds, "LFO 2", SynthLookAndFeel::colLfoAccent);
    }

    void resized() override
    {
        using namespace SynthVibe::Tokens;
        auto area = getLocalBounds().reduced(spaceMd);

        // Top 35%: two LFO panels side-by-side. Bottom 65%: mod matrix.
        auto top = area.removeFromTop(juce::roundToInt(area.getHeight() * 0.35f));
        area.removeFromTop(spaceMd);   // gap between the two regions

        lfo1Bounds = top.removeFromLeft(top.getWidth() / 2).reduced(spaceSm, 0);
        lfo2Bounds = top.reduced(spaceSm, 0);

        const int comboH = 26;
        const int titleH = 20;
        auto layoutLfo = [&](juce::Rectangle<int> bounds,
                             juce::ComboBox& shapeBox, juce::ComboBox& destBox,
                             KnobWithLabel& rate, KnobWithLabel& depth)
        {
            auto b = bounds.withTrimmedTop(titleH);
            shapeBox.setBounds(b.removeFromTop(comboH));
            destBox .setBounds(b.removeFromTop(comboH));
            const int knobW = b.getWidth() / 2;
            rate .setBounds(b.removeFromLeft(knobW));
            depth.setBounds(b);
        };

        layoutLfo(lfo1Bounds, lfo1ShapeBox, lfo1DestBox, knobLfo1Rate, knobLfo1Depth);
        layoutLfo(lfo2Bounds, lfo2ShapeBox, lfo2DestBox, knobLfo2Rate, knobLfo2Depth);

        if (matrix != nullptr)
            matrix->setBounds(area);
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::Rectangle<int> lfo1Bounds, lfo2Bounds;

    juce::ComboBox lfo1ShapeBox, lfo1DestBox, lfo2ShapeBox, lfo2DestBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        lfo1ShapeAttach, lfo1DestAttach, lfo2ShapeAttach, lfo2DestAttach;

    KnobWithLabel knobLfo1Rate  { "Rate",  apvts, ParamIDs::lfo1Rate,  " Hz", 2 };
    KnobWithLabel knobLfo1Depth { "Depth", apvts, ParamIDs::lfo1Depth, "",    2 };
    KnobWithLabel knobLfo2Rate  { "Rate",  apvts, ParamIDs::lfo2Rate,  " Hz", 2 };
    KnobWithLabel knobLfo2Depth { "Depth", apvts, ParamIDs::lfo2Depth, "",    2 };

    std::unique_ptr<SynthVibe::ModMatrixTable> matrix;

    static void drawPanel(juce::Graphics& g, juce::Rectangle<int> bounds,
                          const juce::String& title, juce::uint32 accentColour)
    {
        auto f = bounds.toFloat().reduced(2.f);
        g.setColour(juce::Colour(SynthLookAndFeel::colPanel));
        g.fillRoundedRectangle(f, 6.f);
        g.setColour(juce::Colour(accentColour));
        g.drawRoundedRectangle(f, 6.f, 1.f);
        g.setColour(juce::Colour(SynthLookAndFeel::colHighlight));
        g.setFont(juce::Font(11.f, juce::Font::bold));
        g.drawText(title, bounds.getX() + 10, bounds.getY() + 4,
                   bounds.getWidth() - 20, 16, juce::Justification::centredLeft);
    }
};
