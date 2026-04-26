# Phase A.2 — AI Layer UI Design Spec

> **Scope:** the user-facing modal that turns `ClaudeClient::requestPatches` (shipped in A.1) into an interactive feature — prompt textbox, Generate, 4 variation cards, Settings panel for the API key, error display. After A.2 the AI layer is auditionable end-to-end inside Reaper.

**Source of truth (existing):**
- `docs/superpowers/specs/2026-04-26-ai-layer-a1-design.md` — backend contract that A.2 consumes.
- `Source/AI/*` — public headers locked by A.1 (`ClaudeClient::requestPatches`, `PatchApplier::apply`, `ApiKeyStore::{load,save,clear}`).
- Target visual: user-supplied mockup with modal overlay (textarea + Generate + 4 cards + footer hints).

**Goal sentence:** ship `Source/UI/AiPromptModal.{h,cpp}` + sub-components wired into `AISynthEditor`, such that clicking the existing TopBar PROMPT button opens the modal, the user types a prompt, gets 4 variations back from Claude, and click-to-apply writes the synth's APVTS — all without leaving the plugin window.

---

## What this phase does NOT ship

Deferred to **A.3 (polish)** or later — out of scope for A.2:

- Mini-waveform preview per variation card (requires offline render engine; 3–5 commits on its own).
- "Surprise me" toggle (semantics undefined; defer until product clarity).
- "Attach reference" button (requires preset picker + context injection into the prompt).
- Cost telemetry / "$1.34" indicator (requires token-counting + persistent counter).
- "vol N" badge on each card (semantics unclear in mockup).
- ↓↑ keyboard cycling with auto-apply audition (requires snapshot/restore — see Q3 option C in brainstorm).
- Prompt history drawer.
- APPLY SELECTION button — explicitly removed (per audition decision: click=apply makes the explicit commit button redundant).
- Audition with restore-on-cancel (Q3 option C). A.2 ships click=apply (option A); A.3 can layer C on top via `apvts.copyState`/`replaceState` without API change.
- MCP server / HTTP REST exposure → sub-layer B.

---

## UX decisions (locked)

| # | Decision | Choice |
|---|---|---|
| 1 | Form factor | Modal overlay (synth dimmed but visible behind). Backdrop is a painted layer in `AiPromptModal::paint()`, not a separate OS window. |
| 2 | Audition behavior | Click card = apply instantly via `PatchApplier::apply` (gestures). No snapshot, no APPLY SELECTION button. Host's undo (Ctrl+Z in Reaper) is the safety net. |
| 3 | Settings entry | Gear icon in modal header. Click swaps modal body Generate ↔ Settings. No second modal, no TopBar gear icon. |
| 4 | Error UX | Inline banner between `promptEditor` and the action row. Contextual action button per error type (`Set API key →`, `Retry`, hidden). |
| 5 | Number of variations | Hardcoded N=4 in the request body. ClaudeClient supports 1–8; A.2 doesn't expose the dial. |
| 6 | State preservation | Prompt text + last 4 cards persist across modal open/close while the editor is alive. Cleared only on explicit Generate. |
| 7 | Selection persistence | Selecting a card highlights it permanently until another card is clicked or new variations arrive. No deselect on click-elsewhere. |

---

## Architecture

### Layers

```
AISynthProcessor (lifetime: plugin instance)
 ├ apvts                        existing
 ├ presetManager                existing
 ├ keyboardState                existing
 ├ synth, fxRunner, arp         existing
 │
 ├ ApiKeyStore                  A.1 shipped the class — A.2 adds it as a member
 ├ JuceHttpTransport            A.1 shipped the class — A.2 adds it as a member
 ├ ClaudeClient                 A.1 shipped the class — A.2 adds it as a member (refs apiKeyStore + transport)
 └ PatchApplier                 A.1 shipped the class — A.2 adds it as a member (refs apvts)

AISynthEditor (lifetime: plugin window)
 ├ topBar.onPromptRequested → editor.showAiModal()
 ├ tabs, keyboard, bottomBar    existing
 │
 └ AiPromptModal                NEW — child Component, addChildComponent (hidden by default)
      │ refs:
      │   processor.claudeClient   (call requestPatches)
      │   processor.patchApplier   (call apply)
      │   processor.apiKeyStore    (load/save in Settings)
      │
      └ Callbacks captured via WeakReference<AiPromptModal> for cross-thread safety.
```

