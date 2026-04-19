# Unison Engine — Design Spec

**Date:** 2026-04-19
**Status:** Approved
**Scope:** Step 1 of sound enrichment roadmap (Approach A — Supersaw First)

---

## Context

SynthVibe currently produces thin sounds because each voice uses a single oscillator instance per osc slot. The goal is to add a unison engine that stacks multiple detuned copies of each oscillator per voice, producing the dense, wide sound characteristic of Serum/Massive/Sylenth1.

This is step 1 of 3:
1. **Unison engine** ← this spec
2. Reverb FX
3. Filter drive / saturation

---

## Architecture

A new `UnisonOscillator` class wraps up to 7 `Oscillator` instances. It replaces the two bare `Oscillator` members in `Voice`, keeping `getNextSample()` returning a single `float` — no changes to `Voice`'s interface with `SynthEngine`.

```
Voice
  ├─ UnisonOscillator osc1   (1..7 × Oscillator)
  ├─ UnisonOscillator osc2   (1..7 × Oscillator)
  ├─ Envelope ampEnv          (unchanged)
  ├─ Envelope fltEnv          (unchanged)
  └─ Filter filter            (unchanged)
```

Stereo spread is deferred to step 2 (when Voice output becomes stereo).

---

## New Class: `UnisonOscillator`

**File:** `Source/Engine/UnisonOscillator.h` / `.cpp`

```cpp
class UnisonOscillator {
public:
    static constexpr int MaxUnison = 7;

    void setSampleRate(double sr);
    void setFrequency(double hz);
    void setWaveform(Waveform wf);
    void setDetuneCents(float baseCents);   // per-osc base detune from APVTS
    void setUnison(int voices, float spreadCents);
    void reset();
    float getNextSample();                  // normalized sum of N copies

private:
    Oscillator oscs[MaxUnison];
    int   unisonVoices = 1;
    float spreadCents  = 0.f;
    float baseCents    = 0.f;

    void recomputeDetune();  // called after any detune/spread/voices change
};
```

### Detune distribution

For N voices, voice `i` (0-indexed) receives:

```
offset_i = spreadCents × (i − (N−1)/2) / max(1, (N−1)/2)
```

This distributes voices symmetrically around 0. At N=1, offset=0 — identical to current behaviour, no regression.

### Output normalization

```cpp
return sum / std::sqrt(static_cast<float>(unisonVoices));
```

Power-preserving normalization: volume stays constant regardless of voice count.

---

## New Parameters

Both parameters are global (apply equally to osc1 and osc2).

| Parameter | ID | Type | Range | Default |
|---|---|---|---|---|
| Unison voices | `unison_voices` | int | 1 – 7 | 1 |
| Unison spread | `unison_detune` | float | 0 – 100 cents | 0 |

Added to:
- `ParameterIDs.h` — new ID constants
- `ParameterLayout.cpp` — APVTS registration
- `PluginProcessor::buildVoiceParams()` — read into `VoiceParams`
- `VoiceParams` struct — two new fields: `unisonVoices`, `unisonSpread`

---

## UI Changes

In the OSC tab, add two knobs alongside the existing osc controls:

- `UNISON` — stepped int knob, 1–7
- `SPREAD` — continuous float knob, 0–100 cents, labelled in cents

---

## Out of Scope

- Stereo spread (per-unison-voice panning) — step 2
- Per-osc independent unison settings — YAGNI
- Reverb, filter drive — steps 2 and 3 respectively
