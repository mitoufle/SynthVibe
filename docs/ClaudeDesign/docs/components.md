# SynthVibe — Component Handoff

Complete inventory of every custom component in the design. Each row names a JUCE class to build, the mockup section it comes from, and its required visual states.

---

## Top bar

| Component | Source class | Mockup | States |
|---|---|---|---|
| `BrandBadge` | `components/BrandBadge.h` | top-left logo + "AISynth" wordmark | default, dim (when modal open) |
| `PresetNameField` | `components/PresetNameField.h` | top-center preset title + timestamp meta | default, hovering, editing (inline rename), dirty (unsaved `●`) |
| `PromptButton` | `components/PromptButton.h` | `✦ prompt` gold pill | default, hover, pressed, generating (shimmer) |
| `PrevNextArrows` | `components/HistoryArrows.h` | ◀ ▶ arrows cycling history | default, disabled (ends of list) |
| `MasterVolKnob` | variant of `ArcKnob` at smaller size | vol mini-knob | inherits knob states |

## Tab bar

| Component | Source | Mockup | States |
|---|---|---|---|
| `TabBar` | `components/TabBar.h` | Sound / Mod / FX / Arp | default per tab, active, hover |
| `HistoryToggleButton` | `components/HistoryToggleButton.h` | rightmost "History" button + count badge | default, hover, active (drawer open) |

## Knobs + labels

| Component | Source | Mockup | States |
|---|---|---|---|
| `ArcKnob` | `components/ArcKnob.h` + custom `LookAndFeel::drawRotarySlider` | all knob cells | default, hover, dragging (scaled + glow), tick-flash (at round values), reset-flash (double-click), disabled, modulated (live indicator ring — phase 4) |
| `KnobLabel` | inline in `ArcKnob` | all knob labels | default, highlighted (during drag) |
| `KnobValueText` | inline | all `.kv` spans | default, pop (animates when changed) |
| `KnobTooltip` | `components/KnobTooltip.h` | floating value while dragging | hidden, visible (with arc-color accent) |

## OSC panels

| Component | Source | Mockup | States |
|---|---|---|---|
| `OscilloscopeView` | `components/OscilloscopeView.h` | OSC 1 / OSC 2 wave display | idle-flow (ambient drift), animated (when playing), hover |
| `WaveTypeSelect` | `components/WaveTypeSelect.h` | `saw ▾` dropdown | default, open, hover |
| `PanelHeader` | shared | all panel heads | default (with section-dot color) |

## Filter panel

| Component | Source | Mockup | States |
|---|---|---|---|
| `FilterResponseView` | `components/FilterResponseView.h` | live filter curve | default, dragging (when cutoff being edited), resonance-highlight |
| `FilterTypeSelect` | `components/FilterTypeSelect.h` | LP24 / LP12 / HP / etc pill row | default, active per-type, hover |

## Envelopes

| Component | Source | Mockup | States |
|---|---|---|---|
| `EnvelopeEditor` | `components/EnvelopeEditor.h` | Amp Env + Filter Env | default, node hover (glow), node dragging, playing (scrubs visually during note) |
| `EnvelopeReadout` | inline in editor | `a 0.5ms · d 120ms · ...` | default |

## Modulation Matrix

| Component | Source | Mockup | States |
|---|---|---|---|
| `ModMatrixTable` | `components/ModMatrixTable.h` | the 8-slot list | default, scrolled (fade on top/bottom) |
| `ModRow` | inline | each row | default, hover, empty (ghost / dashed) |
| `ModSourcePicker` | dropdown | `LFO 1 / Env 2 / ...` pills | default, open, source-color-coded (lfo=violet, env=green, vel=gold) |
| `ModDestPicker` | dropdown | destination pill | default, open (shows hierarchical menu: Osc 1 → ▸ cutoff, ...) |
| `BipolarAmountBar` | `components/BipolarAmountBar.h` | the +/- bar | default, dragging, positive (arc color), negative (filter color) |
| `CurveSelect` | `components/CurveSelect.h` | lin / exp / log | default, open |

## FX Chain

| Component | Source | Mockup | States |
|---|---|---|---|
| `FXChainStrip` | `components/FXChainStrip.h` | horizontal chain of slots | default, reordering (drag ghost), scrolled (if > ~6 slots fit) |
| `FXSlot` | `components/FXSlot.h` | each effect card | default, hover, active (focused), bypassed (dim), empty (dashed ghost) |
| `FXSlotHeader` | inline | number + name + enable dot | default, hover, dragging |
| `FXVisualization` | variable per-type | mini wave / impulse response / etc | one impl per effect type |

