# Phase 5 — Arp Polish Design Spec

> **Scope:** flesh out the existing `ArpEngine` + `ArpTab` to match the `param-audit.md` arp specification. Adds 4 behavioral parameters (gate / swing / humanize / latch), 3 new patterns (dnup / asplayed / chord), reorders the pattern + rate enums (preset break, accepted), and rebuilds `ArpTab` around atomized UI components. **Step Sequencer (`seq.N.*`, 64 params, `StepSequencerGrid` UI) is explicitly OUT of scope and shipped as Phase 6.**

**Source of truth:** `docs/ClaudeDesign/docs/param-audit.md` (lines 103–123) for the parameter table.

---

## Goal

A musician picks "Arp" in the tab bar, toggles ON, plays a chord, and gets:

- All 7 patterns selectable (up / down / updn / dnup / rand / asplayed / chord) with audible variation
- All 5 rates (1/4, 1/8, 1/16, 1/16T, 1/32) bpm-locked to host
- **Gate** knob shortens each note (0.05 = staccato, 1.0 = fully filling the step)
- **Swing** knob shifts odd-indexed steps later (0 = straight, 1 = full triplet feel)
- **Humanize** knob jitters per-step velocity + timing (0 = mechanical, 1 = sloppy)
- **Latch** toggle holds notes after key release; pressing a new key replaces the latched chord (Roland convention)

UI atomized to match the project's Phase 2+ component conventions: `PanelHeader`, `ArpOnOffPill`, `SegmentedButtonRow`, `ArcKnob`, no raw colour literals.

---

## What this phase does NOT ship

- **Sequencer** (`seq.1..16.*`, 64 params, `StepSequencerGrid`, `VelocityLane`, `PitchLane`, `SequencerCell`) — Phase 6
- **Sequencer ↔ arp integration** (per-step modulation of arp output) — Phase 6
- **MIDI sync** (clock from external) — host BPM is enough for Phase 5; the engine already reads `bpm` from the processor
- **Per-pattern customisation** (e.g. user-defined arp orderings) — out
- **Note repeats inside a step** (e.g. ratchets) — out
- **Tied notes in arp** (only sequencer has `tie`) — out

---

## Architecture

### Layers

```
APVTS (4 existing arp params + 4 new) → buildArpParams() → ArpEngine::setParams()
                                                          ↓
                                       ArpEngine::process(midi, bpm, sr) — rewrites MIDI in-place
                                                          ↓
                                       SynthEngine consumes the modified MIDI
```

No new files in the engine layer. Just extensions to:
- `Source/Engine/ArpEngine.h` — `Params` struct grows by 4 floats/bools, `Mode` enum grows by 3 values
- `Source/Engine/ArpEngine.cpp` — `process()` gets gate/swing/humanize/latch logic, `buildSequence()` gets dnup/asplayed/chord cases
- `Source/Parameters/ParameterIDs.h` — 4 new constexpr IDs
- `Source/Parameters/ParameterLayout.cpp` — 4 new params + reordered pattern/rate choice arrays
- `Source/PluginProcessor.{h,cpp}` — `buildArpParams()` reads new params
- `Source/UI/ArpTab.h` — full rewrite using new components
- `Source/UI/components/SegmentedButtonRow.h` — new generic component
- `Source/UI/components/ArpOnOffPill.h` — new toggle pill component
- `Tests/ArpEngineTests.cpp` — extensions
- `Tests/ParameterIdMigrationTests.cpp` — defaults + negative legacy test
- `Tests/UIConstructionTests.cpp` — extensions for the 3 new/rewritten UI surfaces
- `CMakeLists.txt` — already lists ArpEngine.cpp and the test files; only header-only additions need no CMake change

---

## Engine semantics

### Parameter additions

| ID | Type | Range | Default | Engine field |
|---|---|---|---|---|
| `arp.gate` | float | 0.05–1.0 | 0.58 | `Params::gate` |
| `arp.swing` | float | 0.0–1.0 | 0.22 | `Params::swing` |
| `arp.humanize` | float | 0.0–1.0 | 0.4 | `Params::humanize` |
| `arp.latch` | bool | — | false | `Params::latch` |

