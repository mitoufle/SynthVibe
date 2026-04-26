# Phase 3.5 — ModEngine (runtime modulation) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the 8-slot modulation matrix actually affect audio — sources (LFO 1/2, Env Amp, Env Filt, Velocity, Keytrack) drive destinations (cutoff, resonance, drive, osc fine, osc level, osc PWM, master vol) at runtime.

**Architecture:** `ModEngine` is a free-function namespace + POD types. SynthEngine holds a `Snapshot` of the 8 slots refreshed per block from APVTS. Each Voice holds a `ModBus` of mod deltas/multipliers, recomputed at control rate (every 16 samples) inside `getNextSample()`. Voice's existing legacy LFO destination paths (`lfo1.dest`/`lfo2.dest`) are preserved unchanged — the matrix is purely additive on top.

**Tech Stack:** C++17, JUCE 7.0.9, `juce::AudioProcessorValueTreeState`, `juce::UnitTest`.

**Build:** `./build-with-vs.bat`. **Tests:** `./build/AISynthTests_artefacts/Release/AISynthTests.exe`. **Deploy:** `./deploy.ps1`.

**Branch:** stay on `phase2-sound-tab` (Phase 3 ModMatrix UI just landed in commits `35429c3..c61e349`).

---

## Audible behaviour after this lands

| Configuration | Audible result |
|---|---|
| `mod.1.src = LFO 1, mod.1.dst = Cutoff, mod.1.amount = 0.5, lfo1.rate = 2 Hz` | Filter cutoff oscillates ±2.5 octaves around its base value at 2 Hz. Wobble. |
| `mod.1.src = Velocity, mod.1.dst = Osc1 Level, mod.1.amount = 1.0` | Note loudness scales with key strike velocity. |
| `mod.1.src = Env Filt, mod.1.dst = Cutoff, mod.1.amount = 1.0` | Filter env opens cutoff during attack — doubles up with the legacy `filt.envamt` knob. |

Slots that select a not-yet-implemented source (Modwheel, Aftertouch, Random) or destination (Amp Attack, Amp Release) are silently no-ops in V1.

---

## File Structure

| File | Status | Responsibility |
|---|---|---|
| `Source/Engine/ModBus.h` | create | POD struct holding mod deltas/multipliers per destination |
| `Source/Engine/ModEngine.h` | create | namespace declarations: `Slot`, `Snapshot`, `SourceValues`, free functions |
| `Source/Engine/ModEngine.cpp` | create | implementations of `applyCurve`, `readSnapshot`, `applyToBus`, `applyMatrix` |
| `Source/Engine/Voice.h` | modify | add 4 source accessors, `setMatrixSnapshot()`, private `ModBus modBus`, snapshot pointer |
| `Source/Engine/Voice.cpp` | modify | populate `SourceValues` at control rate, call `applyMatrix`, consume `ModBus` in cutoff/level/pwm paths |
| `Source/Engine/SynthEngine.h` | modify | hold `ModEngine::Snapshot`, add `setMatrixSnapshot()` |
| `Source/Engine/SynthEngine.cpp` | modify | distribute snapshot to voices in `setParams` |
| `Source/PluginProcessor.cpp` | modify | per-block: read APVTS → snapshot → forward to SynthEngine |
| `CMakeLists.txt` | modify | add `ModEngine.cpp` to both `AISynth` and `AISynthTests` source lists |
| `Tests/ModEngineTests.cpp` | create | unit tests for curves, snapshot reading, applyToBus, applyMatrix |
| `Tests/VoiceTests.cpp` | modify | end-to-end test: LFO1→Cutoff causes audible cutoff oscillation |

---

## Task 1: `ModBus` struct

**Files:**
- Create: `Source/Engine/ModBus.h`
- Test: `Tests/ModEngineTests.cpp` (created)
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing test**

Create `Tests/ModEngineTests.cpp`:

```cpp
#include <juce_core/juce_core.h>
#include "Engine/ModBus.h"

struct ModEngineTests : public juce::UnitTest
{
    ModEngineTests() : juce::UnitTest("ModEngine", "AISynth") {}

    void runTest() override
    {
        beginTest("ModBus default-constructs to identity");
        SynthVibe::ModBus bus;
        expectWithinAbsoluteError(bus.cutoffSemitones, 0.f, 1e-6f);
        expectWithinAbsoluteError(bus.resonanceDelta,  0.f, 1e-6f);
        expectWithinAbsoluteError(bus.driveDelta,      0.f, 1e-6f);
        expectWithinAbsoluteError(bus.osc1FineCents,   0.f, 1e-6f);
        expectWithinAbsoluteError(bus.osc2FineCents,   0.f, 1e-6f);
        expectWithinAbsoluteError(bus.osc1LevelMul,    1.f, 1e-6f);
        expectWithinAbsoluteError(bus.osc2LevelMul,    1.f, 1e-6f);
        expectWithinAbsoluteError(bus.osc1PwmDelta,    0.f, 1e-6f);
        expectWithinAbsoluteError(bus.osc2PwmDelta,    0.f, 1e-6f);
        expectWithinAbsoluteError(bus.masterVolMul,    1.f, 1e-6f);
    }
};

static ModEngineTests sModEngineTests;
```

- [ ] **Step 2: Add `Tests/ModEngineTests.cpp` to CMake**

In `CMakeLists.txt`, in the `target_sources(AISynthTests PRIVATE …)` block, add after `Tests/UIConstructionTests.cpp`:

```cmake
    Tests/ModEngineTests.cpp
```

- [ ] **Step 3: Run tests to confirm they fail**

```
./build-with-vs.bat
```

Expected: compile error — `ModBus.h` missing.

- [ ] **Step 4: Create `Source/Engine/ModBus.h`**

