#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DesignTokens.h"
#include "../Presets/PresetManager.h"
#include "../Parameters/ParameterIDs.h"
#include "components/BrandBadge.h"
#include "components/PresetNameField.h"
#include "components/PromptButton.h"
#include "components/NavArrowButton.h"
#include "components/MasterVolKnob.h"

class TopBar : public juce::Component
{
public:
    std::function<void()> onPromptRequested;   // Phase 4 wires the AI modal here

    TopBar(juce::AudioProcessorValueTreeState& apvts, PresetManager& pm)
        : presetManager(pm),
          prevArrow(SynthVibe::NavArrowButton::Direction::Prev),
          nextArrow(SynthVibe::NavArrowButton::Direction::Next),
          masterKnob(apvts, ParamIDs::masterVolume)
    {
        addAndMakeVisible(brand);
        addAndMakeVisible(prevArrow);
        addAndMakeVisible(nameField);
        addAndMakeVisible(nextArrow);
        addAndMakeVisible(promptBtn);
        addAndMakeVisible(masterKnob);

        prevArrow.onClick = [this] { navigatePreset(-1); };
        nextArrow.onClick = [this] { navigatePreset(+1); };
        promptBtn.onClick = [this] { if (onPromptRequested) onPromptRequested(); };

        refreshPresetLabel();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace SynthVibe::Tokens;
        auto b = getLocalBounds().toFloat();
        g.setColour(panel);
        g.fillRect(b);
        g.setColour(edge);
        g.drawHorizontalLine(getHeight() - 1, 0.f, (float) getWidth());
    }

    void resized() override
    {
        using namespace SynthVibe::Tokens;
        auto area = getLocalBounds().reduced(spaceLg, 0);

        // Left column: brand (260 px)
        auto left = area.removeFromLeft(260);
        brand.setBounds(left.withSizeKeepingCentre(left.getWidth(), 28));

        // Right column: prompt + master volume (260 px)
        auto right = area.removeFromRight(260);
        masterKnob.setBounds(right.removeFromRight(80));
        right.removeFromRight(spaceSm);
        promptBtn.setBounds(right.withSizeKeepingCentre(100, 28));

        // Centre column: [prev-arrow | name-field | next-arrow]
        // Vertically centre a 38 px tall row inside the centre region.
        auto centre = area.withSizeKeepingCentre(area.getWidth(), 38);
        const int arrowW = 36;   // pill-ish; tweak in Phase 2 alongside the full top-bar breakpoint sweep
        prevArrow.setBounds(centre.removeFromLeft(arrowW));
        centre.removeFromLeft(spaceSm);
        nextArrow.setBounds(centre.removeFromRight(arrowW));
        centre.removeFromRight(spaceSm);
        nameField.setBounds(centre);
    }

private:
    PresetManager& presetManager;
    int currentPresetIndex = 0;

    SynthVibe::BrandBadge       brand;
    SynthVibe::NavArrowButton   prevArrow;
    SynthVibe::PresetNameField  nameField;
    SynthVibe::NavArrowButton   nextArrow;
    SynthVibe::PromptButton     promptBtn;
    SynthVibe::MasterVolKnob    masterKnob;

    void refreshPresetLabel()
    {
        auto names = presetManager.getPresetNames();
        if (names.isEmpty())
        {
            nameField.setPresetName("Init");
            nameField.setMetaText("—");
            currentPresetIndex = 0;
            prevArrow.setEnabled(false);
            nextArrow.setEnabled(false);
            return;
        }
        currentPresetIndex = juce::jlimit(0, names.size() - 1, currentPresetIndex);
        nameField.setPresetName(names[currentPresetIndex]);
        nameField.setMetaText(juce::String(currentPresetIndex + 1) + " / " + juce::String(names.size()));
        prevArrow.setEnabled(currentPresetIndex > 0);
        nextArrow.setEnabled(currentPresetIndex < names.size() - 1);
    }

    void navigatePreset(int delta)
    {
        auto names = presetManager.getPresetNames();
        if (names.isEmpty()) return;
        currentPresetIndex = (currentPresetIndex + delta + names.size()) % names.size();
        presetManager.loadPreset(names[currentPresetIndex]);
        refreshPresetLabel();
    }
};
