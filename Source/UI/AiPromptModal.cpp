#include "AiPromptModal.h"
#include "DesignTokens.h"
#include "Fonts.h"

AiPromptModal::AiPromptModal(ClaudeClient& cc, PatchApplier& pa, ApiKeyStore& aks)
    : claudeClient(cc), patchApplier(pa), apiKeyStore(aks),
      settingsView(aks)
{
    using namespace SynthVibe::Tokens;

    setWantsKeyboardFocus(true);
    setInterceptsMouseClicks(true, true);

    // Header
    titleLabel.setText("\xE2\x9C\xA6  Describe the sound", juce::dontSendNotification);
    titleLabel.setColour(juce::Label::textColourId, ink);
    titleLabel.setFont(SynthVibe::Fonts::sans(Font::body, juce::Font::bold));
    addAndMakeVisible(titleLabel);

    gearButton.setButtonText("\xE2\x9A\x99");   // gear
    gearButton.onClick = [this] {
        if (mode == Mode::Generate) enterSettingsView();
        else                        enterGenerateView();
    };
    addAndMakeVisible(gearButton);

    closeButton.setButtonText("\xC3\x97");      // x
    closeButton.onClick = [this] { setVisible(false); };
    addAndMakeVisible(closeButton);

    // Prompt editor + char counter
    promptEditor.setMultiLine(true, true);
    promptEditor.setReturnKeyStartsNewLine(false);
    promptEditor.setColour(juce::TextEditor::backgroundColourId, panel2);
    promptEditor.setColour(juce::TextEditor::textColourId,       ink);
    promptEditor.setColour(juce::TextEditor::outlineColourId,    edge);
    promptEditor.onTextChange = [this] {
        updateGenerateButtonEnabled();
        updateCharCounter();
    };
    promptEditor.onReturnKey = [this] {
        requestGenerate(promptEditor.getText());
    };
    addAndMakeVisible(promptEditor);

    charCounterLabel.setColour(juce::Label::textColourId, ink3);
    charCounterLabel.setFont(SynthVibe::Fonts::mono(Font::tiny));
    charCounterLabel.setJustificationType(juce::Justification::centredRight);
    addChildComponent(charCounterLabel);   // visible only when prompt > 1500 chars

    // addChildComponent (NOT addAndMakeVisible) so the banner stays hidden
    // until errorBanner.show() is called from showErrorForResponse / requestGenerate.
    addChildComponent(errorBanner);
    errorBanner.onActionClicked = [this](SynthVibe::AiErrorBanner::Action a) {
        switch (a) {
            case SynthVibe::AiErrorBanner::Action::SetApiKey:
                // setMode(Settings) inside enterSettingsView already hides the banner.
                enterSettingsView();
                break;
            case SynthVibe::AiErrorBanner::Action::Retry:
                requestGenerate(promptEditor.getText());
                break;
            case SynthVibe::AiErrorBanner::Action::None:
                break;
        }
    };

    // Action row
    generateButton.setButtonText("Generate \xE2\x86\x97");   // arrow
    generateButton.setEnabled(false);
    generateButton.onClick = [this] {
        requestGenerate(promptEditor.getText());
    };
    addAndMakeVisible(generateButton);

    modelLabel.setText("Claude Sonnet 4.6", juce::dontSendNotification);
    modelLabel.setColour(juce::Label::textColourId, ink3);
    modelLabel.setFont(SynthVibe::Fonts::mono(Font::tiny));
    addAndMakeVisible(modelLabel);

    // Variation strip — 4 cards, hidden until first response.
    for (int i = 0; i < 4; ++i)
    {
        cards[i].setIndex(i);
        cards[i].onClicked = [this](int idx) { selectAndApply(idx); };
        cards[i].setEmpty();
        addChildComponent(cards[i]);
    }

    // Footer
    hintLabel.setText("Esc to dismiss \xC2\xB7 Enter to generate", juce::dontSendNotification);
    hintLabel.setColour(juce::Label::textColourId, ink3);
    hintLabel.setFont(SynthVibe::Fonts::mono(Font::tiny));
    addAndMakeVisible(hintLabel);

    regenerateButton.setButtonText("REGENERATE");
    regenerateButton.setEnabled(false);
    regenerateButton.onClick = [this] {
        requestGenerate(promptEditor.getText());
    };
    addAndMakeVisible(regenerateButton);

    // Settings body — hidden by default, switched in by enterSettingsView.
    addChildComponent(settingsView);
}

AiPromptModal::~AiPromptModal() = default;

void AiPromptModal::show()
{
    setVisible(true);
    toFront(true);
    promptEditor.grabKeyboardFocus();
}

void AiPromptModal::paint(juce::Graphics& g)
{
    using namespace SynthVibe::Tokens;

    // Backdrop dim
    g.fillAll(juce::Colour::fromFloatRGBA(0.f, 0.f, 0.f, 0.55f));

    // Modal card
    auto cardBounds = getLocalBounds().withSizeKeepingCentre(640, 380).toFloat();
    g.setColour(panel);
    g.fillRoundedRectangle(cardBounds, radiusLg);
    g.setColour(edge);
    g.drawRoundedRectangle(cardBounds, radiusLg, 1.0f);
}