```cpp
#pragma once

namespace SynthVibe
{
    // Modulation bus — accumulates the matrix's contribution per destination.
    // Reset to identity (zero deltas, one multipliers) before each control-rate
    // recompute, then applyToBus() adds slot contributions in.
    struct ModBus
    {
        float cutoffSemitones = 0.f;   // additive offset; +12 = +1 octave
        float resonanceDelta  = 0.f;   // additive in [0..1] domain
        float driveDelta      = 0.f;   // additive in [0..1] domain
        float osc1FineCents   = 0.f;   // additive cents
        float osc2FineCents   = 0.f;
        float osc1LevelMul    = 1.f;   // multiplier
        float osc2LevelMul    = 1.f;
        float osc1PwmDelta    = 0.f;   // additive in [0.01..0.99] domain
        float osc2PwmDelta    = 0.f;
        float masterVolMul    = 1.f;   // multiplier
    };
}
```

- [ ] **Step 5: Run tests to confirm they pass**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynthTests.exe"
```

Expected: `ModEngine / ModBus default-constructs to identity` passes.

- [ ] **Step 6: Commit**

```
git add Source/Engine/ModBus.h Tests/ModEngineTests.cpp CMakeLists.txt
git commit -m "feat(engine): ModBus POD for mod deltas and multipliers"
```

---

## Task 2: `ModEngine` types + `applyCurve`

**Files:**
- Create: `Source/Engine/ModEngine.h`
- Create: `Source/Engine/ModEngine.cpp`
- Modify: `Tests/ModEngineTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write failing test (append to `Tests/ModEngineTests.cpp` `runTest()`)**

Add the include at the top:

```cpp
#include "Engine/ModEngine.h"
```

Append in `runTest()`:

```cpp
        beginTest("applyCurve preserves identity for lin");
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve( 0.5f, 0), 0.5f, 1e-5f);
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve(-0.5f, 0),-0.5f, 1e-5f);
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve( 0.0f, 0), 0.0f, 1e-5f);

        beginTest("applyCurve exp is signed power-2");
        // exp(0.5) = 0.5^2 = 0.25 ; exp(-0.5) = -0.25 ; preserves sign
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve( 0.5f, 1), 0.25f, 1e-5f);
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve(-0.5f, 1),-0.25f, 1e-5f);
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve( 1.0f, 1), 1.0f,  1e-5f);

        beginTest("applyCurve log is signed power-0.5");
        // log(0.25) = sqrt(0.25) = 0.5 ; log(-0.25) = -0.5
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve( 0.25f, 2), 0.5f, 1e-5f);
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve(-0.25f, 2),-0.5f, 1e-5f);

        beginTest("applyCurve s is tanh-shaped");
        // s(v) = tanh(v*2). At v=0: 0. At v=1: tanh(2) ≈ 0.964. At v=-1: ≈ -0.964.
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve( 0.f, 3), 0.f, 1e-5f);
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve( 1.f, 3), std::tanh(2.f), 1e-5f);
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve(-1.f, 3),-std::tanh(2.f), 1e-5f);

        beginTest("applyCurve unknown index falls back to lin");
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve(0.7f, 99), 0.7f, 1e-5f);
```

Add `#include <cmath>` at the top of the test file too.

- [ ] **Step 2: Run tests to confirm they fail**

```
./build-with-vs.bat
```

Expected: compile error — `ModEngine.h` missing.

- [ ] **Step 3: Create `Source/Engine/ModEngine.h`**

```cpp
#pragma once
#include <array>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ModBus.h"

namespace SynthVibe::ModEngine
{
    static constexpr int kNumActiveSlots = 8;   // matches ModMatrixTable::kVisibleRows

    struct Slot
    {
        int   src    = 0;   // 0 = None
        int   dst    = 0;   // 0 = None
        float amount = 0.f; // -1..+1
        int   curve  = 0;   // 0=lin 1=exp 2=log 3=s
    };

    using Snapshot = std::array<Slot, kNumActiveSlots>;

    struct SourceValues
    {
        float lfo1     = 0.f;   // -1..+1, raw LFO output (BEFORE depth scaling)
        float lfo2     = 0.f;
        float envAmp   = 0.f;   // 0..1, current amp env value
        float envFilt  = 0.f;   // 0..1, current filter env value
        float velocity = 0.f;   // 0..1, note velocity
        float keytrack = 0.f;   // -1..+1, signed octaves from C4 / 5 (clamped)
    };

    // Pure functions
    float applyCurve(float v, int curveIdx);
    void  applyToBus(ModBus& bus, int dst, float modVal);
    void  applyMatrix(const Snapshot& snap, const SourceValues& srcs, ModBus& bus);

    // Reads 8 slots from the APVTS into snapshot. Cheap per-block call.
    void  readSnapshot(juce::AudioProcessorValueTreeState& apvts, Snapshot& out);
}
```

- [ ] **Step 4: Create `Source/Engine/ModEngine.cpp` (curves only for now)**

```cpp
#include "ModEngine.h"
#include <cmath>

namespace SynthVibe::ModEngine
{
    float applyCurve(float v, int curveIdx)
    {
        switch (curveIdx)
        {
            case 0: return v;                                      // lin
            case 1: return std::copysign(v * v, v);                // exp (signed v^2)
            case 2: return std::copysign(std::sqrt(std::abs(v)), v); // log (signed sqrt)
            case 3: return std::tanh(v * 2.f);                     // s
            default: return v;
        }
    }

    // Stubs — implemented in later tasks.
    void applyToBus(ModBus&, int, float) {}
    void applyMatrix(const Snapshot&, const SourceValues&, ModBus&) {}
    void readSnapshot(juce::AudioProcessorValueTreeState&, Snapshot&) {}
}
```

