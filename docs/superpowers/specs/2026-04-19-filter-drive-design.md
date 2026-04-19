# Filter Drive / Saturation — Design Spec

**Date:** 2026-04-19
**Status:** Approved
**Scope:** Step 3 of sound enrichment roadmap (Unison → Reverb → Filter Drive)

---

## Context

SynthVibe has Delay, Chorus, and Reverb in `Source/FX/`. All follow the same pattern: a `Params` struct, `prepare / setParams / process / reset` interface, orchestrated by `FXChain`. Drive follows this exact pattern as a post-FX saturation unit inserted between Chorus and Reverb.

---

## Signal Chain

```
FXChain::process
  → delay.process
  → chorus.process
  → drive.process    ← new, before reverb
  → reverb.process
```

Rationale: Drive saturates the dry signal + delay + chorus. Reverb then wraps the saturated result with a clean tail. Saturating after reverb would distort the reverb tail, producing muddy results.

---

## New Class: `Drive`

**Files:** `Source/FX/Drive.h` / `Drive.cpp`

```cpp
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

class Drive
{
public:
    enum class Type { Soft = 0, Hard, Fold };

    struct Params
    {
        Type  type       = Type::Soft;
        float driveDb    = 0.f;   // 0–24 dB pre-gain
        float mix        = 0.f;   // 0 = dry, 1 = wet
    };

    void prepare(double sampleRate, int maxBlockSize);
    void setParams(const Params& p);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    Params params;

    static float processSample(float x, Type type, float gain) noexcept;
};
```

### Algorithms

All applied sample-by-sample after converting `driveDb` to linear gain:
`gain = std::pow(10.f, driveDb / 20.f)`

| Mode | Formula | Character |
|---|---|---|
| **Soft** | `tanh(gain * x)` | Smooth, analog warmth, progressive |
| **Hard** | `jlimit(-1.f, 1.f, gain * x)` | Symmetric hard clip, aggressive |
| **Fold** | wrap signal back into [-1, 1] when it exceeds bounds | Metallic, FM-like, experimental |

Fold implementation:
```
y = gain * x
while y > 1.f:  y = 2.f - y
while y < -1.f: y = -2.f - y
```

### Mix (parallel saturation)

```cpp
out[i] = dry[i] * (1.f - params.mix) + wet[i] * params.mix;
```

`mix < 0.001f` → early return, zero CPU overhead.

---

## APVTS Parameters

| ID constant | APVTS ID | Type | Range | Default |
|---|---|---|---|---|
| `ParamIDs::driveType` | `"drive_type"` | AudioParameterChoice | Soft / Hard / Fold | Soft |
| `ParamIDs::driveAmount` | `"drive_amount"` | AudioParameterFloat | 0–24 dB, step 0.1 | 0 dB |
| `ParamIDs::driveMix` | `"drive_mix"` | AudioParameterFloat | 0–1, step 0.001 | 0 |

---

## FXChain Changes

`setParams` gains a `Drive::Params` argument:

```cpp
void setParams(const Delay::Params& dp,
               const Chorus::Params& cp,
               const Drive::Params& drp,
               const Reverb::Params& rp);
```

`process()` order: delay → chorus → drive → reverb.

---

## PluginProcessor Changes

- Add `Drive::Params buildDriveParams() const` (declaration in `.h`, implementation in `.cpp`)
- Update `fxChain.setParams(...)` call in `processBlock` to pass `buildDriveParams()`
- `buildDriveParams()` reads `driveType`, `driveAmount`, `driveMix` from APVTS

---

## UI — FXTab

FXTab gains a 4th panel: **DELAY / CHORUS / DRIVE / REVERB**

Layout: 4 equal-width columns. At 1200 px plugin width each panel is ~275 px after padding — comfortable.

DRIVE panel controls:
- `ComboBox` bound to `drive_type` (Soft / Hard / Fold) — consistent with ArpTab's mode selector
- `KnobWithLabel` for Drive (0–24 dB, suffix `" dB"`, 1 decimal)
- `KnobWithLabel` for Mix (0–1, no suffix, 2 decimals)

---

## File Map

| Action | File | What changes |
|---|---|---|
| Create | `Source/FX/Drive.h` | Class declaration |
| Create | `Source/FX/Drive.cpp` | Soft/Hard/Fold implementation |
| Modify | `CMakeLists.txt` | Add `Drive.cpp` to source list |
| Modify | `Source/Parameters/ParameterIDs.h` | 3 new ID constants |
| Modify | `Source/Parameters/ParameterLayout.cpp` | Register 3 APVTS params |
| Modify | `Source/FX/FXChain.h` | Add `Drive` member, extend `setParams` |
| Modify | `Source/FX/FXChain.cpp` | Insert drive between chorus and reverb |
| Modify | `Source/PluginProcessor.h` | Add `buildDriveParams()` declaration |
| Modify | `Source/PluginProcessor.cpp` | Implement `buildDriveParams()`, update `processBlock` call |
| Modify | `Source/UI/FXTab.h` | Add DRIVE panel (ComboBox + 2 knobs) |
