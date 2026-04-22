# Phase 2b — Sound Tab Parameter Completion Design

**Status:** Approved 2026-04-22
**Baseline:** `phase2-sound-tab` tag (commit `3b17ccc`)
**Scope:** single phase, full engine implementation
**Author:** François Lami (design assist: Claude)

---

## 1. Goal

Ship the parameters the original Phase 2 roadmap called for but that Phase 2 did not deliver: starting phase and pulse width on both oscillators, per-instance unison on OSC2, a pre-filter drive stage, bipolar keytracking, and the two filter types (LP24, Notch) the filter section was always meant to expose. Also reorder `filt.type` into a coherent LP12→LP24→HP→BP→Notch layout, delete dead legacy colour aliases, and rename a set of C++ constants whose names imply a global unison scope that no longer exists.

This is a cleanup phase, not a feature expansion: every item here was either promised by an earlier spec or implied by an existing code path. The plugin version stays 1.0 because we have no installed user base — Phase 2b deliberately breaks existing local preset files on the `filt.type` reorder, with no migration.

## 2. Architecture

**Parameters:** nine new APVTS parameters registered in `ParameterLayout.cpp` and `ParameterIDs.h`. One existing choice (`filt.type`) is re-defined (options reordered and extended). No other existing parameter is touched.

**Engine:** `Oscillator` gains `setStartingPhase(float degrees)` and `setPulseWidth(float duty)`. PolyBLEP is reworked to handle variable pulse width on Square (the highest-risk piece of work in this phase). `Filter` cascades two `StateVariableTPTFilter` instances to synthesise LP24 (two LP12s in series), uses the existing SVF bandpass tap with `input - 2·bandpass` for Notch, applies a tanh pre-filter soft-clipper driven by `filt.drive`, and folds a keytrack cutoff multiplier into the existing sub-rate coefficient apply block (every 16 samples) so it costs zero extra per-sample.

**UI:** `SoundTab` reflows into a 3×3 knob grid per oscillator panel (pitch row / shape + level row / unison row). The right column reproportions slightly (Filter 34%, AMP 33%, FLT 33%) to fit the two new filter knobs. `FilterTypeSelect` grows from three to five pills. `FilterResponseView` gains analytic curves for LP24 and Notch.

**Tech stack:** JUCE 7.0.9 (`AudioProcessorValueTreeState`, `AudioParameterFloat`, `AudioParameterChoice`, `ParameterAttachment`, `dsp::StateVariableTPTFilter`), the existing SynthVibe `Tokens`/`BP` design-system modules, and the exponential ADSR from Phase 2.

## 3. New Parameters

All IDs are dot-separated per the existing contract. Ranges use `NormalisableRange<float>`; defaults are chosen so a preset saved with defaults produces the same sound as a Phase 2-era preset.

| ID | Type | Range | Default | Label |
|---|---|---|---|---|
| `osc1.phase` | Float | 0 – 360° (step 1°) | 0° | Osc1 Phase |
| `osc1.pwm` | Float | 0.01 – 0.99 (step 0.001) | 0.50 | Osc1 PWM |
| `osc2.phase` | Float | 0 – 360° (step 1°) | 0° | Osc2 Phase |
| `osc2.pwm` | Float | 0.01 – 0.99 (step 0.001) | 0.50 | Osc2 PWM |
| `osc2.uni.voices` | Int | 1 – 7 | 1 | Osc2 Unison Voices |
| `osc2.uni.detune` | Float | 0 – 100 cents (step 0.1) | 0 | Osc2 Unison Detune |
| `osc2.uni.spread` | Float | 0 – 1 (step 0.01) | 0.5 | Osc2 Unison Spread |
| `filt.drive` | Float | 0 – 1 (step 0.001) | 0 | Filter Drive |
| `filt.keytrack` | Float | −1 – +1 (step 0.001, bipolar) | 0 | Filter Keytrack |

**Design notes on each:**

