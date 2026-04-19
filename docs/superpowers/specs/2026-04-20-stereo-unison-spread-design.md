# Stereo Unison Spread — Design Spec

**Date:** 2026-04-20
**Status:** Approved
**Scope:** Sound Enrichment Phase 2 — Step 1 of 4

---

## Context

SynthVibe's `UnisonOscillator` currently sums all unison sub-voices into a single mono `float`. All polyphonic voices are then written identically to both L and R channels in `SynthEngine`. The result is a mono signal regardless of how many unison voices are active — the main reason the sound feels thin even with Unison enabled.

This spec adds true per-voice stereo panning to the unison oscillator layer, a separate `unison_spread` APVTS parameter, and updates `Voice` and `SynthEngine` to produce and accumulate stereo output.

---

## Signal Flow (after this change)

```
UnisonOscillator::getNextSample(outL, outR)
  → sub-voice i contributes sample * lGain[i] to outL
  → sub-voice i contributes sample * rGain[i] to outR

Voice::getNextSample() → std::pair<float, float>
  mixL = osc1.outL + osc2.outL * osc2Level
  mixR = osc1.outR + osc2.outR * osc2Level
  filtL = filterL.processSample(mixL)   ← existing Filter instance
  filtR = filterR.processSample(mixR)   ← new second Filter instance
  env   = ampEnv.getNextSample()
  return { filtL * env, filtR * env }

SynthEngine::processBlock
  sumL += voice.getNextSample().first
  sumR += voice.getNextSample().second
  buffer.setSample(0, i, sumL * gain)
  buffer.setSample(1, i, sumR * gain)
```

---

## Pan Position Formula

For N unison voices and spread amount S ∈ [0, 1]:

```
pan_i = S * (2 * i / max(N - 1, 1) - 1)    // range [-S, +S], center=0
```

Single voice (N=1): `pan_i = 0` (center regardless of spread).

Constant-power panning from `pan ∈ [-1, 1]` to L/R gains:
```
angle = (pan + 1) * π / 4          // maps [-1,1] → [0, π/2]
lGain = cos(angle)
rGain = sin(angle)
```

Pan gains are precomputed in `recomputePan()` whenever `setUnison()` or `setStereoSpread()` is called. They are stored as `float lGains[MaxUnison]` and `float rGains[MaxUnison]` in `UnisonOscillator`.

Power normalization (`invNormGain = 1/sqrt(N)`) is applied per sub-voice sample before pan gain multiplication so total output power is independent of voice count.

---

## UnisonOscillator Changes

**New method signature (replaces existing `getNextSample()`):**
```cpp
void getNextSample(float& outL, float& outR);
```

**New method:**
```cpp
void setStereoSpread(float spread);   // 0–1
```

**New private members:**
```cpp
float lGains[MaxUnison] = {};
float rGains[MaxUnison] = {};
float stereoSpread = 0.f;
```

**Updated `setUnison()`:** calls `recomputePan()` after existing `recomputeDetune()`.

**New private method:**
```cpp
void recomputePan();
```

The existing `float getNextSample()` mono overload is removed — all callers are updated.

---

## Voice Changes

### `Voice.h`
- `float getNextSample()` → `std::pair<float, float> getNextSample()`
- Add `Filter filterR;` private member (second channel filter, same settings as `filter`)
- `VoiceParams`: rename `unisonSpread` → `unisonDetuneCents`, add `float unisonStereoSpread = 0.5f`

### `Voice.cpp`
- `prepare()`: call `filterR.prepare(spec)` alongside existing `filter.prepare(spec)`
- `setParams()`:
  - Pass `unisonDetuneCents` (renamed) to `osc1.setDetuneCents()` / `osc2.setDetuneCents()`
  - Call `osc1.setStereoSpread(params.unisonStereoSpread)` and same for `osc2`
  - Call `filterR.setType()`, `filterR.setCutoff()`, `filterR.setResonance()` mirroring existing `filter` calls
- `getNextSample()`: see signal flow above
- `noteOn()` / `noteOff()`: no changes needed
- `reset()` (if present): add `filterR.reset()`

---

## SynthEngine Changes

### `SynthEngine.cpp` — `processBlock()`

Replace mono accumulation:
```cpp
// Before
float sample = 0.f;
for (auto& v : voices)
    if (v.isActive()) sample += v.getNextSample();
sample *= gain;
buffer.setSample(0, i, buffer.getSample(0, i) + sample);
buffer.setSample(1, i, buffer.getSample(1, i) + sample);
```

With stereo accumulation:
```cpp
float sumL = 0.f, sumR = 0.f;
for (auto& v : voices) {
    if (v.isActive()) {
        auto [l, r] = v.getNextSample();
        sumL += l;
        sumR += r;
    }
}
sumL *= gain;
sumR *= gain;
buffer.setSample(0, i, buffer.getSample(0, i) + sumL);
buffer.setSample(1, i, buffer.getSample(1, i) + sumR);
```

---

## APVTS Parameter

| ID constant | APVTS ID | Type | Range | Default |
|---|---|---|---|---|
| `ParamIDs::unisonStereoSpread` | `"unison_spread"` | AudioParameterFloat | 0–1, step 0.01 | 0.5 |

Default 0.5 (half spread) so existing presets immediately benefit from some stereo width without sounding extreme.

---

## PluginProcessor Changes

### `ParameterIDs.h`
Add:
```cpp
inline constexpr auto unisonStereoSpread = "unison_spread";
```

### `ParameterLayout.cpp`
Register the new parameter alongside existing unison params.

### `PluginProcessor.cpp` — `buildVoiceParams()`
- Rename the existing `p.unisonSpread` assignment to `p.unisonDetuneCents`
- Add: `p.unisonStereoSpread = *apvts.getRawParameterValue(ParamIDs::unisonStereoSpread);`

---

## UI — SoundTab

The existing Unison panel (Voices + Detune knobs) gains a third knob:

- `KnobWithLabel` bound to `unison_spread`, label `"Spread"`, range 0–1, 2 decimals, no suffix

Layout: 3 knobs side by side within the Unison panel (same pattern as the other panels).

---

## File Map

| Action | File | What changes |
|---|---|---|
| Modify | `Source/Engine/UnisonOscillator.h` | New `getNextSample(float&, float&)`, pan members, `setStereoSpread()` |
| Modify | `Source/Engine/UnisonOscillator.cpp` | Implement pan computation and stereo sample output |
| Modify | `Source/Engine/Voice.h` | Rename `unisonSpread` in VoiceParams, add `unisonStereoSpread`; add `filterR`; change return type |
| Modify | `Source/Engine/Voice.cpp` | Stereo output, second filter, pass spread to oscillators |
| Modify | `Source/Engine/SynthEngine.cpp` | Stereo accumulation in processBlock |
| Modify | `Source/Parameters/ParameterIDs.h` | Add `unisonStereoSpread` constant |
| Modify | `Source/Parameters/ParameterLayout.cpp` | Register `unison_spread` |
| Modify | `Source/PluginProcessor.cpp` | Read `unisonStereoSpread` in `buildVoiceParams()` |
| Modify | `Source/UI/SoundTab.h` | Add Spread knob to Unison panel |