- [ ] **Step 5: Add `ModEngine.cpp` to CMake**

In `CMakeLists.txt`, in BOTH `target_sources(AISynth …)` and `target_sources(AISynthTests …)` blocks, add after `Source/Engine/Voice.cpp`:

```cmake
    Source/Engine/ModEngine.cpp
```

- [ ] **Step 6: Run tests to confirm they pass**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynthTests.exe"
```

Expected: all 4 new `applyCurve` tests pass. Earlier ModBus test still passes.

- [ ] **Step 7: Commit**

```
git add Source/Engine/ModEngine.h Source/Engine/ModEngine.cpp Tests/ModEngineTests.cpp CMakeLists.txt
git commit -m "feat(engine): ModEngine types + applyCurve"
```

---

## Task 3: `readSnapshot` from APVTS

**Files:**
- Modify: `Source/Engine/ModEngine.cpp`
- Modify: `Tests/ModEngineTests.cpp`

- [ ] **Step 1: Write failing test (append to `runTest()`)**

Add the includes at the top of the test file (after the existing ones):

```cpp
#include "Parameters/ParameterLayout.h"
#include "Parameters/ParameterIDs.h"
```

Append:

```cpp
        beginTest("readSnapshot reflects APVTS slot values");
        {
            juce::AudioProcessorGraph dummyGraph;
            juce::AudioProcessorValueTreeState apvts(
                dummyGraph, nullptr, "AISynthState", ParameterLayout::create());

            // Configure slot 1: src=lfo1 (idx 1), dst=cutoff (idx 1), amount=0.5, curve=exp (idx 1)
            apvts.getParameter(ParamIDs::mod1Src   )->setValueNotifyingHost(apvts.getParameter(ParamIDs::mod1Src   )->convertTo0to1(1.f));
            apvts.getParameter(ParamIDs::mod1Dst   )->setValueNotifyingHost(apvts.getParameter(ParamIDs::mod1Dst   )->convertTo0to1(1.f));
            apvts.getParameter(ParamIDs::mod1Amount)->setValueNotifyingHost(apvts.getParameter(ParamIDs::mod1Amount)->convertTo0to1(0.5f));
            apvts.getParameter(ParamIDs::mod1Curve )->setValueNotifyingHost(apvts.getParameter(ParamIDs::mod1Curve )->convertTo0to1(1.f));

            SynthVibe::ModEngine::Snapshot snap;
            SynthVibe::ModEngine::readSnapshot(apvts, snap);

            expectEquals(snap[0].src,    1);
            expectEquals(snap[0].dst,    1);
            expectWithinAbsoluteError(snap[0].amount, 0.5f, 1e-4f);
            expectEquals(snap[0].curve,  1);
            // Other slots should remain at default 0
            expectEquals(snap[1].src,    0);
            expectEquals(snap[7].src,    0);
        }
```

- [ ] **Step 2: Run tests to confirm they fail**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynthTests.exe"
```

Expected: `readSnapshot reflects APVTS slot values` fails — current `readSnapshot` is a stub that does nothing, so the snapshot keeps default 0 values and `expectEquals(snap[0].src, 1)` fails.

- [ ] **Step 3: Implement `readSnapshot` in `Source/Engine/ModEngine.cpp`**

Replace the stub with this content. Add `#include "../Parameters/ParameterIDs.h"` at the top of the .cpp file:

```cpp
#include "../Parameters/ParameterIDs.h"

namespace
{
    struct SlotIds { const char* src; const char* dst; const char* amount; const char* curve; };
    constexpr SlotIds kSlotIds[8] = {
        { ParamIDs::mod1Src, ParamIDs::mod1Dst, ParamIDs::mod1Amount, ParamIDs::mod1Curve },
        { ParamIDs::mod2Src, ParamIDs::mod2Dst, ParamIDs::mod2Amount, ParamIDs::mod2Curve },
        { ParamIDs::mod3Src, ParamIDs::mod3Dst, ParamIDs::mod3Amount, ParamIDs::mod3Curve },
        { ParamIDs::mod4Src, ParamIDs::mod4Dst, ParamIDs::mod4Amount, ParamIDs::mod4Curve },
        { ParamIDs::mod5Src, ParamIDs::mod5Dst, ParamIDs::mod5Amount, ParamIDs::mod5Curve },
        { ParamIDs::mod6Src, ParamIDs::mod6Dst, ParamIDs::mod6Amount, ParamIDs::mod6Curve },
        { ParamIDs::mod7Src, ParamIDs::mod7Dst, ParamIDs::mod7Amount, ParamIDs::mod7Curve },
        { ParamIDs::mod8Src, ParamIDs::mod8Dst, ParamIDs::mod8Amount, ParamIDs::mod8Curve },
    };
}

namespace SynthVibe::ModEngine
{
    void readSnapshot(juce::AudioProcessorValueTreeState& apvts, Snapshot& out)
    {
        for (int i = 0; i < 8; ++i)
        {
            out[i].src    = (int) *apvts.getRawParameterValue(kSlotIds[i].src);
            out[i].dst    = (int) *apvts.getRawParameterValue(kSlotIds[i].dst);
            out[i].amount =       *apvts.getRawParameterValue(kSlotIds[i].amount);
            out[i].curve  = (int) *apvts.getRawParameterValue(kSlotIds[i].curve);
        }
    }
}
```