### Existing parameter changes (preset break — accepted, 1.A pattern)

| ID | Old choices / default | New choices / default |
|---|---|---|
| `arp.pattern` | `{Up, Down, UpDown, Random}` default 0 | `{up, down, updn, dnup, rand, asplayed, chord}` default 2 (`updn`) |
| `arp.rate` | `{1/16, 1/8, 1/4, 1/2}` default 0 | `{1/4, 1/8, 1/16, 1/16T, 1/32}` default 2 (`1/16`) |
| `arp.octaves` | int 1–4 default 1 | int 1–4 default 2 |

Old preset with `arp.pattern=3` (was Random) silently loads as `dnup`; with `arp.rate=3` (was 1/2) loads as `1/16T`. Documented break, no migration helper.

### Mode enum mapping

```cpp
enum class Mode {
    Up = 0,         // existing
    Down = 1,       // existing
    UpDown = 2,     // existing (was at index 2, name kept "UpDown" internally; choice label "updn")
    Dnup = 3,       // NEW — descend then ascend
    Random = 4,     // existing (was at index 3 — moves to 4 — preset break)
    AsPlayed = 5,   // NEW — preserve insertion order, no sort
    Chord = 6       // NEW — emit ALL held notes simultaneously per step
};
```

Tests must assert that `static_cast<int>(Mode::Random) == 4` (not 3 like before).

### Algorithm modifications in `ArpEngine::process()`

Current state: `process()` walks samples, increments `sampleCounter`, when it crosses `samplesPerStep(bpm, sr)` emits the next note from `sequence` at `stepIndex`, advances `stepIndex`. NoteOff fires at the next step boundary.

New state — same loop, with these refinements:

```
samplesPerStep = samplesPerStep(bpm, sr)
trigger = stepIndex * samplesPerStep
       + (stepIndex & 1 ? swing * samplesPerStep / 2 : 0)
       + humanize_timing_jitter(stepIndex)        // ±humanize * samplesPerStep / 8
gateOff = trigger + gate * samplesPerStep         // when to emit noteOff for the current note

vel_out  = vel_in * (1 - humanize * uniform(0, 0.5))   // per-step velocity reduction up to 50%
```

`humanize_timing_jitter` and the velocity scale draw from a single `std::mt19937 humanizeRng` member, seeded fixed (`0xC0FFEE`) at `prepare()` so tests are deterministic. Real-world play still varies per step (the RNG advances) but is reproducible across plugin sessions; users won't perceive this as "fake humanization" because each step still differs from the next.