- **`osc1.phase` / `osc2.phase`** — starting phase captured at note-on, not a per-sample offset. `Voice::noteOn` resets each oscillator's phase accumulator to `startingPhase/360°`. Free-running oscillators do not read the parameter after the note starts.
- **`osc1.pwm` / `osc2.pwm`** — duty cycle for Square waves. Range clamped to `[0.01, 0.99]` to keep PolyBLEP stable; values outside that window degenerate the waveform. On non-Square waves the knob is greyed and ignored (engine-level: `Oscillator::getNextSample()` only reads PWM in the `Square` case).
- **`osc2.uni.voices`** — range matches the existing `osc1.uni.voices` (1 – 7) for visual parity in the UI. `osc2.uni` semantics match OSC1 exactly; the existing `UnisonOscillator` engine class is already per-Voice-per-instance, so the work is wiring parameters into `Voice`, not refactoring the engine.
- **`filt.drive`** — pre-filter soft-clip, `input = tanh(input · (1 + drive · 9))`. At `drive = 0` this is a no-op; at `drive = 1` the pre-gain is ×10 (~20 dB) and the signal is fully into tanh saturation before the filter resonates the harmonics. Lives **before** the SVF, complementing the existing post-chorus `fx.drive.*` stage (Serum-style two-drive model: pre-filter for resonance interaction, post-FX for master colour).
- **`filt.keytrack`** — bipolar, 0 = no tracking, +1 = cutoff follows note pitch 1:1 (one octave up per octave played), −1 = inverse. The multiplier `exp2((midiNote − 60)/12 · keytrack)` folds into the existing sub-rate cutoff apply (every 16 samples) — see §5.

## 4. Filter Type Reorder (Breaking Change)

`filt.type` is currently `{ "Low Pass", "High Pass", "Band Pass" }` with default 0. It becomes:

```
{ "LP12", "LP24", "HP", "BP", "Notch" }
```

Default 0 (LP12). The existing "Low Pass" was already 12 dB/oct — the SVF is a second-order filter — so index 0's sonic behaviour is unchanged; only the label changes.

**Preset impact:** a Phase 2-era preset that stored index 1 ("High Pass") will now resolve to LP24; index 2 ("Band Pass") will resolve to HP. We accept the break: there is no installed user base, local dev presets will be regenerated, and adding migration code for a five-user problem is cost-for-nothing. No `filt.type.v2` or version bump.

**Rollback path:** if a preset break turns out to be worse than expected, revert this section alone by restoring the three-option StringArray — engine code for LP24/Notch lives in new branches of the type switch and stays inert if the parameter can never reach those values.

## 5. Engine Changes

### 5.1 Oscillator (`Source/Engine/Oscillator.{h,cpp}`)

Add:
- `void setStartingPhase(float degrees)` — stores phase in `[0, 1)`. Does not touch the running accumulator.
- `void setPulseWidth(float duty)` — clamps to `[0.01, 0.99]`. Used only by `Square`.
- `void resetPhaseToStart()` — sets the accumulator to the stored starting phase. Called by `Voice::noteOn` before any sample is produced.

Modify Square path: the naive square becomes `phase < pulseWidth ? +1 : −1`. The PolyBLEP correction currently compensates two discontinuities at `phase ≈ 0` and `phase ≈ 0.5`; both move: one stays at 0, the other moves to `pulseWidth`. The corrections must be re-derived for the variable-duty case. This is the highest-risk item in Phase 2b — allocate a plan task purely to re-derive and test PolyBLEP with three duty cycles (0.10, 0.50, 0.90).

### 5.2 Filter (`Source/Engine/Filter.{h,cpp}`)

The current `Filter` holds a single `juce::dsp::StateVariableTPTFilter<float>` switched between LP/HP/BP. The rewrite:

- Holds **two** SVF instances (`svf1`, `svf2`). For LP12/HP/BP/Notch only `svf1` is active; for LP24 both are driven in series, each configured as LP with the same cutoff/Q.
- Notch is synthesised as `input − 2 · bandpass_output`. This is the algebraic identity for an SVF (`LP + HP = input`, `LP − HP = input − 2·HP`, and bandpass is the tap between the two integrators); it lets us reuse the existing SVF without adding a dedicated notch primitive.
- The per-sample process becomes:
  1. If `filt.drive > 0`, `x = tanh(x · (1 + drive · 9))`.
  2. Apply `svf1` (and `svf2` if LP24).
  3. If type == Notch, `y = x_dry_after_drive − 2 · svf1.bandpassOutput`.
