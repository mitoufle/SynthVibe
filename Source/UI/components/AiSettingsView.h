#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../DesignTokens.h"
#include "../Fonts.h"
#include "../../AI/ApiKeyStore.h"

namespace SynthVibe
{
    class AiSettingsView : public juce::Component
    {
    public:
        explicit AiSettingsView(ApiKeyStore& storeRef) : store(storeRef)
        {
            apiKeyEditor.setMultiLine(false);
            apiKeyEditor.setPasswordCharacter(juce::juce_wchar(0x2022));   // bullet
            apiKeyEditor.setColour(juce::TextEditor::backgroundColourId, Tokens::panel2);
            apiKeyEditor.setColour(juce::TextEditor::textColourId,       Tokens::ink);
            apiKeyEditor.setColour(juce::TextEditor::outlineColourId,    Tokens::edge);
            addAndMakeVisible(apiKeyEditor);

            revealToggle.setButtonText(juce::String(juce::CharPointer_UTF8("\xF0\x9F\x91\x81")));   // eye glyph
            revealToggle.setClickingTogglesState(true);
            revealToggle.onClick = [this] {
                apiKeyEditor.setPasswordCharacter(
                    revealToggle.getToggleState() ? juce::juce_wchar(0)
                                                  : juce::juce_wchar(0x2022));
                apiKeyEditor.repaint();
            };
            addAndMakeVisible(revealToggle);

            saveButton.setButtonText("Save");
            saveButton.onClick = [this] {
                store.save(apiKeyEditor.getText().trim());
                statusLabel.setText("Saved.", juce::dontSendNotification);
            };
            addAndMakeVisible(saveButton);

            clearButton.setButtonText("Clear");
            clearButton.onClick = [this] {
                store.clear();
                apiKeyEditor.setText({}, juce::dontSendNotification);
                statusLabel.setText("Cleared.", juce::dontSendNotification);
            };
            addAndMakeVisible(clearButton);

            statusLabel.setColour(juce::Label::textColourId, Tokens::ink2);
            statusLabel.setFont(Fonts::sans(Tokens::Font::label));
            statusLabel.setJustificationType(juce::Justification::centredLeft);
            addAndMakeVisible(statusLabel);
        }

        void refreshFromStore()
        {
            apiKeyEditor.setText(store.load(), juce::dontSendNotification);
            statusLabel.setText({}, juce::dontSendNotification);
        }

        // Test seams.
        juce::TextEditor& getApiKeyEditor() { return apiKeyEditor; }
        juce::TextButton& getSaveButton()   { return saveButton;   }
        juce::TextButton& getClearButton()  { return clearButton;  }
        juce::Label&      getStatusLabel()  { return statusLabel;  }

        void resized() override
        {
            using namespace Tokens;
            auto area = getLocalBounds().reduced(spaceMd);

            // Top row: api key editor + reveal toggle
            auto row1 = area.removeFromTop(28);
            revealToggle.setBounds(row1.removeFromRight(28));
            row1.removeFromRight(spaceSm);
            apiKeyEditor.setBounds(row1);

            area.removeFromTop(spaceSm);

            // Buttons row
            auto row2 = area.removeFromTop(28);
            saveButton.setBounds(row2.removeFromLeft(80));
            row2.removeFromLeft(spaceSm);
            clearButton.setBounds(row2.removeFromLeft(80));
            row2.removeFromLeft(spaceMd);
            statusLabel.setBounds(row2);
        }

    private:
        // SyncButton overrides triggerClick() to fire onClick synchronously.
        // juce::TextButton::triggerClick() posts an async command message, which
        // means unit tests calling triggerClick() would need to pump the message
        // queue before asserting state. SyncButton bypasses that for test seams;
        // real mouse-click paths still go through internalClickCallback → onClick.
        struct SyncButton : juce::TextButton
        {
            void triggerClick() override
            {
                if (onClick) onClick();
            }
        };

        ApiKeyStore&     store;
        juce::TextEditor apiKeyEditor;
        juce::TextButton revealToggle;
        SyncButton       saveButton;
        SyncButton       clearButton;
        juce::Label      statusLabel;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AiSettingsView)
    };
}
