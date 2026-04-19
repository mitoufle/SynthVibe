#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "KnobWithLabel.h"
#include "LookAndFeel.h"
#include "../Presets/PresetManager.h"
#include "../Parameters/ParameterIDs.h"

class TopBar : public juce::Component
{
public:
    TopBar(juce::AudioProcessorValueTreeState& apvts, PresetManager& pm)
        : presetManager(pm)
    {
        logoLabel.setText("AI Synth", juce::dontSendNotification);
        logoLabel.setFont(juce::Font(14.f, juce::Font::bold));
        logoLabel.setColour(juce::Label::textColourId, juce::Colour(SynthLookAndFeel::colHighlight));
        addAndMakeVisible(logoLabel);

        presetNameLabel.setColour(juce::Label::textColourId, juce::Colour(SynthLookAndFeel::colText));
        presetNameLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(presetNameLabel);

        prevButton.setButtonText("<");
        nextButton.setButtonText(">");
        saveButton.setButtonText("Save");
        loadButton.setButtonText("Load");

        prevButton.onClick = [this] { navigatePreset(-1); };
        nextButton.onClick = [this] { navigatePreset(+1); };
        saveButton.onClick = [this] { saveCurrentPreset(); };
        loadButton.onClick = [this] { loadCurrentPreset(); };

        for (auto* b : { &prevButton, &nextButton, &saveButton, &loadButton })
            addAndMakeVisible(b);

        knobMaster = std::make_unique<KnobWithLabel>("Vol", apvts, ParamIDs::masterVolume, "", 2);
        addAndMakeVisible(*knobMaster);

        refreshPresetLabel();
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colour(SynthLookAndFeel::colAccent));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(6, 4);
        logoLabel.setBounds(area.removeFromLeft(90));
        if (knobMaster) knobMaster->setBounds(area.removeFromRight(56));
        saveButton.setBounds(area.removeFromRight(52));
        loadButton.setBounds(area.removeFromRight(52));
        area.removeFromRight(4);
        nextButton.setBounds(area.removeFromRight(28));
        prevButton.setBounds(area.removeFromLeft(28));
        presetNameLabel.setBounds(area);
    }

private:
    PresetManager& presetManager;
    int            currentPresetIndex = 0;

    juce::Label       logoLabel;
    juce::Label       presetNameLabel;
    juce::TextButton  prevButton, nextButton, saveButton, loadButton;
    std::unique_ptr<KnobWithLabel> knobMaster;

    void refreshPresetLabel()
    {
        auto names = presetManager.getPresetNames();
        if (names.isEmpty())
        {
            presetNameLabel.setText("Init", juce::dontSendNotification);
            currentPresetIndex = 0;
        }
        else
        {
            currentPresetIndex = juce::jlimit(0, names.size() - 1, currentPresetIndex);
            presetNameLabel.setText(names[currentPresetIndex], juce::dontSendNotification);
        }
    }

    void navigatePreset(int delta)
    {
        auto names = presetManager.getPresetNames();
        if (names.isEmpty()) return;
        currentPresetIndex = (currentPresetIndex + delta + names.size()) % names.size();
        presetManager.loadPreset(names[currentPresetIndex]);
        refreshPresetLabel();
    }

    void saveCurrentPreset()
    {
        auto* dialog = new juce::AlertWindow("Save Preset",
                                             "Enter preset name:",
                                             juce::MessageBoxIconType::NoIcon);
        dialog->addTextEditor("presetName", "New Preset", {});
        dialog->addButton("Save",   1, juce::KeyPress(juce::KeyPress::returnKey));
        dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

        juce::Component::SafePointer<TopBar> safeThis(this);

        dialog->enterModalState(true,
            juce::ModalCallbackFunction::create([safeThis, dialog](int result)
            {
                if (safeThis == nullptr)
                    return;

                if (result == 1)
                {
                    auto name = dialog->getTextEditorContents("presetName").trim();
                    if (name.isNotEmpty())
                    {
                        safeThis->presetManager.savePreset(name);
                        safeThis->refreshPresetLabel();
                    }
                }
            }),
            true); // deleteWhenDismissed
    }

    void loadCurrentPreset()
    {
        auto names = presetManager.getPresetNames();
        if (names.isEmpty()) return;
        presetManager.loadPreset(names[currentPresetIndex]);
    }
};