### Lifetime invariants

- Services on Processor outlive the editor → callbacks fired from worker thread can safely reference services even if the editor is closed.
- Modal lives as long as Editor → callbacks targeting modal use `WeakReference<AiPromptModal>` — modal destroyed = callback no-op.
- ClaudeClient already exposes `JUCE_DECLARE_WEAK_REFERENCEABLE` (A.1). We mirror the macro on `AiPromptModal`.

### Why this architecture (alternatives considered)

| Alternative | Verdict |
|---|---|
| `juce::DialogWindow` (OS-native modal) | Rejected — doesn't match the mockup (synth visible/dimmed behind = overlay child). DialogWindow titlebar conflicts with the design. |
| Lazy-create modal on PROMPT click, destroy on close | Rejected — loses prompt text + cards across opens, more complex lifetime, no real footprint gain (modal is small). |
| Services on Editor instead of Processor | Rejected — Editor is recreated on every plugin window open/close; services would lose state (e.g., in-flight requests). |

### Files (new)

| File | Lines (est.) | Responsibility |
|---|---|---|
| `Source/UI/AiPromptModal.h` | ~80 | Public Component, owns sub-components, `show()` / `hide()`, test seams |
| `Source/UI/AiPromptModal.cpp` | ~300 | Layout + state machine + callback wiring + paint backdrop |
| `Source/UI/components/VariationCard.h` | ~80 | Header-only Component (matches existing pattern) |
| `Source/UI/components/AiErrorBanner.h` | ~70 | Header-only — shows message + optional action button |
| `Source/UI/components/AiSettingsView.h` | ~90 | Header-only — API key input + save / clear / status label |
| **Tests:** | | |
| `Tests/AiPromptModalTests.cpp` | ~250 | 9 blocks (see Tests section) |
| `Tests/AiSettingsViewTests.cpp` | ~80 | 3 blocks |

### Files (modified)

| File | Change |
|---|---|
| `Source/PluginProcessor.h` | Add 4 service members (`ApiKeyStore`, `JuceHttpTransport`, `ClaudeClient`, `PatchApplier`); make them public for editor access. None of these were wired into the Processor in A.1 — A.2 owns this integration. |
| `Source/PluginProcessor.cpp` | Initialize members in constructor. No `prepareToPlay` impact. |
| `Source/PluginEditor.h` | Add `AiPromptModal` member; add `showAiModal()` private helper. |
| `Source/PluginEditor.cpp` | Wire `topBar.onPromptRequested = [this]{ showAiModal(); }`. Add modal to layout (covers full editor bounds via `setBounds` in `resized`). |
| `Tests/UIConstructionTests.cpp` | +2 blocks: `AiPromptModal` construction, `VariationCard` construction. |
| `CMakeLists.txt` | Add `Source/UI/AiPromptModal.cpp` to plugin sources. Add 2 new test sources to test target. |

No CMake `find_package` additions. No new third-party dependency.

---

## Components

### `AiPromptModal` (top-level)

```
AiPromptModal : juce::Component
 ├─ Backdrop          painted in paint() — full-bounds dim layer rgba(0,0,0,0.55)
 ├─ ModalCard         centered Component (~640 x 380 px) holding all content
 │    ├─ Header
 │    │    ├─ titleLabel        "✦ Describe the sound" / "✦ Settings"
 │    │    ├─ gearButton        toggles Settings view
 │    │    └─ closeButton       × → setVisible(false)
 │    │
 │    ├─ GenerateBody (visible iff mode == Generate)
 │    │    ├─ promptEditor       juce::TextEditor multi-line, returnKeyTriggers callback
 │    │    ├─ AiErrorBanner      hidden unless last response was an error
 │    │    ├─ ActionRow
 │    │    │    ├─ generateButton   "Generate ↗" (spinner state when loading)
 │    │    │    └─ modelLabel       "Claude Sonnet 4.6"
 │    │    ├─ VariationStrip     row of 4 VariationCards (hidden until first response)
 │    │    └─ Footer
 │    │         ├─ hintLabel        "Esc to dismiss · Enter to generate"
 │    │         └─ regenerateButton "REGENERATE" (disabled until first response, disabled while loading)
 │    │
 │    └─ SettingsBody (visible iff mode == Settings)
 │         └─ AiSettingsView      composite of api-key editor + save/clear + status
```