- The keytrack multiplier (computed once per note-on from `effective_cutoff = cutoff_hz · exp2((midiNote − 60)/12 · keytrack)`) is baked into the cutoff value written to the SVF inside the existing `filterCoefCounter` sub-rate block in `Voice::getNextSample()` (see `docs/superpowers/specs/2026-04-21-filter-coefficient-recompute-design.md`). The counter runs every 16 samples; folding the multiplier in adds one `exp2` every 16 samples per voice, which is ~62 Hz of additional compute per voice — negligible.

### 5.3 Voice (`Source/Engine/Voice.{h,cpp}`)

- `Voice::noteOn(midi, velocity)` calls `osc1.resetPhaseToStart()` and `osc2.resetPhaseToStart()` before the first sample. Captures `currentMidiNote` for the keytrack multiplier.
- `Voice::prepare` and parameter push points already exist for OSC1 unison; extend them to write `osc2.uni.*` into the OSC2 `UnisonOscillator` instance. No new engine class.
- PWM is written into both oscillators every parameter push (smoothed values); the oscillator ignores PWM on non-Square waves, so no branching needed at the voice level.

## 6. UI Changes

### 6.1 SoundTab grid

Each oscillator panel places the Wave selector (combo) across the top of the panel, with a 3×3 knob grid below it:

| Row | Col 1 | Col 2 | Col 3 |
|---|---|---|---|
| Pitch | Octave | Semitone | Detune |
| Shape | Phase | PWM | Level |
| Unison | Voices | Detune | Spread |

Nine knobs fit the 3×3 exactly. The Wave selector stays outside the grid (it is a combo, not a knob, and needs wider horizontal space). "Unison Detune" in row 3 is distinct from "Detune" in row 1 — row 1's Detune is coarse pitch fine-tune (`osc*.fine`, ±100¢), row 3's is unison spread detune (`osc*.uni.detune`, 0–100¢). The panel header labels disambiguate visually ("Detune" vs "Uni Detune").

### 6.2 FilterTypeSelect

Grows from three pills to five, same pill pattern, same radio-group binding. Pill labels: `LP12 LP24 HP BP NO` (the last abbreviation fits the existing pill width better than "Notch"; the tooltip spells out "Notch"). The existing `FilterTypeSelect` implementation is already a radio-group `TextButton` array attached via `juce::ParameterAttachment` — growing the array from 3 to 5 is a local change in the component.

### 6.3 FilterResponseView

The existing response view analytically renders LP/HP/BP shapes. Add:
- **LP24** — same transfer function as LP12 but squared (two cascaded LP12s); visually a steeper rolloff at the cutoff.
- **Notch** — zero at the cutoff, unity gain elsewhere; Q controls notch width.

Use analytic curves, not runtime FFTs — the existing LP/HP/BP curves are already analytic and the code pattern generalises.

### 6.4 PWM knob enable-state

The PWM knob greys out when the wave selector is not on Square. Implementation: a `juce::ParameterAttachment` on `osc1.wave` / `osc2.wave` whose callback flips `pwmKnob.setEnabled(waveIndex == 2)` and repaints. No engine-side change (the engine already ignores PWM for non-Square).

### 6.5 Right-column reproportion

Column 3 currently splits Filter / AMP / FLT 40 / 30 / 30. It becomes 34 / 33 / 33 to give the Filter panel room for the two new knobs (`filt.drive`, `filt.keytrack`). The split happens in `SoundTab::resized()`; no design-token change.

## 7. Legacy Cleanup

**Colour aliases to delete from `Source/UI/LookAndFeel.h`:**

```
colOscAccent     // duplicates Tokens::osc
colFilterAccent  // duplicates Tokens::filter
colEnvAccent     // duplicates Tokens::env
colMasterAccent  // duplicates a Tokens equivalent (panel/master)
```

These aliases survived the Phase 1 token migration because we were nervous about missing a use site. A grep pass at plan-writing time confirms they're dead; deleting them is a one-line cleanup with no behavioural change.