void AiPromptModal::resized()
{
    using namespace SynthVibe::Tokens;
    auto card = getLocalBounds().withSizeKeepingCentre(640, 380).reduced(spaceLg);

    // Header
    auto header = card.removeFromTop(28);
    closeButton.setBounds(header.removeFromRight(28));
    header.removeFromRight(spaceSm);
    gearButton .setBounds(header.removeFromRight(28));
    header.removeFromRight(spaceSm);
    titleLabel .setBounds(header);

    card.removeFromTop(spaceMd);

    if (mode == Mode::Settings)
    {
        settingsView.setBounds(card);
        return;
    }

    // Generate body
    // Prompt editor (3 lines)
    auto editorRow = card.removeFromTop(60);
    auto counterRow = editorRow.removeFromBottom(14);
    charCounterLabel.setBounds(counterRow);
    promptEditor.setBounds(editorRow);

    card.removeFromTop(spaceSm);

    // Error banner (32 px row, but only visible iff errorBanner.isVisible)
    if (errorBanner.isVisible())
    {
        errorBanner.setBounds(card.removeFromTop(32));
        card.removeFromTop(spaceSm);
    }

    // Action row
    auto actionRow = card.removeFromTop(34);
    generateButton.setBounds(actionRow.removeFromLeft(140));
    actionRow.removeFromLeft(spaceMd);
    modelLabel.setBounds(actionRow);

    card.removeFromTop(spaceMd);

    // Variation strip — 4 cards
    auto strip = card.removeFromTop(110);
    const int cardW = (strip.getWidth() - 3 * spaceSm) / 4;
    for (int i = 0; i < 4; ++i)
    {
        cards[i].setBounds(strip.removeFromLeft(cardW));
        if (i < 3) strip.removeFromLeft(spaceSm);
    }

    card.removeFromTop(spaceMd);

    // Footer
    auto footer = card.removeFromTop(24);
    regenerateButton.setBounds(footer.removeFromRight(120));
    footer.removeFromRight(spaceMd);
    hintLabel.setBounds(footer);
}

bool AiPromptModal::keyPressed(const juce::KeyPress& k)
{
    if (k == juce::KeyPress::escapeKey)
    {
        setVisible(false);
        return true;
    }
    return false;
}

// --- State machine ---------------------------------------------------------

void AiPromptModal::setMode(Mode m)
{
    mode = m;
    const bool genVisible = (m == Mode::Generate);

    // Generate body. Sources of truth for sub-component visibility:
    //   currentBannerError != None  -> banner visible
    //   ! variations.empty()        -> variation strip visible
    promptEditor.setVisible(genVisible);
    charCounterLabel.setVisible(genVisible && promptEditor.getText().length() > 1500);
    errorBanner.setVisible(genVisible && currentBannerError != ClaudeClientError::None);
    generateButton.setVisible(genVisible);
    modelLabel.setVisible(genVisible);
    const bool stripVisible = genVisible && ! variations.empty();
    for (auto& c : cards) c.setVisible(stripVisible);
    hintLabel.setVisible(genVisible);
    regenerateButton.setVisible(genVisible);

    // Settings body
    settingsView.setVisible(! genVisible);

    titleLabel.setText(genVisible ? juce::String("\xE2\x9C\xA6  Describe the sound")
                                  : juce::String("\xE2\x9C\xA6  Settings"),
                       juce::dontSendNotification);
    resized();
    repaint();
}

void AiPromptModal::setState(State s)
{
    state = s;
    const bool busy = (s == State::Loading);
    generateButton.setButtonText(busy ? "Generating\xE2\x80\xA6" : "Generate \xE2\x86\x97");
    generateButton.setEnabled(! busy && ! promptEditor.getText().trim().isEmpty());
    regenerateButton.setEnabled(! busy && ! variations.empty());
}

void AiPromptModal::enterSettingsView()
{
    settingsView.refreshFromStore();
    setMode(Mode::Settings);
}

void AiPromptModal::enterGenerateView()
{
    setMode(Mode::Generate);
}

void AiPromptModal::saveApiKey(const juce::String& key)
{
    apiKeyStore.save(key.trim());
    // The user just supplied a key, so MissingApiKey is no longer the issue.
    // Don't clear other HttpError sub-cases (429 rate-limit, 5xx) — those want
    // the user to Retry, not just to set a key. Task 7's 401-specific clearing
    // can layer on top by also tracking the AiErrorBanner::Action.
    if (currentBannerError == ClaudeClientError::MissingApiKey)
    {
        errorBanner.hide();
        currentBannerError = ClaudeClientError::None;
        resized();
    }
}

void AiPromptModal::clearApiKey()
{
    apiKeyStore.clear();
}

// --- Stubs filled in Tasks 6-8 ---------------------------------------------