### `AiPromptModal` public API

```cpp
class AiPromptModal : public juce::Component {
public:
    AiPromptModal(ClaudeClient&, PatchApplier&, ApiKeyStore&);
    ~AiPromptModal() override;

    void show();                              // setVisible(true) + grabKeyboardFocus
    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;   // Esc to dismiss

    // Test seams (also called from internal click/key handlers)
    void requestGenerate(const juce::String& prompt);
    void selectAndApply(int cardIndex);
    void enterSettingsView();
    void enterGenerateView();
    void saveApiKey(const juce::String& key);
    void clearApiKey();

    // Inspection (test-only callers; harmless in production)
    int          getVariationCount() const;
    juce::String getErrorBannerText() const;
    bool         isLoading() const;
    int          getSelectedCardIndex() const;

private:
    enum class Mode { Generate, Settings };
    enum class State { Idle, Loading };

    void onClaudeResponse(ClaudeResponse);
    void rebuildVariationStrip();
    void setMode(Mode);
    void setState(State);

    ClaudeClient& claudeClient;
    PatchApplier& patchApplier;
    ApiKeyStore&  apiKeyStore;

    Mode  mode = Mode::Generate;
    State state = State::Idle;

    std::vector<Variation> variations;
    int selectedCard = -1;

    /* sub-components ... */

    JUCE_DECLARE_WEAK_REFERENCEABLE(AiPromptModal)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AiPromptModal)
};
```

### `VariationCard` (header-only)

```cpp
class VariationCard : public juce::Component {
public:
    std::function<void(int)> onClicked;   // index passed back to modal

    void setVariation(const Variation&);
    void setEmpty();                       // paints "—", non-clickable
    void setSelected(bool);                // toggles highlight border
    void setIndex(int);                    // for the onClicked callback

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void resized() override;

private:
    Variation variation;
    bool empty = true;
    bool selected = false;
    int  index = 0;
    juce::Label nameLabel, descLabel;
    juce::OwnedArray<juce::Label> tagBadges;   // up to 3
};
```

### `AiErrorBanner` (header-only)

```cpp
class AiErrorBanner : public juce::Component {
public:
    enum class Action { None, SetApiKey, Retry };
    std::function<void(Action)> onActionClicked;

    void show(const juce::String& message, Action);
    void hide();

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    juce::Label messageLabel;
    juce::TextButton actionButton;
    Action currentAction = Action::None;
};
```

### `AiSettingsView` (header-only)

```cpp
class AiSettingsView : public juce::Component {
public:
    explicit AiSettingsView(ApiKeyStore&);
    std::function<void()> onBackRequested;

    void refreshFromStore();                // called when entering view
    void resized() override;

private:
    ApiKeyStore& store;
    juce::TextEditor apiKeyEditor;          // passwordChar = '•' by default
    juce::TextButton revealToggle;          // 👁 eye toggle
    juce::TextButton saveButton, clearButton;
    juce::Label      statusLabel;
};
```

---

## Data flow

### Flow / Open modal

```
TopBar PROMPT click
 → topBar.onPromptRequested()
 → editor.showAiModal()
     → aiPromptModal.setVisible(true)
     → aiPromptModal.toFront(true)
     → promptEditor.grabKeyboardFocus()
 (state untouched: prompt + cards persist)
```

### Flow / Generate (happy path)

