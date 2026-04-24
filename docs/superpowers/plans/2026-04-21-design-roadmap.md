# SynthVibe Design Implementation — Roadmap

Companion to `2026-04-21-phase1-foundation.md`. Captures task seeds for Phases 2–5 so the executor knows what's coming next; each phase gets its own detailed plan written when Phase 1 lands (since later phases will be shaped by what Phase 1 surfaces — breakpoint API, font-loading quirks, component API conventions, etc.).

**Source of truth:** `docs/ClaudeDesign/` — the HTML mockup, `docs/components.md`, `docs/param-audit.md`, `docs/ai-contract.md`, `docs/preset-format.md`, `docs/responsive.md`, `Source/UI/DesignTokens.h`.

**Palette:** Night Plum (chosen). Accent `#C693E8`.

---

## Phase 2 — Sound Tab

**Goal:** Replace the current `SoundTab` with the mockup layout: OSC 1 / OSC 2 side panels (each with WaveTypeSelect + 4 knobs + oscilloscope), Filter panel with FilterResponseView + pill row + knobs, Amp Env + Filter Env each with an EnvelopeEditor + knob readouts. Remove legacy colour aliases from `LookAndFeel.h`.

**New components (from `components.md`):**
- `Source/UI/components/OscilloscopeView.h` — per-osc live wave display with idle drift
- `Source/UI/components/WaveTypeSelect.h` — waveform picker (dropdown-styled pill)
- `Source/UI/components/FilterResponseView.h` — live cutoff/resonance curve
- `Source/UI/components/FilterTypeSelect.h` — LP24/LP12/HP/BP/Notch pill row
- `Source/UI/components/EnvelopeEditor.h` — ADSR node drag editor
- `Source/UI/components/PanelHeader.h` — shared section-dot + title strip
- `Source/UI/components/KnobTooltip.h` — floating value readout while dragging

**New parameters (per `param-audit.md`):**
- `osc1.phase`, `osc1.pwm`
- `filt.drive`, `filt.keytrack`
- `osc2.phase`, `osc2.pwm`, `osc2.uni.{voices,detune,spread}` (osc2 needs its own unison block)
- New filter type choice entries: add `"LP12"`, `"Notch"` to the `filt.type` choice array

**Migration notes:**
- Replace every `KnobWithLabel` instance in `SoundTab` with `SynthVibe::ArcKnob`.
- Delete `Source/UI/KnobWithLabel.h` once no other tab references it.
- Drop legacy colour aliases (`colOscAccent`, etc.) from `LookAndFeel.h`; inline-replace usages with `SynthVibe::Tokens::osc` etc.
- `SoundTab::resized()` gets a `layoutFor(BP)` helper — Phase 2 can introduce the `enum class BP` scaffold in `PluginEditor` since this is the first tab to need reflow.

**Open questions to close before writing the detailed plan:**
- OscilloscopeView data source: tap `Voice::getNextSample()` output into a ring buffer, or feed from the mixed block in `PluginProcessor`? (Latter is simpler; former needs per-voice scope.)
- EnvelopeEditor interaction: only curves (no per-segment shape) in Phase 2, or also `s`-curve/log support? Spec only shows ADSR.

---

## Phase 3 — Mod / FX / Arp

**Goal:** Replace the current ModTab, FXTab, ArpTab with the mockup's rich components. Add the huge bundle of new parameters (~180) driven by the generic slot scheme.

**New components:**
- `Source/UI/components/ModMatrixTable.h`, `ModRow` (inline), `ModSourcePicker.h`, `ModDestPicker.h`, `BipolarAmountBar.h`, `CurveSelect.h`
- `Source/UI/components/FXChainStrip.h`, `FXSlot.h`, `FXSlotHeader` (inline), `FXVisualization` specialisations per type (Phase 3 delivers ≥ 3 types: drive, delay, reverb; the rest are stubs)
- `Source/UI/components/SegmentedButtonRow.h`, `StepSequencerGrid.h`, `SequencerCell` (inline), `VelocityLane.h`, `PitchLane.h`, `ArpOnOffPill.h`

**New parameters (per `param-audit.md`):**
- Mod Matrix: `mod.1.{src,dst,amount,curve}` … `mod.8.*`, reserving `mod.9..16.*` with IDs declared but UI hidden (so Phase 4+ can expand slot count without preset migration).
- Dest-choice table: a single `kDestinations[]` in `Source/Parameters/ModDestinations.cpp` mapping choice-index → paramId string — the mod engine looks up the target by ID at runtime.
- FX chain: `fx.1.{type,bypass,mix,p1,p2,p3,p4}` … `fx.10.*`. Interim delay/chorus/reverb/drive params from Phase 1 get deprecated — migrate values into `fx.1..4` slot defaults.
- Arp: add `arp.gate`, `arp.swing`, `arp.humanize`, `arp.latch`, and full sequencer `seq.1..16.{gate,pitch,vel,tie}`.

**Engine work (not just UI):**
- `Source/Engine/ModEngine.h/.cpp` — runs the matrix, writes into a parameter bus that `Voice::setParams` consumes.
- `Source/FX/FXChain.cpp` — refactor from the hardcoded delay/chorus/drive/reverb chain to a slot-driven runner that instantiates effects by type-enum.
- `Source/Engine/Sequencer.h/.cpp` — 16-step driver feeding notes into `ArpEngine` / synth directly.

