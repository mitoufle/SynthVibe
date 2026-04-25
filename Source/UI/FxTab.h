#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DesignTokens.h"
#include "components/FxChainStrip.h"
#include "components/PanelHeader.h"

class FxTab : public juce::Component
{
public:
    explicit FxTab(juce::AudioProcessorValueTreeState& apvts) : apvts(apvts)
    {
        strip = std::make_unique<SynthVibe::FxChainStrip>(apvts);
        addAndMakeVisible(*strip);
    }

    SynthVibe::FxChainStrip* getStrip() noexcept { return strip.get(); }

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
        if (strip != nullptr)
            strip->setBounds(area);
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::unique_ptr<SynthVibe::FxChainStrip> strip;
};