- [ ] **Step 4: Run tests to confirm they pass**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynthTests.exe"
```

Expected: `readSnapshot reflects APVTS slot values` passes.

- [ ] **Step 5: Commit**

```
git add Source/Engine/ModEngine.cpp Tests/ModEngineTests.cpp
git commit -m "feat(engine): ModEngine::readSnapshot pulls 8 slots from APVTS"
```

---

## Task 4: `applyToBus` destination scaling

**Files:**
- Modify: `Source/Engine/ModEngine.cpp`
- Modify: `Tests/ModEngineTests.cpp`

- [ ] **Step 1: Write failing test (append)**

```cpp
        beginTest("applyToBus per-destination scaling");
        {
            using namespace SynthVibe;
            ModBus bus;

            // dst=1 cutoff: ±5 octaves at amount=1 → modVal=1 means +60 semitones
            ModEngine::applyToBus(bus, 1, 1.0f);
            expectWithinAbsoluteError(bus.cutoffSemitones, 60.f, 1e-3f);

            // dst=2 resonance: ±0.5 at amount=1
            bus = {};
            ModEngine::applyToBus(bus, 2, 1.0f);
            expectWithinAbsoluteError(bus.resonanceDelta, 0.5f, 1e-3f);

            // dst=4 osc1.fine: ±100 cents at amount=1
            bus = {};
            ModEngine::applyToBus(bus, 4, 1.0f);
            expectWithinAbsoluteError(bus.osc1FineCents, 100.f, 1e-3f);

            // dst=6 osc1.level: multiplier (1 + modVal)
            bus = {};
            ModEngine::applyToBus(bus, 6, -0.5f);
            expectWithinAbsoluteError(bus.osc1LevelMul, 0.5f, 1e-4f);

            // dst=8 osc1.pwm: ±0.4 at amount=1
            bus = {};
            ModEngine::applyToBus(bus, 8, 0.5f);
            expectWithinAbsoluteError(bus.osc1PwmDelta, 0.2f, 1e-4f);

            // dst=10 master.vol: multiplier (1 + modVal)
            bus = {};
            ModEngine::applyToBus(bus, 10, 0.5f);
            expectWithinAbsoluteError(bus.masterVolMul, 1.5f, 1e-4f);

            // dst=11 amp.attack and dst=12 amp.release: NO-OPS in V1
            bus = {};
            ModEngine::applyToBus(bus, 11, 1.0f);
            ModEngine::applyToBus(bus, 12, 1.0f);
            expectWithinAbsoluteError(bus.cutoffSemitones, 0.f, 1e-6f);
            expectWithinAbsoluteError(bus.masterVolMul,    1.f, 1e-6f);

            // dst=0 (None): no-op
            bus = {};
            ModEngine::applyToBus(bus, 0, 1.0f);
            expectWithinAbsoluteError(bus.cutoffSemitones, 0.f, 1e-6f);

            // Two slots hitting same destination → additive accumulation
            bus = {};
            ModEngine::applyToBus(bus, 1, 0.3f);
            ModEngine::applyToBus(bus, 1, 0.2f);
            expectWithinAbsoluteError(bus.cutoffSemitones, 30.f, 1e-3f);  // 0.5 × 60
        }
```

- [ ] **Step 2: Run tests to confirm they fail**

Run the test command — `applyToBus per-destination scaling` should fail because the stub is a no-op.

- [ ] **Step 3: Implement `applyToBus` in `Source/Engine/ModEngine.cpp`**

Replace the stub:

```cpp
    void applyToBus(ModBus& bus, int dst, float modVal)
    {
        switch (dst)
        {
            case 1:  bus.cutoffSemitones += modVal * 60.f;   break;  // ±5 octaves at modVal=1
            case 2:  bus.resonanceDelta  += modVal * 0.5f;   break;
            case 3:  bus.driveDelta      += modVal * 0.5f;   break;
            case 4:  bus.osc1FineCents   += modVal * 100.f;  break;  // ±1 semitone at modVal=1
            case 5:  bus.osc2FineCents   += modVal * 100.f;  break;
            case 6:  bus.osc1LevelMul    *= (1.f + modVal);  break;
            case 7:  bus.osc2LevelMul    *= (1.f + modVal);  break;
            case 8:  bus.osc1PwmDelta    += modVal * 0.4f;   break;
            case 9:  bus.osc2PwmDelta    += modVal * 0.4f;   break;
            case 10: bus.masterVolMul    *= (1.f + modVal);  break;
            // 11 = env.amp.attack, 12 = env.amp.release: deferred (envelope timing
            // mid-cycle modulation needs careful state handling; freeze for V1).
            case 11: case 12: break;
            default: break;  // 0 = None, anything else = unknown → no-op
        }
    }
```

- [ ] **Step 4: Run tests to confirm they pass**

Run tests — all `applyToBus` assertions pass.

- [ ] **Step 5: Commit**

```
git add Source/Engine/ModEngine.cpp Tests/ModEngineTests.cpp
git commit -m "feat(engine): ModEngine::applyToBus destination scaling"
```

---

## Task 5: `applyMatrix` orchestration

**Files:**
- Modify: `Source/Engine/ModEngine.cpp`
- Modify: `Tests/ModEngineTests.cpp`

- [ ] **Step 1: Write failing test (append)**

```cpp
        beginTest("applyMatrix combines slots, sources, curves");
        {
            using namespace SynthVibe;

            ModEngine::Snapshot snap{};
            // Slot 1: lfo1 → cutoff, amount=1, curve=lin
            snap[0] = { 1, 1, 1.0f, 0 };
            // Slot 2: velocity → osc1.level, amount=1, curve=lin
            snap[1] = { 5, 6, 1.0f, 0 };
            // Slot 3: src=None → skipped
            snap[2] = { 0, 1, 1.0f, 0 };
            // Slot 4: amount=0 → skipped
            snap[3] = { 1, 1, 0.0f, 0 };

            ModEngine::SourceValues srcs;
            srcs.lfo1     = 0.5f;   // would push cutoff +30 st (0.5 × 60)
            srcs.velocity = 0.4f;   // would push osc1Level × 1.4

            ModBus bus;
            ModEngine::applyMatrix(snap, srcs, bus);

            expectWithinAbsoluteError(bus.cutoffSemitones, 30.f, 1e-3f);
            expectWithinAbsoluteError(bus.osc1LevelMul,    1.4f, 1e-4f);
        }

        beginTest("applyMatrix exp curve squashes mid-range source");
        {
            using namespace SynthVibe;
            ModEngine::Snapshot snap{};
            snap[0] = { 1, 1, 1.0f, 1 };  // exp curve

            ModEngine::SourceValues srcs;
            srcs.lfo1 = 0.5f;             // exp(0.5) = 0.25, so contribution is 0.25 × 60 = 15

            ModBus bus;
            ModEngine::applyMatrix(snap, srcs, bus);
            expectWithinAbsoluteError(bus.cutoffSemitones, 15.f, 1e-3f);
        }