(The existing `rng` field used for the `Random` pattern stays — it can keep its `random_device` seed since Random pattern tests don't assert exact note sequences, only "notes are within the held set".)

### `Chord` pattern

Special: emits **all** held notes' noteOns at the step trigger, all noteOffs at gateOff. Treats the sorted/insertion-ordered `sequence` as one logical step playing in unison. `stepIndex` advances by 1 per step regardless of held-set size.

### `AsPlayed` pattern

`buildSequence` skips the std::sort step that the other patterns use. Insertion order in `heldNotes` (which is already maintained by the existing code's append-on-noteOn) is preserved as the sequence.

### `Latch` behavior

Add a `bool latched[128]` member (or `std::vector<HeldNote> latchedNotes` mirroring `heldNotes`). Behavior:

- `noteOn(n, v)` with `latch=false`: existing behavior (append to heldNotes, rebuild sequence).
- `noteOn(n, v)` with `latch=true`:
  - Clear latchedNotes (i.e. clear the held set).
  - Append (n, v).
  - Rebuild sequence.
  - This is the "new chord replaces latched chord" semantic. Multiple noteOns close in time (e.g. a chord) need to be coalesced → use a "fresh latch group" flag that arms on first noteOn after all keys released, and clears on the first noteOff. Specifically:
    - Track `bool freshLatchGroup = true` initially.
    - On `noteOn`: if `freshLatchGroup`, clear latchedNotes first, set `freshLatchGroup = false`. Then append.
    - On `noteOff`: with `latch=true`, do NOT remove from heldNotes; set `freshLatchGroup = true` (so the next noteOn after a release will start a new chord).
    - With `latch=false`: existing behavior (remove note, also clear latchedNotes).
- `setParams` flipping `latch` from `true` → `false`: clear latchedNotes (notes stop hanging immediately).
- `reset()` clears latchedNotes too.

### Boundary cases

- `humanize=0` → seeded RNG still advances but the `(1 - 0 * uniform)` and `(0 * jitter)` factors yield exactly the input — bit-exact deterministic per setParams call.
- `swing=0` → no shift, identical to current behavior.
- `gate=1.0` → noteOff fires exactly at the next step boundary (= current behavior).
- `gate < smallest float that maps to ≥1 sample` (rare with realistic BPM) → still fire noteOff at least 1 sample after noteOn to avoid 0-length notes.
- `chord` pattern with empty `heldNotes` → no notes emitted (already the existing safeguard).

---

## Tests

### `Tests/ArpEngineTests.cpp` — 9 new test blocks

1. **`gate=0.5 fires noteOff at half-step`** — process 1 step at known sample rate / bpm, scan resulting `juce::MidiBuffer` for noteOff sample position, expect ≈ `samplesPerStep / 2` ± 1 sample.
2. **`gate=1.0 fires noteOff at next step boundary`** — same as legacy behavior, regression check.
3. **`swing=1.0 shifts odd steps by half-step`** — process 4 steps, capture noteOn sample positions, expect even=`{0, 2*sps}`, odd=`{1.5*sps, 3.5*sps}`.
4. **`humanize=0 produces bit-exact deterministic output across two prepare()→process() runs`** — same input → byte-equal MIDI buffer.
5. **`humanize=1 produces velocity in [0.5, 1.0] of input across 32 steps`** — bound check, no exceedances.
6. **`latch=true: noteOff doesn't remove from sequence`** — noteOn(60), noteOff(60) with latch, run 4 steps, expect note 60 still emitted.
7. **`latch=true: new noteOn after release replaces latched chord`** — noteOn(60), noteOff(60), noteOn(64), expect sequence cycles 64 only (not 60 or 60+64).
8. **`dnup pattern with [60, 64, 67] yields 67→64→60→64→67→…`** — 5 steps, assert exact note order.
9. **`asplayed pattern with input order [64, 60, 67] yields 64→60→67→64→…`** — 4 steps, assert exact note order.
10. **`chord pattern with [60, 64, 67] emits 3 simultaneous noteOns per step`** — 1 step, assert 3 noteOns at same sample position with notes {60, 64, 67}.

### `Tests/ParameterIdMigrationTests.cpp` — 4 new assertions

11. **`arp gate/swing/humanize/latch register with audit defaults`** — assert defaults 0.58 / 0.22 / 0.4 / false.
12. **`arp.pattern default index = 2 (updn)`** — APVTS default check.
13. **`arp.rate default index = 2 (1/16)`** — APVTS default check.
14. **`arp.octaves default = 2`** — APVTS default check.

### `Tests/UIConstructionTests.cpp` — 3 new test blocks

15. **`SegmentedButtonRow constructs with N labels and binds to choice param`** — instantiate on `arp.pattern` (7 segments), assert `getNumSegments() == 7`, default selection matches the choice default.
16. **`ArpOnOffPill toggles arp.on via attachment`** — construct, click programmatically (`pill.setToggleState(true, sendNotificationSync)`), assert `apvts.getRawParameterValue(arpEnabled)->load() ≈ 1.0f`.
17. **`ArpTab constructs atomized with all components`** — assert `tab.getStrip()` analogue (e.g. `getPatternRow() != nullptr`, `getRateRow() != nullptr`, `getOnPill() != nullptr`).

Total Phase 5 test additions: **17 blocks** (existing test count after Phase 4 fix = 64 → expected post-Phase 5 = 81).

---

## UI structure

### New components

#### `Source/UI/components/SegmentedButtonRow.h` (~80 LOC, header-only)

Generic segmented pill row bound to a Choice param. Each segment is a flat-styled button; the active segment is filled with `Tokens::accent`, others outline-only. Constructor:

```cpp
SegmentedButtonRow(juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& choiceParamID,
                   const juce::StringArray& labels,
                   juce::uint32 accentColour);
```

Internally holds a `std::vector<juce::TextButton>` (one per label) and a hidden `juce::ComboBox` + `ComboBoxAttachment` that the buttons drive (clicking segment N calls `combo.setSelectedId(N+1, sendNotificationSync)`). This pattern reuses JUCE's APVTS attachment plumbing without inventing a new `ChoiceAttachment` type.

`getNumSegments() const noexcept`, `getSegmentButton(int) noexcept` for tests.

`resized()` lays out segments equally across the row width. No wrapping (V1).

`paint()` draws the rounded outer outline; segment buttons handle their own fill.

#### `Source/UI/components/ArpOnOffPill.h` (~40 LOC, header-only)

Styled bool toggle. Single button with text "ON". When off: outline only, `Tokens::ink3` text. When on: filled with `Tokens::arp` (the section accent), `Tokens::ink` text, optional glow. Bound via `juce::AudioProcessorValueTreeState::ButtonAttachment`. Uses `getCombo()`-style accessor `getButton()` for tests.

### `Source/UI/ArpTab.h` (full rewrite, ~120 LOC)

Replaces the legacy `KnobWithLabel` + `colArpAccent` + bare `juce::ToggleButton` layout with:

```
┌─────────────────────────────────────────┐
│  PanelHeader("ARP", Tokens::arp)        │  ← header strip
├─────────────────────────────────────────┤
│  ArpOnOffPill          [Latch toggle]   │  ← top row (pill on left, latch on right)
├─────────────────────────────────────────┤
│  SegmentedButtonRow (pattern, 7 seg)    │  ← row 2
├─────────────────────────────────────────┤
│  SegmentedButtonRow (rate, 5 seg)       │  ← row 3
├─────────────────────────────────────────┤
│  [Octaves] [Gate] [Swing] [Humanize]    │  ← knob row, 4 ArcKnobs
└─────────────────────────────────────────┘
```

Accessors for tests: `getOnPill()`, `getPatternRow()`, `getRateRow()`, `getOctavesKnob()`.

`paint()` and `resized()` use `Tokens::spaceMd` / `radiusLg` / `panel` etc. — no raw literals, no `colArpAccent`.

The legacy `KnobWithLabel` import is removed; `LookAndFeel.h` references via `colArpAccent` etc. become `Tokens::arp`.

---

## Migration / preset compat

- **`arp.pattern`**: old saved presets with index 3 (was `Random`) silently load as new `dnup`. Old index 2 (`UpDown`) is unchanged (`updn`). No migration helper.
- **`arp.rate`**: old saved indices remap silently to whatever the new index points to (0/1/2/3 → 1/4 / 1/8 / 1/16 / 1/16T). Acceptable.
- **`arp.octaves`**: range unchanged, default changes from 1 to 2 — old presets keep their saved value, new presets start at 2.
- **New params** (gate/swing/humanize/latch): not in old presets, load as default.

Negative test (`Tests/ParameterIdMigrationTests.cpp`): assert that `kModeNames[3] == "dnup"` (or equivalent — the choice array entry at index 3 is the new value, NOT the old `Random`). Documents and locks the break.

---

## Task decomposition

| # | Task | Type | Commit message |
|---|---|---|---|
| 1 | Params: add 4 IDs, register in layout, reorder pattern/rate choices, update defaults, write 4 default-check tests + 1 negative legacy test | params, **breaking** | `feat(params)!: arp gate/swing/humanize/latch + extended pattern/rate enums` |
| 2 | Engine: extend `Mode` enum (Dnup, AsPlayed, Chord), update `buildSequence` cases, 3 new pattern tests | engine | `feat(engine): arp dnup/asplayed/chord patterns` |
| 3 | Engine: gate length (`Params::gate`, noteOff timing logic, gate-vs-step-boundary tests) | engine | `feat(engine): arp gate length controls noteOff timing` |
| 4 | Engine: swing (`Params::swing`, odd-step shift logic, 1 test) | engine | `feat(engine): arp swing shifts odd steps` |
| 5 | Engine: humanize (`Params::humanize`, fixed-seed `humanizeRng`, vel + timing jitter, 2 tests) | engine | `feat(engine): arp humanize via per-step seeded jitter` |
| 6 | Engine: latch (`Params::latch`, `freshLatchGroup` flag, latched-set behavior, 2 tests) | engine | `feat(engine): arp latch holds notes after key release` |
| 7 | Processor: extend `buildArpParams` to read 4 new params, pipe to `ArpEngine::setParams` | wiring | `feat(processor): pipe new arp params to engine` |
| 8 | UI: `SegmentedButtonRow` generic component + 1 test | ui | `feat(ui): SegmentedButtonRow generic choice picker` |
| 9 | UI: `ArpOnOffPill` toggle pill component + 1 test | ui | `feat(ui): ArpOnOffPill styled bool toggle` |
| 10 | UI: `ArpTab` rewrite atomized + 1 test, drop `KnobWithLabel` import + `colArpAccent` literals | ui | `feat(ui): rewrite ArpTab around new components` |
| 11 | Build, deploy.ps1, manual smoke validation in Reaper (all 7 patterns, all 5 rates, latch holds, gate cuts early, swing audible, humanize audible) | manual | n/a |

**Tasks 2–6 can be reordered** (each touches a different chunk of `ArpEngine::process` / `buildSequence`) but the conventional sequence is "easiest visible to engine work first" → patterns → gate → swing → humanize → latch.

**Branch:** `phase5-arp-polish` (created from main after Phase 4 merge).

**No breaking middle:** unlike Phase 4 (where the plugin target was broken between Tasks 1 and 12), every Phase 5 task leaves both targets green. Deploy + audition possible after each commit.

---

## Self-review

- **Placeholder scan:** every step contains a concrete behavior + formula, no TBDs.
- **Internal consistency:**
  - `Mode::Random == 4` is asserted in the engine and in the layout choice array (positions match).
  - `arp.pattern` choice labels listed in spec section "Existing parameter changes" match the `Mode` enum names mapped via the `Mode::Random == 4` reordering.
  - The 4 new params + 4 default-check tests + 1 negative test = 5 test points in Task 1.
- **Scope check:** sequencer is explicitly out (Phase 6 owns 64 `seq.N.*` params + grid UI). Only `arp.*` changes in this phase.
- **Ambiguity check:**
  - "Humanize seeded RNG" — fixed seed at `prepare()`, RNG advances per step (not per `prepare()`); tests reset by calling `prepare()`. Locked.
  - "Latch replaces" — `freshLatchGroup` flag covers chord-input vs single-note-input ambiguity. Locked.
  - "Gate min" — for very small gate × very low BPM, ensure noteOff is at least 1 sample after noteOn. Locked.
- **Out-of-scope items declared:** sequencer, MIDI sync, ratchets, ties, custom orderings.

---

## Execution Handoff

When ready, the next step is to invoke `superpowers:writing-plans` to produce the detailed task-by-task implementation plan at `docs/superpowers/plans/2026-04-26-phase5-arp-polish.md`. That plan will have the verbatim code edits for each of the 11 tasks (in the same format as `2026-04-25-phase4-fx-slots.md`).

After the plan is written, execution mode (subagent-driven vs inline) is your call.