**C++ constant renames in `Source/Parameters/ParameterIDs.h`:**

```
unisonVoices       → osc1UnisonVoices
unisonDetune       → osc1UnisonDetune
unisonStereoSpread → osc1UnisonSpread
```

The **string values** (`"osc1.uni.voices"` etc.) stay identical — presets bind by string, so this rename is invisible to stored state. The new `osc2.uni.*` constants follow the same pattern. This is a find-and-replace across the project and is safe to do in one commit.

## 8. Testing

### 8.1 Automated (Tests/)

Extend the existing `ParameterRegistrationTests` and `UIConstructionTests`, and add DSP cases:

- **ParameterRegistrationTests** — assert all nine new parameters are registered with correct ranges and defaults.
- **UIConstructionTests** — construct the new `FilterTypeSelect` (5 pills), construct `SoundTab` with the 3×3 grid, verify the PWM attachment initialises to disabled when the wave default is not Square.
- **Filter DSP tests:**
  - LP24 rolloff measured at cutoff + 1 octave is approximately −24 dB relative to pass-band (tolerance ±3 dB).
  - Notch at cutoff attenuates a pure tone to < −30 dB; the tone one octave away is attenuated < 1 dB.
  - `filt.drive` at 0.75 on a 1 kHz sine produces a THD reading strictly greater than the same signal at `filt.drive = 0` (relative comparison, not an absolute dB threshold).
  - `filt.keytrack = 1.0` — a note one octave above MIDI 60 produces an effective cutoff 2× the parameter value.
- **Oscillator DSP tests:**
  - `setStartingPhase(180°)` followed by the first sample of a Sine produces ≈ 0 (sine of π). Tolerance ±0.01.
  - Square with `pulseWidth = 0.25` has a positive-sample fraction close to 0.25 over a window of 10 cycles (tolerance ±5%).

### 8.2 Manual DAW smoke test

A nine-item checklist pinned to the plan:

1. Plugin loads in Reaper with the Phase 2 preset reference; no crashes.
2. OSC1 phase = 90° on Sine audibly delays the attack transient (quick A/B vs phase = 0).
3. OSC1 PWM sweep on Square produces the classic narrow-to-wide pulse sound; no clicks at extremes.
4. OSC2 unison voices = 5, detune = 25¢, spread = 0.8 — stereo field opens, supersaw character present.
5. Filter type = LP24 is audibly steeper than LP12 on the same cutoff + resonance (sweep both types with a saw).
6. Filter type = Notch audibly carves a hole in a saw pad at cutoff.
7. `filt.drive` at 0.7 thickens a plucky bass without volume ballooning (pre-filter drive + filter together).
8. `filt.keytrack = +1` — playing two octaves of notes, the filter opens with pitch; no artifacts.
9. All new parameters automate from the DAW host (wiggle each in Reaper's envelope lane).

## 9. Rollback

Branch off the `phase2-sound-tab` tag. If Phase 2b proves unstable we can abandon the branch and ship the Phase 2 tag as-is. Within Phase 2b, the riskiest isolated piece is PolyBLEP-with-variable-PWM; if that derivation blocks the phase, we ship `osc*.pwm` as a parameter that only takes effect on Square **with aliasing** and file a follow-up task to re-derive the PolyBLEP correction (acceptable tradeoff since the PWM sweep is usually automated slowly).

## 10. Estimate

- 9 parameters registered + 1 parameter redefined (small).
- Engine work: oscillator PWM/phase (~3 h including PolyBLEP derivation), filter LP24/Notch/drive/keytrack (~4 h).
- UI work: 3×3 grid reflow, 5-pill FilterTypeSelect, response-view shapes, PWM enable-state, reproportion (~3 h).
- Tests: parameter + UI + DSP (~2 h).
- Cleanup: legacy aliases + constant rename (~0.5 h).

**Total estimate: 12 – 15 hours across ~15 plan tasks.** The implementation plan (written next, via the `writing-plans` skill) breaks this into TDD-shaped tasks and orders them so the PolyBLEP derivation comes early — if it blows up, we find out before the UI work is committed.

---