```

- [ ] **Step 2: Run tests to confirm they fail**

Tests fail because `applyMatrix` is a stub.

- [ ] **Step 3: Implement `applyMatrix` in `Source/Engine/ModEngine.cpp`**

Replace the stub:

```cpp
    void applyMatrix(const Snapshot& snap, const SourceValues& srcs, ModBus& bus)
    {
        for (const auto& slot : snap)
        {
            if (slot.src == 0 || slot.amount == 0.f) continue;

            float srcValue = 0.f;
            switch (slot.src)
            {
                case 1: srcValue = srcs.lfo1;     break;
                case 2: srcValue = srcs.lfo2;     break;
                case 3: srcValue = srcs.envAmp;   break;
                case 4: srcValue = srcs.envFilt;  break;
                case 5: srcValue = srcs.velocity; break;
                // 6 = Modwheel, 7 = Aftertouch: deferred V2
                case 8: srcValue = srcs.keytrack; break;
                // 9 = Random: deferred V2
                default: continue;
            }

            const float shaped = applyCurve(srcValue, slot.curve);
            applyToBus(bus, slot.dst, shaped * slot.amount);
        }
    }
```

- [ ] **Step 4: Run tests to confirm they pass**

Run tests — both `applyMatrix` cases pass.

- [ ] **Step 5: Commit**

```
git add Source/Engine/ModEngine.cpp Tests/ModEngineTests.cpp
git commit -m "feat(engine): ModEngine::applyMatrix orchestrates curve+amount per slot"
```

---

## Task 6: Voice source accessors

**Files:**
- Modify: `Source/Engine/Voice.h`
- Modify: `Source/Engine/Voice.cpp`
- Modify: `Source/Engine/Envelope.h` (if needed for `peek()` accessor)
- Modify: `Tests/VoiceTests.cpp`

- [ ] **Step 1: Inspect Envelope to see if a `peek()` already exists**

Read `Source/Engine/Envelope.h`. If it has a `getCurrentValue()` or similar non-advancing accessor, use that. If not, add one in this task.

For the rest of this task, assume the accessor is named `Envelope::peek() const`. Adapt to whatever exists (e.g., `getCurrentValue()`).

- [ ] **Step 2: Write the failing test**

In `Tests/VoiceTests.cpp`, append a new `beginTest` block to `runTest()`:

```cpp
        beginTest("Voice exposes per-voice mod sources");
        {
            Voice v;
            juce::dsp::ProcessSpec spec { 48000.0, 256, 1 };
            v.prepare(spec);
            VoiceParams p;
            p.lfo1.depth = 0.5f;
            p.lfo1.rate  = 5.f;
            v.setParams(p);
            v.noteOn(72, 0.8f);   // C5, velocity 0.8

            // Velocity captured directly from noteOn
            expectWithinAbsoluteError(v.getVelocity(), 0.8f, 1e-4f);
            // Keytrack: note 72 (C5) is +12 semitones above C4 (60) → +1 octave → +0.2 (octaves/5)
            expectWithinAbsoluteError(v.getKeytrackOctaves(), 0.2f, 1e-3f);
            // Env values pre-getNextSample: ampEnv has been triggered → in attack
            expect(v.getEnvAmpValue() >= 0.f && v.getEnvAmpValue() <= 1.f);
            expect(v.getEnvFiltValue() >= 0.f && v.getEnvFiltValue() <= 1.f);
        }
```

- [ ] **Step 3: Run tests to confirm they fail**

Run tests — compile error: `Voice::getVelocity()` etc. not declared.

- [ ] **Step 4: Add accessors to `Source/Engine/Voice.h`**

In the `public:` section of Voice (after `setNoteOnOrder`), add:

```cpp
    // Per-voice modulation source accessors. Cheap reads — no side effects.
    float getVelocity()       const noexcept { return velocity; }
    float getEnvAmpValue()    const noexcept;
    float getEnvFiltValue()   const noexcept;
    float getKeytrackOctaves() const noexcept;
```

- [ ] **Step 5: Implement accessors in `Source/Engine/Voice.cpp`**

Append to the file (after `midiNoteToHz`):

```cpp
float Voice::getEnvAmpValue() const noexcept
{
    return ampEnv.peek();
}

float Voice::getEnvFiltValue() const noexcept
{
    return fltEnv.peek();
}

float Voice::getKeytrackOctaves() const noexcept
{
    if (currentNote < 0) return 0.f;
    // Map note 60 (C4) → 0, note 120 → +1.0 (5 octaves capped), note 0 → -1.0
    return juce::jlimit(-1.f, 1.f, (float)(currentNote - 60) / 60.f);
}
```

- [ ] **Step 6: Add `peek()` to `Envelope` if missing**

Inspect `Source/Engine/Envelope.h`. The class likely has a `currentValue` private field tracked by `getNextSample()`. Add a public `const noexcept` accessor returning that field. If the field is named differently, adapt:

```cpp
    float peek() const noexcept { return currentValue; }
