#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <map>
#include "../AI/ClaudeClient.h"
#include "../AI/PatchApplier.h"
#include "../AI/ApiKeyStore.h"
#include "../AI/Variation.h"
#include "components/VariationCard.h"
#include "components/AiErrorBanner.h"
#include "components/AiSettingsView.h"

class AiPromptModal : public juce::Component
{
public:
    AiPromptModal(ClaudeClient&, PatchApplier&, ApiKeyStore&,
                  juce::AudioProcessorValueTreeState& apvts);
    ~AiPromptModal() override;

    void show();

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;

    // Test seams (also called from internal click/key handlers).
    void requestGenerate(const juce::String& prompt);
    void selectAndApply(int cardIndex);
    void enterSettingsView();
    void enterGenerateView();
    void saveApiKey(const juce::String& key);
    void clearApiKey();

    // Inspection.
    int          getVariationCount()  const;
    juce::String getErrorBannerText() const;
    bool         isLoading()          const;
    int          getSelectedCardIndex() const;

private:
    enum class Mode  { Generate, Settings };
    enum class State { Idle, Loading };

    void onClaudeResponse(ClaudeResponse);
    void rebuildVariationStrip();
    void setMode(Mode);
    void setState(State);
    void showErrorForResponse(const ClaudeResponse&);
    void updateGenerateButtonEnabled();
    void updateCharCounter();

    ClaudeClient& claudeClient;
    PatchApplier& patchApplier;
    ApiKeyStore&  apiKeyStore;
    juce::AudioProcessorValueTreeState& apvts;

    Mode  mode  = Mode::Generate;
    State state = State::Idle;

    std::vector<Variation> variations;
    int selectedCard = -1;
    ClaudeClientError currentBannerError = ClaudeClientError::None;

    // Audition snapshot: per-paramId raw values captured on the FIRST card
    // click of each modal session. Subsequent clicks restore these values
    // before applying the new variation, so V1 -> V2 -> V1 returns to
    // identical knob positions. Cleared in show() so each modal opening
    // starts a fresh audition session. Empty == "not yet snapshotted".
    // (apvts.copyState/replaceState was tried first but didn't reliably push
    // restored values back into the parameter atomics in JUCE 7.0.9 — using
    // setValueNotifyingHost per parameter is more deterministic.)
    std::map<juce::String, float> preApplySnapshot;

    // Header
    juce::Label      titleLabel;
    juce::TextButton gearButton, closeButton;

    // Generate body
    juce::TextEditor       promptEditor;
    juce::Label            charCounterLabel;
    SynthVibe::AiErrorBanner errorBanner;
    juce::TextButton       generateButton;
    juce::Label            modelLabel;
    SynthVibe::VariationCard cards[4];
    juce::Label            hintLabel;
    juce::TextButton       regenerateButton;

    // Settings body
    SynthVibe::AiSettingsView settingsView;

    JUCE_DECLARE_WEAK_REFERENCEABLE(AiPromptModal)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AiPromptModal)
};
