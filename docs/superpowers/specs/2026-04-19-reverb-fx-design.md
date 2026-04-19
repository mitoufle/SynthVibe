# Reverb FX — Design Spec

**Date:** 2026-04-19
**Status:** Approved
**Scope:** Step 2 of sound enrichment roadmap (Unison → Reverb → Filter Drive)

---

## Context

SynthVibe has Delay and Chorus in `Source/FX/`. Both follow the same pattern: a `Params` struct, `prepare / setParams / process / reset` interface, orchestrated by `FXChain`. Reverb follows the exact same pattern using JUCE's built-in `juce::Reverb` (Freeverb algorithm) — no custom DSP needed.

---

## Architecture

```
FXChain::process
  → delay.process
  → chorus.process
  → reverb.process    ← new, last in chain
```

Reverb goes last: room ambience wraps the modulation, which is the natural signal flow for a synth.

---

## New Class: `Reverb`

**Files:** `Source/FX/Reverb.h` / `Reverb.cpp`

```cpp
#pragma once
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>

class Reverb
{
public:
    struct Params
    {
        float room = 0.5f;  // 0–1
        float damp = 0.5f;  // 0–1
        float mix  = 0.f;   // 0 = dry, 1 = wet
    };

    void prepare(double sampleRate, int maxBlockSize);
    void setParams(const Params& p);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    juce::Reverb reverb;
    Params params;
};
```

### `setParams` mapping

`juce::Reverb::Parameters` fields:
- `roomSize`  ← `params.room`
- `damping`   ← `params.damp`
- `wetLevel`  ← `params.mix`
- `dryLevel`  ← `1.f - params.mix`
- `width`     = 1.f (fixed)
- `freezeMode`= 0.f (fixed)

### `process` guard

Skip processing entirely when `params.mix < 0.001f` to avoid CPU cost at default (mix = 0).

---

## Parameters

Three new entries in `ParameterIDs.h` and `ParameterLayout.cpp`.

| Constant | ID string | Type | Range | Step | Default |
|---|---|---|---|---|---|
| `reverbRoom` | `reverb_room` | float | 0–1 | 0.01 | 0.5 |
| `reverbDamp` | `reverb_damp` | float | 0–1 | 0.01 | 0.5 |
| `reverbMix`  | `reverb_mix`  | float | 0–1 | 0.001 | 0.0 |

Added after the Chorus block, before the Unison block.

---

## FXChain Changes

**`FXChain.h`** — add `Reverb reverb` member, update `setParams` signature:

```cpp
#include "Reverb.h"

class FXChain {
public:
    void prepare(double sampleRate, int maxBlockSize);
    void setParams(const Delay::Params& dp,
                   const Chorus::Params& cp,
                   const Reverb::Params& rp);   // ← add rp
    void process(juce::AudioBuffer<float>&);
    void reset();
private:
    Delay  delay;
    Chorus chorus;
    Reverb reverb;                               // ← add
};
```

**`FXChain.cpp`** — forward all calls to `reverb` alongside `delay` and `chorus`.

---

## PluginProcessor Changes

**`PluginProcessor.h`** — add `buildReverbParams()` declaration (private, `const`).

**`PluginProcessor.cpp`**:
- `buildReverbParams()` reads the three new APVTS params.
- `processBlock()` passes the result as third argument to `fxChain.setParams(...)`.

---

## UI Changes

**`Source/UI/FXTab.h`** — add a third "REVERB" panel.

Layout change: currently 50/50 split for Delay/Chorus. Change to three equal columns (33/33/33):

```cpp
// resized()
const int colW = area.getWidth() / 3;
delayBounds  = area.removeFromLeft(colW).reduced(pad, 0);
chorusBounds = area.removeFromLeft(colW).reduced(pad, 0);
reverbBounds = area.reduced(pad, 0);
```

Three new `KnobWithLabel` members:
```cpp
KnobWithLabel knobReverbRoom { "Room", apvts, ParamIDs::reverbRoom, "",    2 };
KnobWithLabel knobReverbDamp { "Damp", apvts, ParamIDs::reverbDamp, "",    2 };
KnobWithLabel knobReverbMix  { "Mix",  apvts, ParamIDs::reverbMix,  "",    2 };
```

`paint()` adds `drawPanel(g, reverbBounds, "REVERB", SynthLookAndFeel::colFxAccent)`.

---

## CMakeLists.txt

Add `Source/FX/Reverb.cpp` after `Source/FX/Chorus.cpp`. Ensure `juce_dsp` is in the target's linked modules (check if already present for other FX).

---

## File Map

| Action | File |
|---|---|
| Create | `Source/FX/Reverb.h` |
| Create | `Source/FX/Reverb.cpp` |
| Modify | `Source/Parameters/ParameterIDs.h` |
| Modify | `Source/Parameters/ParameterLayout.cpp` |
| Modify | `Source/FX/FXChain.h` |
| Modify | `Source/FX/FXChain.cpp` |
| Modify | `Source/PluginProcessor.h` |
| Modify | `Source/PluginProcessor.cpp` |
| Modify | `Source/UI/FXTab.h` |
| Modify | `CMakeLists.txt` |

---

## Out of Scope

- Pre-delay parameter
- Freeze/infinite reverb mode
- Per-voice reverb (reverb is always post-mix, global)
- Custom FDN or convolution algorithm