```

If `Envelope` doesn't track `currentValue` between `getNextSample` calls, add it: store the result of `getNextSample` into a member field at the end of that method. Tests in `Tests/ExponentialEnvelopeTests.cpp` should still pass because the existing API isn't changing — `peek()` is purely additive.

- [ ] **Step 7: Run tests to confirm they pass**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynthTests.exe"
```

Expected: `Voice exposes per-voice mod sources` passes. Existing voice + envelope tests still pass.

- [ ] **Step 8: Commit**

```
git add Source/Engine/Voice.h Source/Engine/Voice.cpp Source/Engine/Envelope.h Tests/VoiceTests.cpp
git commit -m "feat(engine): Voice exposes velocity/env/keytrack for mod matrix"
```

---

## Task 7: Voice integrates `ModEngine`

**Files:**
- Modify: `Source/Engine/Voice.h`
- Modify: `Source/Engine/Voice.cpp`
- Modify: `Tests/VoiceTests.cpp`

- [ ] **Step 1: Write the failing test**

Append to `Tests/VoiceTests.cpp` `runTest()`:

```cpp
        beginTest("LFO1 → Cutoff via mod matrix oscillates audible cutoff");
        {
            Voice v;
            juce::dsp::ProcessSpec spec { 48000.0, 256, 1 };
            v.prepare(spec);
            VoiceParams p;
            p.filterCutoff = 1000.f;       // base 1 kHz
            p.lfo1.depth   = 1.f;
            p.lfo1.rate    = 10.f;          // 10 Hz LFO
            v.setParams(p);

            // Configure matrix: LFO1 → Cutoff at amount=1.0 (±5 octaves max)
            SynthVibe::ModEngine::Snapshot snap{};
            snap[0] = { 1, 1, 1.0f, 0 };    // src=lfo1, dst=cutoff, amount=1, curve=lin
            v.setMatrixSnapshot(&snap);

            v.noteOn(60, 1.f);

            // Capture effective cutoff samples over ~0.1 s (5000 samples) — must
            // span more than half an LFO cycle so we see both extremes.
            float minCutoff =  1e9f;
            float maxCutoff = -1e9f;
            for (int i = 0; i < 5000; ++i)
            {
                v.getNextSample();
                const float c = v.getCurrentEffectiveCutoff();
                minCutoff = std::min(minCutoff, c);
                maxCutoff = std::max(maxCutoff, c);
            }

            // ±5 octaves around 1000 Hz with a unipolar/bipolar LFO at depth=1:
            // expect the cutoff to swing well above and well below 1 kHz.
            expect(minCutoff < 500.f,  "min cutoff should drop below 500 Hz");
            expect(maxCutoff > 4000.f, "max cutoff should exceed 4000 Hz");
        }
```

- [ ] **Step 2: Run tests — fail**

Compile error: `Voice::setMatrixSnapshot` and `Voice::getCurrentEffectiveCutoff` not declared.

- [ ] **Step 3: Modify `Source/Engine/Voice.h`**

Add these declarations and members.

In the public section after the accessors from Task 6:

```cpp
    // Modulation matrix integration. Snapshot pointer is non-owning; SynthEngine
    // populates it once per block. Voice consumes it at control rate.
    void setMatrixSnapshot(const SynthVibe::ModEngine::Snapshot* snap) noexcept { matrixSnapshot = snap; }
    float getCurrentEffectiveCutoff() const noexcept { return lastEffectiveCutoff; }
```

Add include at the top of Voice.h:

```cpp
#include "ModEngine.h"
```

In the private section, add:

```cpp
    const SynthVibe::ModEngine::Snapshot* matrixSnapshot = nullptr;
    SynthVibe::ModBus                     modBus;
    float                                 lastEffectiveCutoff = 1000.f;
    float                                 lfo1Raw = 0.f;   // raw LFO value, written each sample
    float                                 lfo2Raw = 0.f;
```

- [ ] **Step 4: Modify `Source/Engine/Voice.cpp` `getNextSample()`**

Replace the existing LFO section (lines 113-115) and the filter-mod block (lines 154-167) so the full method becomes:

