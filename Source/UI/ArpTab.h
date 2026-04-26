#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Parameters/ParameterIDs.h"
#include "DesignTokens.h"
#include "components/PanelHeader.h"
#include "components/ArpOnOffPill.h"
#include "components/SegmentedButtonRow.h"
#include "components/ArcKnob.h"

class ArpTab : public juce::Component
{
public:
    explicit ArpTab(juce::AudioProcessorValueTreeState& apvts)
        : header("ARP", SynthVibe::Tokens::arp),
          onPill(apvts, ParamIDs::arpEnabled),
          patternRow(apvts, ParamIDs::arpMode,
                     juce::StringArray { "Up", "Dn", "UpDn", "DnUp", "Rnd", "Played", "Chord" },
                     SynthVibe::Tokens::arp),
          rateRow(apvts, ParamIDs::arpRate,
                  juce::StringArray { "1/4", "1/8", "1/16", "1/16T", "1/32" },
                  SynthVibe::Tokens::arp),
          octavesKnob ("Octaves",  apvts, ParamIDs::arpOctaveRange, SynthVibe::Tokens::arp, "", 0),
          gateKnob    ("Gate",     apvts, ParamIDs::arpGate,        SynthVibe::Tokens::arp, "", 2),
          swingKnob   ("Swing",    apvts, ParamIDs::arpSwing,       SynthVibe::Tokens::arp, "", 2),
          humanizeKnob("Humanize", apvts, ParamIDs::arpHumanize,    SynthVibe::Tokens::arp, "", 2)
    {
        latchToggle.setButtonText("Latch");
        latchToggle.setClickingTogglesState(true);
        latchAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, ParamIDs::arpLatch, latchToggle);

        addAndMakeVisible(header);
        addAndMakeVisible(onPill);
        addAndMakeVisible(latchToggle);
        addAndMakeVisible(patternRow);
        addAndMakeVisible(rateRow);
        addAndMakeVisible(octavesKnob);
        addAndMakeVisible(gateKnob);
        addAndMakeVisible(swingKnob);
        addAndMakeVisible(humanizeKnob);
    }

    SynthVibe::ArpOnOffPill*       getOnPill()       noexcept { return &onPill; }
    SynthVibe::SegmentedButtonRow* getPatternRow()   noexcept { return &patternRow; }
    SynthVibe::SegmentedButtonRow* getRateRow()      noexcept { return &rateRow; }
    juce::ToggleButton*            getLatchToggle()  noexcept { return &latchToggle; }

    void paint(juce::Graphics& g) override
    {
        using namespace SynthVibe::Tokens;
        auto b = getLocalBounds().toFloat().reduced(2.f);
        g.setColour(panel);
        g.fillRoundedRectangle(b, radiusLg);
        g.setColour(edge);
        g.drawRoundedRectangle(b.reduced(0.5f), radiusLg, 1.f);
    }

    void resized() override
    {
        using namespace SynthVibe::Tokens;
        auto area = getLocalBounds().reduced(spaceMd);

        header.setBounds(area.removeFromTop(20));
        area.removeFromTop(spaceSm);

        // Top row: pill (left) + latch toggle (right)
        auto topRow = area.removeFromTop(28);
        onPill.setBounds(topRow.removeFromLeft(100));
        topRow.removeFromLeft(spaceSm);
        latchToggle.setBounds(topRow.removeFromRight(80));
        area.removeFromTop(spaceSm);

        patternRow.setBounds(area.removeFromTop(28));
        area.removeFromTop(spaceXs);
        rateRow.setBounds(area.removeFromTop(28));
        area.removeFromTop(spaceSm);

        // Knob row: 4 knobs, equal width
        auto knobRow = area.removeFromTop(80);
        const int kw = knobRow.getWidth() / 4;
        octavesKnob .setBounds(knobRow.removeFromLeft(kw));
        gateKnob    .setBounds(knobRow.removeFromLeft(kw));
        swingKnob   .setBounds(knobRow.removeFromLeft(kw));
        humanizeKnob.setBounds(knobRow);
    }

private:
    SynthVibe::PanelHeader header;
    SynthVibe::ArpOnOffPill onPill;
    juce::ToggleButton latchToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> latchAttach;
    SynthVibe::SegmentedButtonRow patternRow;
    SynthVibe::SegmentedButtonRow rateRow;
    SynthVibe::ArcKnob octavesKnob;
    SynthVibe::ArcKnob gateKnob;
    SynthVibe::ArcKnob swingKnob;
    SynthVibe::ArcKnob humanizeKnob;
};
