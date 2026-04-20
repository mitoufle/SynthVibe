# Parameter Smoothing — Design Spec

**Date:** 2026-04-21
**Issue:** I-4 from 2026-04-20 code review
**Goal:** Eliminate zipper noise on knob moves by smoothing audio-rate parameters with `juce::SmoothedValue`.

---

## Problem

No parameter is smoothed before being applied to the audio graph. When the user moves a knob, the raw APVTS value jumps between blocks, causing step discontinuities that manifest as zipper noise — most audible on filter cutoff and resonance, also present on oscillator levels, master volume, and FX mix controls.

---

## Approach

`juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>` with a **5 ms ramp time** for all parameters. Smoothers live where per-sample work already happens so they advance correctly.

---

## Voice (4 smoothers per voice, 32 total)

**File:** `Source/Engine/Voice.h` / `Voice.cpp`

Add four members:
```cpp
juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothCutoff;
juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothResonance;
juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothOsc1Level;
juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothOsc2Level;
```

- `prepare()`: call `.reset(sampleRate, 0.005)` on each, then `.setCurrentAndTargetValue()` with a sensible default (cutoff=20000, resonance=0.5, levels=1.0).
- `setParams()`: call `.setTargetValue()` for each instead of relying on raw `params` fields.
- `getNextSample()`: replace `params.filterCutoff`, `params.filterResonance`, `params.osc1.level`, `params.osc2.level` with `.getNextValue()` calls. The `hasFilterMod` guard stays; it uses the smoothed cutoff as its base.

---

## PluginProcessor (1 smoother — master volume)

**File:** `Source/PluginProcessor.h` / `PluginProcessor.cpp`

Add one member:
```cpp
juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMasterVol;
```

- `prepareToPlay()`: `.reset(sampleRate, 0.005)`, `.setCurrentAndTargetValue(1.0f)`.
- `processBlock()`: call `.setTargetValue(*apvts.getRawParameterValue(ParamIDs::masterVolume))` each block, then replace `buffer.applyGain(masterVol)` with a per-sample loop advancing the smoother.

---

## FX Units (1 smoother each — mix parameter)

Each FX unit already has a per-sample loop in `process()`. Each gets one `SmoothedValue<float, Linear>` for its mix.

| Unit | File |
|---|---|
| `Delay` | `Source/FX/Delay.h` / `Delay.cpp` |
| `Chorus` | `Source/FX/Chorus.h` / `Chorus.cpp` |
| `Drive` | `Source/FX/Drive.h` / `Drive.cpp` |
| `Reverb` | `Source/FX/Reverb.h` / `Reverb.cpp` |

Pattern for each:
- Add `juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMix;`
- `prepare()`: `.reset(sampleRate, 0.005)`, `.setCurrentAndTargetValue(0.0f)`.
- `setParams()`: `.setTargetValue(params.mix)`.
- `process()`: replace `params.mix` in the wet/dry blend with `.getNextValue()`.

`FXChain` itself is unchanged — smoothing lives inside each unit.

---

## What is NOT smoothed

- Waveform, octave, semitone (stepped/enum, not audio-rate)
- ADSR times (not moved in real-time during a note)
- LFO rate/depth/dest (changed infrequently)
- Unison count/detune/spread (stepped, voice lifecycle handles transitions)
- Delay time (separate concern — time-domain artifacts need a different approach)

---

## Files changed

| File | Change |
|---|---|
| `Source/Engine/Voice.h` | Add 4 SmoothedValue members |
| `Source/Engine/Voice.cpp` | prepare/setParams/getNextSample |
| `Source/PluginProcessor.h` | Add smoothMasterVol |
| `Source/PluginProcessor.cpp` | prepareToPlay + processBlock |
| `Source/FX/Delay.h` / `.cpp` | smoothMix |
| `Source/FX/Chorus.h` / `.cpp` | smoothMix |
| `Source/FX/Drive.h` / `.cpp` | smoothMix |
| `Source/FX/Reverb.h` / `.cpp` | smoothMix |