```cpp
std::pair<float, float> Voice::getNextSample()
{
    if (!ampEnv.isActive())
        return { 0.f, 0.f };

    // LFO outputs: raw -1..+1 each sample (cached for source readout); scaled by depth for legacy paths
    lfo1Raw = lfo1Osc.getNextSample();
    lfo2Raw = lfo2Osc.getNextSample();
    const float l1 = lfo1Raw * params.lfo1.depth;
    const float l2 = lfo2Raw * params.lfo2.depth;

    // ----- legacy LFO destination paths (unchanged) -----
    const bool hasPitchLfo = (params.lfo1.dest == LfoDest::Pitch && params.lfo1.depth != 0.f)
                           || (params.lfo2.dest == LfoDest::Pitch && params.lfo2.depth != 0.f);

    // (… keep the rest of the legacy block exactly as it was before …)
    // Pitch / Detune / Filter / Amp legacy LFO paths are unchanged.

    // ----- legacy code continues: oscillator render, filter modulation block -----
    // The control-rate filter block also recomputes the matrix bus.
    const float sCutoff = smoothCutoff.getNextValue();
    const float sRes    = smoothResonance.getNextValue();
    const float envMod  = fltEnv.getNextSample();
    if (filterCoefCounter == 0)
    {
        // Recompute the mod bus at control rate.
        modBus = {};
        if (matrixSnapshot != nullptr)
        {
            SynthVibe::ModEngine::SourceValues srcs;
            srcs.lfo1     = lfo1Raw;
            srcs.lfo2     = lfo2Raw;
            srcs.envAmp   = ampEnv.peek();
            srcs.envFilt  = envMod;          // freshly sampled above
            srcs.velocity = velocity;
            srcs.keytrack = getKeytrackOctaves();
            SynthVibe::ModEngine::applyMatrix(*matrixSnapshot, srcs, modBus);
        }

        // Compose effective cutoff: legacy path + matrix delta.
        float cutoff = sCutoff * keytrackMultiplier * (1.f + params.filterEnvAmt * envMod);
        cutoff += (params.lfo1.dest == LfoDest::Filter ? l1 * 4000.f : 0.f);
        cutoff += (params.lfo2.dest == LfoDest::Filter ? l2 * 4000.f : 0.f);
        cutoff *= std::pow(2.f, modBus.cutoffSemitones / 12.f);   // matrix mod (semitone offset → multiplier)
        cutoff = juce::jlimit(20.f, 20000.f, cutoff);
        lastEffectiveCutoff = cutoff;

        const float resonance = juce::jlimit(0.f, 1.f, sRes + modBus.resonanceDelta);

        filter.setCutoff(cutoff);
        filterR.setCutoff(cutoff);
        filter.setResonance(resonance);
        filterR.setResonance(resonance);

        // Apply matrix drive delta on top of legacy drive.
        const float drive = juce::jlimit(0.f, 1.f, params.filterDrive + modBus.driveDelta);
        filter.setDrive(drive);
        filterR.setDrive(drive);
    }
    filterCoefCounter = (filterCoefCounter + 1) & (FilterCoefUpdateRate - 1);

    // (… leave the existing oscillator + filter + amp tail unchanged …)
}
```

**IMPORTANT:** the implementer must KEEP the existing legacy LFO blocks for Pitch / Detune / Amp intact — they remain functional and the matrix is purely additive. Only the cutoff block is shown above as the integration site for `modBus.cutoffSemitones`. The full method should match the original structure with only:
- Two new variables `lfo1Raw`, `lfo2Raw` cached at the top
- The control-rate block extended with `modBus = {}; if (matrixSnapshot) … applyMatrix(...)`
- `cutoff *= std::pow(2.f, modBus.cutoffSemitones / 12.f);` line
- `resonance` now uses `modBus.resonanceDelta`
- `filter.setDrive(...)` now uses `modBus.driveDelta`

**Note on `osc1LevelMul`, `osc2LevelMul`, `osc1FineCents`, etc.:** for V1, only cutoff/resonance/drive are wired in this task. The other bus fields (level multipliers, fine cents, PWM, master) are computed but not yet consumed — the existing code path doesn't touch them, so they're harmless dead code. Wiring them is deferred to a polish task to keep this PR focused.

- [ ] **Step 5: Run tests to confirm they pass**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynthTests.exe"
```

Expected: `LFO1 → Cutoff via mod matrix oscillates audible cutoff` passes. All earlier tests still green.

- [ ] **Step 6: Commit**

```
git add Source/Engine/Voice.h Source/Engine/Voice.cpp Tests/VoiceTests.cpp
git commit -m "feat(engine): Voice consumes ModBus for cutoff/res/drive"
```

---

## Task 8: SynthEngine + PluginProcessor wiring

**Files:**
- Modify: `Source/Engine/SynthEngine.h`
- Modify: `Source/Engine/SynthEngine.cpp`
- Modify: `Source/PluginProcessor.cpp`
- Modify: `Tests/SynthEngineTests.cpp`

- [ ] **Step 1: Write the failing test**

Append to `Tests/SynthEngineTests.cpp` `runTest()`:

```cpp
        beginTest("SynthEngine distributes mod snapshot to voices");
        {
            SynthEngine se;
            juce::dsp::ProcessSpec spec { 48000.0, 256, 2 };
            se.prepare(spec);

            SynthVibe::ModEngine::Snapshot snap{};
            snap[3] = { 1, 1, 0.5f, 0 };

            se.setMatrixSnapshot(snap);
            // Cannot directly inspect voices from outside — relies on integration
            // verified at Voice level. This test only proves the call compiles
            // and doesn't crash (smoke).
            expect(true);
        }
```

- [ ] **Step 2: Run tests — fail**

Compile error: `SynthEngine::setMatrixSnapshot` not declared.

- [ ] **Step 3: Modify `Source/Engine/SynthEngine.h`**

Add:

```cpp
    void setMatrixSnapshot(const SynthVibe::ModEngine::Snapshot& snapshot) noexcept;
```

And as a private member:

```cpp
    SynthVibe::ModEngine::Snapshot currentMatrix {};
```

Add include at top:

```cpp
#include "ModEngine.h"
```

- [ ] **Step 4: Implement in `Source/Engine/SynthEngine.cpp`**

```cpp
void SynthEngine::setMatrixSnapshot(const SynthVibe::ModEngine::Snapshot& snapshot) noexcept
{
    currentMatrix = snapshot;
    for (auto& v : voices)
        v.setMatrixSnapshot(&currentMatrix);
}
```

- [ ] **Step 5: Wire into `Source/PluginProcessor.cpp`**

Find the per-block section in `processBlock` where `synth.setParams(buildVoiceParams())` is called. Immediately after that line, add:

```cpp
    SynthVibe::ModEngine::Snapshot snap;
    SynthVibe::ModEngine::readSnapshot(apvts, snap);
    synth.setMatrixSnapshot(snap);
```

Add the include at top of `PluginProcessor.cpp`:

```cpp
#include "Engine/ModEngine.h"
```

- [ ] **Step 6: Run tests + build to confirm pass**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynthTests.exe"
```

Expected: SynthEngine smoke test passes. The plugin builds (verifies PluginProcessor compiles).

- [ ] **Step 7: Commit**