```
generateButton click  [or Enter pressed in promptEditor]
 → AiPromptModal::requestGenerate(prompt)
     1. trim prompt; if empty → flash promptEditor border, return
     2. if apiKeyStore.load() empty → ErrorBanner.show("No API key set.", Action::SetApiKey); return
     3. setState(Loading): generateButton spinner, regenerateButton disabled, ErrorBanner.hide()
     4. claudeClient.requestPatches(prompt, 4,
            [weakSelf = WeakReference<AiPromptModal>(this)] (ClaudeResponse resp) {
                if (auto* self = weakSelf.get())
                    self->onClaudeResponse(std::move(resp));
            })
 → (worker thread, A.1 logic — assembles request, posts, parses tool_use blocks)
 → (back on message thread via MessageManager::callAsync inside ClaudeClient)
 → AiPromptModal::onClaudeResponse(resp)
     5. setState(Idle)
     6. if resp.error != None → ErrorBanner.show(<contextual message>, <contextual action>); return
     7. variations = std::move(resp.variations); rebuildVariationStrip()
     8. variationStrip.setVisible(true), trigger resized()
     9. selectedCard = -1
```

### Flow / Apply card

```
VariationCard click on card[i]
 → VariationCard::onClicked(i) → AiPromptModal::selectAndApply(i)
     1. if i out of range or variations[i].params empty → return
     2. selectedCard = i; trigger repaint
     3. patchApplier.apply(variations[i])  → ApplyReport
         (begin/setValue/end gestures per param, A.1 logic)
     4. if report.unknown > 0 → juce::Logger::warning(...); no UI banner
```

### Flow / Regenerate

Identical to Generate flow starting at step 1. ClaudeClient supersedes any in-flight request automatically (A.1 invariant).

### Flow / Settings — open and save

```
gearButton click
 → setMode(Settings):
     GenerateBody.setVisible(false), SettingsBody.setVisible(true)
     titleLabel.setText("✦ Settings")
     aiSettingsView.refreshFromStore()
       → apiKeyEditor.setText(apiKeyStore.load(), juce::dontSendNotification)

(within Settings)
 saveButton click
   → AiPromptModal::saveApiKey(apiKeyEditor.getText())
   → apiKeyStore.save(text.trim())
   → statusLabel.setText("Saved.")

 clearButton click
   → AiPromptModal::clearApiKey()
   → apiKeyStore.clear()
   → apiKeyEditor.setText("")
   → statusLabel.setText("Cleared.")

gearButton click again [or close button]
 → setMode(Generate)
```

### Flow / Error display

| Error | Banner text | Action button |
|---|---|---|
| `MissingApiKey` | "No API key set." | `Set API key →` (toggles to Settings view) |
| `NetworkFailure` | "Could not reach Anthropic API." | `Retry` (re-runs Generate) |
| `Timeout` | "Request timed out (20s)." | `Retry` |
| `HttpError` 401 | "API key rejected by Anthropic." | `Set API key →` |
| `HttpError` 429 | "Rate limit reached. Wait a moment." | `Retry` |
| `HttpError` other | "Anthropic API error (status N)." | `Retry` |
| `ParseError` | "Could not parse Claude's response." | `Retry` |
| `SchemaError` | "Claude returned no patches. Try a different prompt." | hidden |

Partial successes (≥1 valid + ≥1 malformed Variation) — A.1 returns `error: None`. Valid cards populate; malformed slots paint "—" and are non-clickable.

### Flow / Close-during-flight + editor-close-during-flight

- `Esc` or × → `setVisible(false)`. In-flight request keeps running. Callback fires later, modal still alive but hidden — state updates silently. Next open shows the new cards.
- Editor close → modal destroyed → `WeakReference` becomes null → callback fires later, `weakSelf.get()` returns null, callback no-op. Processor + ClaudeClient remain alive until the plugin instance unloads.

---

## Edge cases and invariants

### Input validation

| Case | Behavior |
|---|---|
| Prompt empty / whitespace | `generateButton` disabled (reduced opacity). |
| Prompt > 1500 chars | "(N / 2000)" counter appears; soft warning, no truncation. Hard ceiling enforced by Anthropic API anyway. |
| API key with leading/trailing whitespace | `apiKeyStore.save()` trims. |
| API key empty saved | Equivalent to clear. statusLabel reads "Cleared." |

### Generate spam