**Risks / unknowns:**
- 180 new params may exceed what JUCE's default APVTS serialises efficiently. Benchmark `apvts.copyState()` on preset save once populated.
- FX reorder via drag-and-drop on `FXChainStrip` — JUCE's DragAndDropContainer is workable but needs a shared coordinate map.

---

## Phase 4 — AI Layer

**Goal:** Implement the Claude API integration exactly as specified in `ai-contract.md`.

**New files:**
- `Source/AI/ClaudeClient.h/.cpp` — HTTP + JSON round-trip via `juce::URL` (requires enabling `JUCE_USE_CURL=1` in CMake, or switching to a minimal HTTP client).
- `Source/AI/PatchApplier.h/.cpp` — consumes Claude's response, writes into APVTS via `beginChangeGesture / setValueNotifyingHost / endChangeGesture`.
- `Source/AI/SystemPrompt.cpp` — the system prompt constant + paramId index generator.
- `Source/AI/ParamIdIndex.h` — table of every paramId with its type, range, and choice-labels, fed to Claude as the allowed-IDs list.
- `Source/UI/components/PromptModal.h`, `PromptInput` (inline), `PromptExampleChips.h`, `VariationGrid.h`, `VariationCard.h`, `HistoryDrawer.h`, `HistorySearch.h`, `HistorySortButtons.h`, `HistoryTagChips.h`, `HistoryItem.h`, `HistoryEmpty` (inline).
- `Source/Presets/HistoryStore.h/.cpp` — reads/writes `~/Documents/SynthVibe/History/` sidecar JSON files per `preset-format.md` (cap 500 entries).

**Settings:**
- API key storage via `juce::PropertiesFile`. Add a Settings modal (or simple alert) triggered when the key is missing.
- Enable `JUCE_USE_CURL=1` in `CMakeLists.txt`; evaluate on Windows (curl is bundled by JUCE but static-linking sometimes needs explicit flag).

**Error-handling matrix** (`ai-contract.md`):
- Network fail → toast + retry
- Non-JSON / parse error → log + "couldn't parse" banner
- Timeout > 20 s → cancel + toast
- Partial / malformed → apply valid keys, warn on the rest
- Missing key → open Settings

**Wiring:**
- In Phase 1 the `TopBar::onPromptRequested` callback was stubbed. Phase 4 wires it to `PromptModal::show()`.
- Extend `preset-format.md`'s `.synthvibe.json` sidecar in `PresetManager::savePreset` — add `prompt`, `tags`, `ai.*` fields.

---

## Phase 5 — Polish

**Goal:** The micro-interactions, responsiveness, accessibility, and quality-of-life items deferred from Phases 1–4.

**Micro-interactions (motion timings already in `Tokens::Motion`):**
- `Source/UI/Animator.h` — timer-based interpolator used by knob tweens, ripple, flash
- ArcKnob tick-flash at round values (14th ms, motion.tickFlashMs)
- Knob double-click reset with flash (500 ms)
- KeyboardRipple at press (900 ms)
- PromptModal apply-tween (400 ms, parameters tween to AI values even though engine has already jumped)
- OscilloscopeView ambient drift (3.2 s period)
- Modulated-knob ring indicator (Phase 4 data hooks but visual lands here)

**Responsive:**
- Implement the three-breakpoint switch from `responsive.md`:
  ```cpp
  enum class BP { Compact, Default, Wide };
  BP current = width < 1150 ? BP::Compact : width < 1550 ? BP::Default : BP::Wide;
  ```
- Each tab exposes `layoutFor(BP)`; `PluginEditor::resized` is the only place that reads window dimensions to pick a BP.
- `juce::FlexBox` for row/column primitives, `juce::Grid` for knob arrays.
- Font-size scale via `getScale()` helper (1.0/0.85/1.15).
- Keyboard octave count changes re-layout entirely — disable mid-resize animation.

**Accessibility (`components.md` checklist):**
- `setTitle()` / `setDescription()` on every control for screen readers
- Keyboard focus order = visual reading order (Z-path)
- Arrow-key nudging on knobs; Home/End → min/max
- Reduce-motion preference disables scope drift, LFO tracer, keyboard ripple

**MIDI learn:**
- Right-click knob → "Learn MIDI CC"; store mapping in APVTS state.

**Inline preset rename:**
- `PresetNameField` gains an edit-on-doubleclick path; writes via `PresetManager::renamePreset`.

**Font embedding:**
- Drop `Resources/Fonts/JetBrainsMono-Regular.ttf`, `Inter-Regular.ttf`, `InstrumentSerif-Regular.ttf` into the repo; add them to `juce_add_binary_data` in CMake; swap `Fonts.h` internals to `Typeface::createSystemTypefaceFor(BinaryData::…)`.

**Top-bar Save/Load resurrection:**
- The Phase 1 rewrite dropped Save/Load buttons. Return them here as a dropdown in `PresetNameField` (Save Copy / Rename / Reveal in Finder) — matches the mockup's contextual preset menu.

---

## How to request the next detailed plan

When Phase 1 is executed and merged, ask:

> "Write the detailed Phase 2 plan."

The author will re-read this roadmap, check which assumptions Phase 1 validated or broke, resolve the open questions listed under Phase 2, and produce `docs/superpowers/plans/YYYY-MM-DD-phase2-sound-tab.md` in the same task-decomposed format as Phase 1.