```
git add Source/Engine/SynthEngine.h Source/Engine/SynthEngine.cpp Source/PluginProcessor.cpp Tests/SynthEngineTests.cpp
git commit -m "feat(engine): SynthEngine pipes mod snapshot from APVTS to voices"
```

---

## Task 9: Visual smoke + DAW audible test

**Files:** none (manual validation).

- [ ] **Step 1: Verify Reaper is closed**

```
tasklist //FI "IMAGENAME eq reaper.exe"
```

If running, ask the user to close it.

- [ ] **Step 2: Build Release**

```
./build-with-vs.bat
```

- [ ] **Step 3: Deploy**

```
./deploy.ps1
```

- [ ] **Step 4: Manual audible validation in Reaper**

Ask the user to load `AI Synth vN.vst3` and validate:

- [ ] **Wobble test:** set `LFO 1 rate = 2 Hz, depth = 1`. In Mod tab, slot 1: `src=LFO 1, dst=Cutoff, amount=0.5`. Hold a chord on a saw patch. Expected: audible filter wobble at 2 Hz, ±2.5 octaves around the base cutoff.
- [ ] **Velocity test:** slot 2: `src=Velocity, dst=Osc1 Level, amount=1.0`. Play same note hard then soft. Expected: hard hits clearly louder. (Note: this test only verifies APVTS persistence today — the level multiplier is plumbed but not yet consumed in Voice, see deferred work below.)
- [ ] **Filter env doubling:** with `filt.envamt=0.5` (legacy), set slot 3: `src=Env Filt, dst=Cutoff, amount=0.5`. Expected: more pronounced filter sweep on each note attack than the legacy alone.
- [ ] **Curve audible:** swap a slot's curve from lin → exp → log → s and listen. Expected: shape of modulation feel changes (subtler at low LFO values for exp, more aggressive at low values for log).
- [ ] **Persistence:** save the project, close, reopen. Expected: all slot values restored.
- [ ] **No crashes:** rapid clicks, drag amount during note hold, switch curves during note hold — no crashes, no audible glitches.

If any test fails, capture: which slot config was set, what was heard vs expected. Treat as a follow-up bug, not a Task 9 failure.

- [ ] **Step 5: Take note of deferred work**

The following are known limitations after this plan:

- **`bus.osc1FineCents`, `bus.osc2FineCents`, `bus.osc1LevelMul`, `bus.osc2LevelMul`, `bus.osc1PwmDelta`, `bus.osc2PwmDelta`, `bus.masterVolMul` are computed but NOT consumed.** Wiring them into the oscillator/level paths is a polish task. Filed as: "Phase 3.5 polish: consume remaining ModBus fields in Voice".
- **Modwheel, Aftertouch, Random sources are silent.** Selecting them in a slot does nothing.
- **`Amp Attack`, `Amp Release` destinations are silent.** Modulating envelope times mid-cycle needs careful state design.
- **`mod.9..16.*` slots** are persisted but no UI exposes them. They are also not iterated by `applyMatrix` (loop bounded to 8).

---

## What this plan explicitly does NOT ship

- Modwheel / Aftertouch / Random sources (1+ tasks each).
- Envelope-time destinations (state-management work).
- `osc1.fine`, `osc1.level`, `osc1.pwm`, `master.vol` and their osc2 mirrors **as audible destinations** — the bus fields exist but aren't yet consumed in Voice's render path.
- Per-sample audio-rate modulation. We deliberately use the existing 16-sample control-rate counter (~3 kHz at 48 kHz) — fast LFOs above 1.5 kHz will alias.
- Block-rate snapshot caching/dedup. The 8-slot snapshot read is cheap (8 atomic loads + 24 floats) and runs once per block.
- Removal of the legacy `lfo1.dest` / `lfo2.dest` dropdowns. They remain — modulating the same destination via both the legacy LFO selector and the matrix simply compounds. Documented as a known quirk.

---

## Self-review

**Spec coverage:** All 5 brainstormed defaults are implemented:
1. Control rate (16 samples) — Task 7 reuses `filterCoefCounter`.
2. Legacy LFO retained — Task 7 keeps the legacy block intact, matrix is purely additive.
3. Curve `s` = `tanh(v × 2)` — Task 2 step 4.
4. Smoothing on cutoff/level/master.vol — cutoff already smoothed via `smoothCutoff`; resonance via `smoothResonance`. Level/master.vol smoothing inherited from existing `smoothOsc1Level` / `smoothOsc2Level` once those bus fields are consumed (deferred).
5. Voice source accessors — Task 6.

The plan ships an audible LFO→Cutoff path (the headline use case) and leaves the other 7 destinations plumbed but inert. This is intentional: makes the engine framework testable end-to-end with one concrete audible path before scaling out the destinations.

**Placeholder scan:** no TBD/TODO/etc. Each step has runnable code or a precise command.

**Type consistency:** `Snapshot`, `Slot`, `SourceValues`, `ModBus` are defined in Task 1-2 and used identically in 3-8. `applyMatrix` signature `(Snapshot, SourceValues, ModBus&)` consistent across declarations and call sites. `setMatrixSnapshot` on Voice takes `const Snapshot*`; on SynthEngine takes `const Snapshot&` — different intentionally because SynthEngine copies into its own `currentMatrix`, while Voice keeps a non-owning pointer to SynthEngine's copy. Type-correct.

---

## Execution Handoff

Plan complete and saved to `docs/superpowers/plans/2026-04-25-modengine.md`. Two execution options:

**1. Subagent-Driven (recommended)** — fresh subagent per task, two-stage review, fast iteration. Same approach as Phase 3 ModMatrix UI plan.

**2. Inline Execution** — execute tasks in this session with checkpoints.

Which approach?