void AiPromptModal::requestGenerate(const juce::String& prompt)
{
    if (state == State::Loading) return;       // debounce

    const auto trimmed = prompt.trim();
    if (trimmed.isEmpty()) return;

    if (apiKeyStore.load().isEmpty())
    {
        errorBanner.show("No API key set.", SynthVibe::AiErrorBanner::Action::SetApiKey);
        currentBannerError = ClaudeClientError::MissingApiKey;
        resized();
        return;
    }

    setState(State::Loading);
    errorBanner.hide();
    currentBannerError = ClaudeClientError::None;
    resized();

    juce::WeakReference<AiPromptModal> weakSelf(this);
    claudeClient.requestPatches(trimmed, 4,
        [weakSelf](ClaudeResponse resp)
        {
            if (auto* self = weakSelf.get())
                self->onClaudeResponse(std::move(resp));
        });
}

void AiPromptModal::onClaudeResponse(ClaudeResponse resp)
{
    setState(State::Idle);

    if (resp.error != ClaudeClientError::None)
    {
        showErrorForResponse(resp);
        return;
    }

    variations    = std::move(resp.variations);
    selectedCard  = -1;
    rebuildVariationStrip();
    resized();
}

void AiPromptModal::showErrorForResponse(const ClaudeResponse& resp)
{
    juce::String msg;
    auto action = SynthVibe::AiErrorBanner::Action::Retry;

    switch (resp.error)
    {
        case ClaudeClientError::MissingApiKey:
            msg    = "No API key set.";
            action = SynthVibe::AiErrorBanner::Action::SetApiKey;
            break;

        case ClaudeClientError::NetworkFailure:
            msg    = "Could not reach Anthropic API.";
            action = SynthVibe::AiErrorBanner::Action::Retry;
            break;

        case ClaudeClientError::Timeout:
            msg    = "Request timed out (20s).";
            action = SynthVibe::AiErrorBanner::Action::Retry;
            break;

        case ClaudeClientError::HttpError:
            if (resp.httpStatus == 401)
            {
                msg    = "API key rejected by Anthropic.";
                action = SynthVibe::AiErrorBanner::Action::SetApiKey;
            }
            else if (resp.httpStatus == 429)
            {
                msg    = "Rate limit reached. Wait a moment.";
                action = SynthVibe::AiErrorBanner::Action::Retry;
            }
            else
            {
                msg    = "Anthropic API error (status " + juce::String(resp.httpStatus) + ").";
                action = SynthVibe::AiErrorBanner::Action::Retry;
            }
            break;

        case ClaudeClientError::ParseError:
            msg    = "Could not parse Claude's response.";
            action = SynthVibe::AiErrorBanner::Action::Retry;
            break;

        case ClaudeClientError::SchemaError:
            msg    = "Claude returned no patches. Try a different prompt.";
            action = SynthVibe::AiErrorBanner::Action::None;
            break;

        case ClaudeClientError::None:
            return;
    }

    currentBannerError = resp.error;
    errorBanner.show(msg, action);
    resized();
}
void AiPromptModal::selectAndApply(int cardIndex)
{
    if (cardIndex < 0 || cardIndex >= (int) variations.size()) return;

    const auto& v = variations[cardIndex];
    if (v.params.empty()) return;     // empty-Variation = no-op, no selection

    selectedCard = cardIndex;
    rebuildVariationStrip();          // refresh selection highlight on cards

    auto report = patchApplier.apply(v);
    if (report.unknown > 0)
    {
        juce::Logger::writeToLog("AiPromptModal: " + juce::String(report.unknown)
                                 + " unknown paramIds in variation '" + v.name + "'");
    }
}

// --- Inspection ------------------------------------------------------------

int          AiPromptModal::getVariationCount()    const { return (int) variations.size(); }
juce::String AiPromptModal::getErrorBannerText()   const { return errorBanner.getMessageText(); }
bool         AiPromptModal::isLoading()            const { return state == State::Loading; }
int          AiPromptModal::getSelectedCardIndex() const { return selectedCard; }

// --- Helpers ---------------------------------------------------------------

void AiPromptModal::rebuildVariationStrip()
{
    const bool stripVisible = (mode == Mode::Generate) && ! variations.empty();
    for (int i = 0; i < 4; ++i)
    {
        if (i < (int) variations.size())
        {
            cards[i].setVariation(variations[i]);
            cards[i].setSelected(i == selectedCard);
        }
        else
        {
            cards[i].setEmpty();
            cards[i].setSelected(false);
        }
        cards[i].setVisible(stripVisible);   // empty slots in a populated strip paint "—"
    }
}

void AiPromptModal::updateGenerateButtonEnabled()
{
    generateButton.setEnabled(state != State::Loading
                              && ! promptEditor.getText().trim().isEmpty());
}

void AiPromptModal::updateCharCounter()
{
    const int n = promptEditor.getText().length();
    if (n > 1500)
    {
        charCounterLabel.setText("(" + juce::String(n) + " / 2000)",
                                 juce::dontSendNotification);
        charCounterLabel.setVisible(mode == Mode::Generate);
    }
    else
    {
        charCounterLabel.setVisible(false);
    }
}