## Arp / Sequencer

| Component | Source | Mockup | States |
|---|---|---|---|
| `SegmentedButtonRow` | `components/SegmentedButtonRow.h` | pattern / rate / octave pickers | default per-button, active, hover |
| `StepSequencerGrid` | `components/StepSequencerGrid.h` | 16-step × 4-lane grid | default, cell-hovered, cell-active, cell-playhead (glow) |
| `SequencerCell` | inline | single cell | off, on, tied, playhead, painting (drag across to toggle multiple) |
| `VelocityLane` / `PitchLane` | specializations | the filled bars | default, dragging (changes height) |
| `ArpOnOffPill` | `components/ArpOnOffPill.h` | `ON` badge | off, on (glowing) |

## Keyboard

| Component | Source | Mockup | States |
|---|---|---|---|
| `Keyboard` | `components/Keyboard.h` | bottom keys | default, playing-external (MIDI in highlights keys), playing-user-click |
| `KeyboardKey` | inline | white or black key | default, hover, playing (accent gradient + glow), MIDI-highlighted (dim accent) |
| `KeyboardRipple` | transient | ripple at top edge on press | animating (900ms), then removed |

## AI Modal

| Component | Source | Mockup | States |
|---|---|---|---|
| `PromptModal` | `components/PromptModal.h` | the full pull-down | closed, opening (transform), open, closing, generating |
| `PromptInput` | inline | text field | default, focused, typing, submitted |
| `PromptExampleChips` | inline | suggestion pills below input | default, hover |
| `VariationGrid` | `components/VariationGrid.h` | the 4-card grid | empty, thinking (shimmer), revealed (stroke-draw waves), selected |
| `VariationCard` | `components/VariationCard.h` | single variation | thinking, revealing, default, hover, selected |

## History Drawer

| Component | Source | Mockup | States |
|---|---|---|---|
| `HistoryDrawer` | `components/HistoryDrawer.h` | right-side slide-out | closed, opening, open, closing |
| `HistorySearch` | `components/HistorySearch.h` | search input + icon | empty, focused, has-text |
| `HistorySortButtons` | `components/HistorySortButtons.h` | recent / name / tag | default, active-sort |
| `HistoryTagChips` | `components/HistoryTagChips.h` | tag filter pills | default, active, hover |
| `HistoryItem` | `components/HistoryItem.h` | single entry card | default, hover, current (gold border), search-matched (highlights) |
| `HistoryEmpty` | inline | no-results state | hidden, visible |

## Status bar

| Component | Source | Mockup | States |
|---|---|---|---|
| `VoiceMeter` | `components/VoiceMeter.h` | voice LEDs | 0..8 lit; LED states: off, lit, lit-breathing |
| `StatusReadout` | `components/StatusReadout.h` | `4 / 8 · 44.1kHz · 0.2% cpu` | default, cpu-warn (>60%, amber), cpu-crit (>90%, red) |

## Shared / utility

| Component | Source | Purpose |
|---|---|---|
| `ArcKnobLookAndFeel` | `ui/ArcKnobLookAndFeel.h` | drawRotarySlider implementation for all knobs |
| `DesignTokens` | `ui/DesignTokens.h` | colors, spacing, radii, font sizes as constants |
| `Fonts` | `ui/Fonts.h` | loads JetBrains Mono, Inter, Instrument Serif at startup |
| `Animator` | `ui/Animator.h` | timer-based interpolation used by knob tweens, ripple, etc |
| `TooltipManager` | `ui/TooltipManager.h` | singleton for the floating knob tooltip |

---

## State color legend (for visuals)

- **Default** — the main static look
- **Hover** — mouse-over
- **Active / Pressed** — mouse down or keyboard activation
- **Focused** — keyboard focus ring
- **Disabled** — dim + no pointer events
- **Dragging** — mid-interaction feedback (scale, glow)
- **Modulated** — a ring around a knob indicating live modulation (roadmap, phase 4)

## Accessibility checklist

- [ ] Every control has a `setTitle()` / `setDescription()` for screen readers
- [ ] Keyboard focus order matches visual reading order (Z-path)
- [ ] Color contrast ≥ 4.5:1 for text on panel backgrounds
- [ ] Knob values readable via arrow keys + Home/End for min/max
- [ ] Reduce-motion respected: disable scope drift, LFO tracer, ripple
- [ ] Tooltips never the ONLY source of info for a label