| Case | Behavior |
|---|---|
| Click Generate while loading | `generateButton` disabled during loading → no-op. |
| Click Regenerate while loading | Same — disabled. |
| Edit prompt while loading | Allowed. In-flight uses the previous prompt; new prompt applies on next click. |
| Spam click after enable | ClaudeClient supersedes — older callback dropped silently (A.1 invariant). User pays for both calls. Acceptable for A.2. |

### Concurrent state

| Case | Behavior |
|---|---|
| Click card during loading | Cards hidden during loading; not reachable. |
| Toggle to Settings during loading | Allowed. Spinner stays on `generateButton` (hidden under SettingsBody). When response arrives, GenerateBody's state updates even when not visible. |
| Save different API key during in-flight | In-flight uses old key. New key applies on next Generate. |
| Modal hidden during in-flight callback | Modal updates state silently. Next show reveals it. |

### Apply edge cases

| Case | Behavior |
|---|---|
| Variation has empty `params` map | `patchApplier.apply` runs zero gestures, returns `{applied:0}`. Selection still highlights. |
| Variation has paramId not in APVTS | `report.unknown++`. Aggregated count logged via `juce::Logger::warning`. No banner. |
| Variation has out-of-range value | `report.clamped++`. Silent. |
| Click on empty card slot ("—") | Non-clickable (mouseDown early-return). |

### Threading invariants

- `claudeClient.requestPatches` called only on message thread (jassert in A.1).
- `patchApplier.apply` called only on message thread (jassert in A.1).
- No direct cross-thread access from the modal to ClaudeClient state — only via the callback wrapped in `MessageManager::callAsync` inside ClaudeClient.
- `apiKeyStore.load()` / `.save()` on message thread — file I/O is small (~50 bytes), acceptable.

---

## Tests

### `Tests/AiPromptModalTests.cpp` — 9 blocks

1. `requestGenerate with empty prompt is no-op (transport never called)`
2. `requestGenerate with no API key sets ErrorBanner to MissingApiKey, transport never called`
3. `requestGenerate happy path: 4 valid Variations populate 4 cards` — FakeTransport returns canned 4-block response, expect `getVariationCount() == 4`
4. `requestGenerate with HttpError 401 sets ErrorBanner to "API key rejected by Anthropic"`
5. `requestGenerate with NetworkFailure sets ErrorBanner to "Could not reach Anthropic API"`
6. `requestGenerate with SchemaError sets ErrorBanner to "Claude returned no patches…"`
7. `selectAndApply(i) writes APVTS via gestures` — feed Variation with 2 paramIds, expect 2 begin + 2 end events via APVTS Listener
8. `selectAndApply on a card with empty Variation is no-op` — no APVTS listener triggered
9. `Generate during in-flight is debounced (state == Loading prevents re-entry)`

### `Tests/AiSettingsViewTests.cpp` — 3 blocks

10. `saveApiKey persists to ApiKeyStore (round-trip via load)`
11. `clearApiKey empties the store`
12. `saveApiKey trims whitespace on input`

### `Tests/UIConstructionTests.cpp` — +2 blocks

13. `AiPromptModal constructs and lays out at 1280x720 host bounds`
14. `VariationCard constructs and paints a Variation without crashing`

**Test count delta : +14 blocks (112 → 126 post-A.2).**

### Test seam justification

`AiPromptModal` exposes `requestGenerate`, `selectAndApply`, `enterSettingsView`, `saveApiKey`, `clearApiKey`, `getVariationCount`, `getErrorBannerText`, `isLoading`, `getSelectedCardIndex` as public methods. They are also called from internal click/key handlers, so this is a Component logic boundary, not test-only API. The trade-off (slightly larger public surface) buys us the ability to test the state machine without mouse simulation — the existing `UIConstructionTests` pattern already accepts this kind of seam.

### What we do NOT test

- Pixel-perfect rendering / colors → out (no headless renderer in CI).
- Mouse event simulation on `juce::Button` → out (we use the test seams instead).
- Live Anthropic round-trip from the modal → out (live test stays on `ClaudeClient` directly, A.1 covers it via `Tests/ClaudeClientLiveTests.cpp`).
- Modal animations / transitions → none planned.

### Manual smoke checklist (run once at end of A.2, in Reaper)

