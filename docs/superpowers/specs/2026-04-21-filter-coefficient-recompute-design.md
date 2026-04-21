# Filter Coefficient Sub-Rate Update — Design Spec

**Date:** 2026-04-21
**Issue:** I-2 from 2026-04-20 code review
**Goal:** Reduce per-sample `std::tan` cost of filter coefficient updates by moving coefficient application from audio rate to control rate inside `Voice::getNextSample()`.

---

## Problem

`juce::dsp::StateVariableTPTFilter::setCutoffFrequency()` calls `std::tan(π · cutoff / sampleRate)` every time it is invoked. `Voice::getNextSample()` currently calls `filter.setCutoff()` and `filterR.setCutoff()` on every sample whenever `hasFilterMod || smoothCutoff.isSmoothing()` is true — so on any patch with filter-envelope amount non-zero, any LFO routed to Filter, or during the 5 ms ramp after any cutoff knob move.

With 8 active voices × 2 filters × 48 kHz, worst case is ~768 000 transcendental calls/sec.

The modulators driving the cutoff (envelope, LFO, parameter smoother) are themselves smooth and well below audio rate, so per-sample coefficient updates are wasteful.

---

## Approach

Apply filter coefficients at **control rate — every 16 samples** — inside `Voice::getNextSample()`. The outer `hasFilterMod || smoothCutoff.isSmoothing()` guard is kept, so nothing runs when modulation and smoother are both idle.

16 samples at 48 kHz = 0.33 ms control period ≈ 3 kHz control rate — well above any envelope or LFO frequency used musically.

Cost drops to ~48 000 `std::tan` calls/sec (~94 % reduction).

The sub-rate boundary lives in Voice, not Filter. `Filter` stays a deterministic wrapper; Voice owns the modulation math and therefore owns the decision of how often to push coefficients.

---

## Voice changes

**File:** `Source/Engine/Voice.h` / `Voice.cpp`

Add two private members to `Voice.h`:

```cpp
static constexpr int FilterCoefUpdateRate = 16;   // power of 2 → cheap bitmask
int filterCoefCounter = 0;
```

Reset `filterCoefCounter = 0` in:
- `Voice::prepare()` — hygiene at construction
- `Voice::noteOn()` — so the first sample of a new note applies fresh coefficients

Rewrite the cutoff-apply block in `Voice::getNextSample()`. The current code:

```cpp
const float envMod = fltEnv.getNextSample();
const bool hasFilterMod = ...;
if (hasFilterMod || smoothCutoff.isSmoothing())
{
    float cutoff = sCutoff * (1.f + params.filterEnvAmt * envMod);
    cutoff += (params.lfo1.dest == LfoDest::Filter ? l1 * 4000.f : 0.f);
    cutoff += (params.lfo2.dest == LfoDest::Filter ? l2 * 4000.f : 0.f);
    cutoff = juce::jlimit(20.f, 20000.f, cutoff);
    filter.setCutoff(cutoff);
    filterR.setCutoff(cutoff);
}
```

becomes:

```cpp
const float envMod = fltEnv.getNextSample();
const bool hasFilterMod = ...;
if ((hasFilterMod || smoothCutoff.isSmoothing()) && filterCoefCounter == 0)
{
    float cutoff = sCutoff * (1.f + params.filterEnvAmt * envMod);
    cutoff += (params.lfo1.dest == LfoDest::Filter ? l1 * 4000.f : 0.f);
    cutoff += (params.lfo2.dest == LfoDest::Filter ? l2 * 4000.f : 0.f);
    cutoff = juce::jlimit(20.f, 20000.f, cutoff);
    filter.setCutoff(cutoff);
    filterR.setCutoff(cutoff);
}
filterCoefCounter = (filterCoefCounter + 1) & (FilterCoefUpdateRate - 1);
```

The counter is advanced every sample the voice produces output (past the `ampEnv.isActive()` early return), regardless of whether the guard body ran. The 1-in-16 cadence is therefore deterministic for any active voice; idle voices neither accrue counter state nor do coefficient work. `noteOn()` resets the counter so the first sample of a new note starts the cadence fresh.

---

## What is NOT changed

- `Filter` class internals — stays a deterministic wrapper with no hidden throttling.
- The resonance path — already gated by `smoothResonance.isSmoothing()` and only recomputes when the resonance knob actually moves; `svf.setResonance()` is a cheap divide, not a transcendental.
- The outer `hasFilterMod || smoothCutoff.isSmoothing()` guard — still skips the whole modulation/apply block when everything is idle, so steady-state voices do zero coefficient work.
- The modulators themselves — envelope, LFOs, and smoother still advance every sample.

---

## Data flow

| Step | Every sample | Every 16th sample (when guarded) |
|---|---|---|
| smoother advances | ✓ | |
| envelope advances (`fltEnv.getNextSample()`) | ✓ | |
| LFOs advance | ✓ | |
| counter increments | ✓ | |
| modulated cutoff computed | | ✓ |
| `filter.setCutoff` / `filterR.setCutoff` called | | ✓ |
| `filter.processSample` / `filterR.processSample` | ✓ | |

---

## Files changed

| File | Change |
|---|---|
| `Source/Engine/Voice.h` | Add `FilterCoefUpdateRate` constant and `filterCoefCounter` member |
| `Source/Engine/Voice.cpp` | `prepare()` and `noteOn()` reset counter; `getNextSample()` gates coefficient apply on counter |

No changes to `Filter.h` / `Filter.cpp`.

---

## Testing

- **Existing `AISynthTests` must still pass** — `VoiceTests`, `SynthEngineTests`, `ArpEngineTests`. No existing test asserts on per-sample coefficient identity, so sub-rate updates should not regress any assertion.
- **Manual audio check** — load a patch with `filterEnvAmt = 1.0` and/or LFO routed to Filter, play notes across the keyboard, confirm the filter sweep sounds identical to the audio-rate version (16-sample quantization ≈ 0.33 ms is well below any audible threshold for envelope/LFO-driven modulation).
- **No new unit test** — there is no clean, non-fragile way to assert "stair-stepped coefficients are inaudible." Regressions in the cutoff math itself are already covered by existing filter-sweep behavior in `VoiceTests`.

---

## Risks

- **Stair-stepping audible under extreme modulation?** At 3 kHz control rate with musical modulators (LFOs up to ~30 Hz, envelope attacks down to ~1 ms), the modulation signal has negligible spectral content above Nyquist/2 of the control rate. If a future audio-rate modulation source is added (e.g., ring-mod-style FM on cutoff), sub-rate update will need to be revisited or bypassed for that path.
- **First-sample latency after `noteOn`** — mitigated by resetting the counter to 0 on `noteOn()`, so the first sample of a new note triggers an immediate coefficient application when modulation is active.
- **Counter drift under voice steal** — `noteOn(stolen=true)` still resets the counter; coefficients re-apply immediately on the first sample of the stolen voice's new note.