1. Open plugin in Reaper, open editor.
2. Click PROMPT → modal appears, synth dimmed.
3. Click gear → Settings view, paste API key, click Save → "Saved." appears.
4. Click gear → back to Generate.
5. Type "warm pad with slow attack", click Generate → spinner → 4 cards appear.
6. Click each card in turn → knobs visibly change in the synth behind.
7. Click REGENERATE → 4 new cards.
8. Edit prompt, click Generate again → new cards.
9. Click ×, reopen → prompt + 4 cards still there.
10. Settings view → click Clear → back to Generate, click Generate → MissingApiKey banner with "Set API key →" → click it → Settings view.

---

## Migration / compat

- **No preset format change.** No APVTS schema change.
- **No new build dependency.** All required pieces shipped in A.1.
- **CMake additive:** plugin target +1 source (`AiPromptModal.cpp`); test target +2 sources, +2 blocks in `UIConstructionTests.cpp`. No removals.
- **A.1 contract preserved.** `ClaudeClient::requestPatches(prompt, N, callback)` and `PatchApplier::apply(Variation)` signatures frozen. A.2 consumes them as-is.
- **A.3 forward compat.** Audition-with-restore (Q3 option C) layers on top by snapshotting `apvts.copyState()` on modal open and `replaceState()` on Esc — no public API change. Same for waveform render and cost telemetry (purely additive UI work).
- **Backwards compat with TopBar.** `topBar.onPromptRequested` callback existed as a stub (Phase 4 planning); A.2 only sets the lambda — no TopBar source change.
- **No commit-tracked secrets.** API key remains in user app data dir per A.1.

---

## Self-review

- **Placeholder scan:** every component has explicit method signatures. Test list has explicit asserts. Error matrix enumerates each `ClaudeClientError` with concrete strings. No "TBD". No "appropriate handling".
- **Internal consistency:**
  - APPLY SELECTION button removed from mockup-derived list because Q3 decision A (click=apply) makes it redundant. Footer shows `[hintLabel] [regenerateButton]` only.
  - Modal owns sub-components by value (or by `std::unique_ptr` for header-only headers that need polymorphism — none currently). Header-only sub-components stored by value match the existing `Source/UI/components/*` pattern.
  - `AiPromptModal` is `addChildComponent`-d (hidden by default) in Editor, then `setVisible(true)` on PROMPT.
- **Scope check:** strictly UI + wiring. No new audio code. No new APVTS params. No new backend logic — all backend behaviors come from A.1. Achievable in ~10 commits (services-as-Processor-members → modal skeleton → Generate body → variation cards → error banner → settings view → wire into Editor → test files → manual smoke).
- **Ambiguity check:**
  - "Click = apply instantly" — locked in Q3 decision A. APPLY SELECTION button removed.
  - "Settings entry" — gear icon in modal header swaps body. Locked in Q4 decision B.
  - "Modal lifetime" — child Component, addChildComponent (hidden), setVisible toggles. Locked in Q5 decision A.
  - "Empty card slot" — paints "—", non-clickable. Locked.
  - "State preservation" — prompt + cards persist across open/close while editor lives. Locked.
  - "Test seams" — `requestGenerate`, `selectAndApply`, etc. public on `AiPromptModal`. Locked. Tests skip mouse simulation.

---

## Execution Handoff

Next step: invoke `superpowers:writing-plans` to produce the task-by-task implementation plan at `docs/superpowers/plans/2026-04-26-ai-layer-a2.md`. The plan will sequence ~10 commits in TDD-friendly order (start with services-on-Processor wiring + UIConstructionTest scaffold for the modal, then sub-components header-by-header, then `AiPromptModal` orchestration, then test files, then `AiPromptModal` ↔ Editor wiring, then manual smoke).

Execution mode (subagent-driven vs inline) is your call after the plan is written. Given size (~10 commits, mostly UI plumbing with one async callback site that needs care) and the past lesson from A.1 (subagents falsely report "linked OK" when build-with-vs.bat skips the test target after a plugin failure — see `feedback_subagent_build_verify.md`), **inline or subagent-with-mtime-check** are both reasonable here. UI work is less subtle than the A.1 ClaudeClient threading, so a subagent run with explicit verify-via-mtime in each task should work fine.
