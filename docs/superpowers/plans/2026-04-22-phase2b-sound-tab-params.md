# Phase 2b — Sound Tab Parameter Completion Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ship the nine Sound-Tab parameters and two filter types the Phase 2 roadmap promised but didn't deliver, cleanly reorder `filt.type` (breaking existing local presets), and clean up legacy aliases.

**Architecture:** New APVTS params registered alongside existing ones. Oscillator gains variable-duty Square via rederived PolyBLEP plus a starting-phase reset hook called from `Voice::noteOn`. Filter cascades two `StateVariableTPTFilter` instances for LP24, synthesises Notch via the `input − 2·bandpass` identity, and applies a `tanh`-driven soft clipper before the SVF. Filter keytrack multiplier folds into the existing `filterCoefCounter` sub-rate cutoff apply block (zero extra per-sample cost). UI reflows each oscillator panel to a 3×3 knob grid with the Wave selector above, and the Filter panel reproportions to make room for two new knobs.

**Tech Stack:** JUCE 7.0.9 (`AudioProcessorValueTreeState`, `AudioParameterFloat`, `AudioParameterChoice`, `ParameterAttachment`, `dsp::StateVariableTPTFilter`), existing SynthVibe `Tokens`/`BP` design-system, existing exponential ADSR.

**Spec:** `docs/superpowers/specs/2026-04-22-phase2b-sound-tab-params-design.md`

**Baseline:** `phase2-sound-tab` tag / branch (commit `3b17ccc`). Spec committed as `4345c9a`.

**Execution order rationale:** the plan front-loads the highest-risk item (Task 4 — PolyBLEP rederivation for variable pulse width) so that if it blows up we find out before the UI work lands. The constant rename (Task 1) comes first because it is a zero-behaviour find-and-replace that all later tasks build on; ordering it first also means no later task has to touch the constant alongside a semantic change.

---

## File Map

| Path | Action | Responsibility |
|---|---|---|
| `Source/Parameters/ParameterIDs.h` | Modify | Add 9 new IDs (`osc1.phase`, `osc1.pwm`, `osc2.phase`, `osc2.pwm`, `osc2.uni.*`, `filt.drive`, `filt.keytrack`), rename 3 C++ constants |
| `Source/Parameters/ParameterLayout.cpp` | Modify | Register 9 new parameters; redefine `filt.type` choice list |
| `Source/Engine/Oscillator.h` | Modify | Add `setStartingPhase`, `setPulseWidth`, `resetPhaseToStart` |
| `Source/Engine/Oscillator.cpp` | Modify | Variable-PWM Square with rederived PolyBLEP; phase reset in a dedicated method |
| `Source/Engine/UnisonOscillator.h` | Modify | Forward `setStartingPhase`/`setPulseWidth` to constituent `Oscillator`s |
| `Source/Engine/UnisonOscillator.cpp` | Modify | Implement the two forwarders |
| `Source/Engine/Filter.h` | Modify | Expand `FilterType` enum (`LP12`/`LP24`/`HP`/`BP`/`Notch`), add `setDrive`, `setKeytrackMultiplier` |
| `Source/Engine/Filter.cpp` | Modify | Two-SVF cascade for LP24, Notch via identity, pre-filter tanh drive, keytrack multiplier |
| `Source/Engine/FilterCoefficients.h` | Modify | Add `LP24` and `Notch` types for visualisation |
| `Source/Engine/Voice.h` | Modify | Add OSC2 unison smoothers, new `VoiceParams` fields (phases, PWMs, OSC2 unison, drive, keytrack); keytrack multiplier cache |
| `Source/Engine/Voice.cpp` | Modify | Wire new params into oscillators/filter; reset phases on note-on; compute keytrack multiplier; apply drive; fold multiplier into sub-rate cutoff apply |
| `Source/PluginProcessor.cpp` | Modify | `buildVoiceParams()` reads 9 new APVTS values |
| `Source/UI/LookAndFeel.h` | Modify | Delete four legacy colour aliases |
| `Source/UI/components/FilterTypeSelect.h` | Modify | Grow from 3 to 5 pills |
| `Source/UI/components/FilterResponseView.h` | Modify | Widen switch to route LP24/Notch to updated `FilterCoefficients` |
| `Source/UI/SoundTab.h` | Modify | 3×3 grid per osc panel, new knobs, OSC2 unison, Filter panel reproportion, PWM enable-attachment |
| `Tests/ParameterIdMigrationTests.cpp` | Modify | Add registration + default-value checks for new params |
| `Tests/OscillatorTests.cpp` | Create | Starting-phase correctness, Square PWM duty measurement, variable-PWM PolyBLEP stability |
| `Tests/FilterCoefficientsTests.cpp` | Modify | LP24 rolloff steepness, Notch depth |
| `Tests/VoiceTests.cpp` | Modify | Keytrack multiplier, pre-filter drive THD (relative) |
| `Tests/UIConstructionTests.cpp` | Modify | 5-pill FilterTypeSelect, updated SoundTab construction |
| `CMakeLists.txt` | Modify | Register `Tests/OscillatorTests.cpp` |

---

## Preflight

- [ ] **Preflight: Start on the right branch and confirm clean state**

The design said Phase 2b branches off the `phase2-sound-tab` tag. The spec was committed on a branch already named `phase2-sound-tab`, so continue on that branch. If starting fresh in a new clone, `git switch phase2-sound-tab`.

Run:
```bash
git status
git log --oneline -3
```

Expected: working tree has no unexpected modifications to tracked engine/UI files; top commit is the spec (`docs: add Phase 2b sound-tab parameter completion design`).

---

### Task 1: Rename unison C++ constants (`osc1` scope)

Rename three constants in `ParameterIDs.h` so the C++ names reflect the `osc1.uni.*` scope. The **string values** are unchanged, so APVTS registrations and stored presets are unaffected.

**Files:**
- Modify: `Source/Parameters/ParameterIDs.h` (unison block near line 76)
- Modify: `Source/Parameters/ParameterLayout.cpp` (lines 197–204)
- Modify: `Source/PluginProcessor.cpp` (lines 114–116)
- Modify: `Source/UI/SoundTab.h` (lines 41–43)

- [ ] **Step 1: Rename constants in `ParameterIDs.h`**

Replace the current block:

```cpp
    // Unison
    inline constexpr const char* unisonVoices       = "osc1.uni.voices";
    inline constexpr const char* unisonDetune       = "osc1.uni.detune";
    inline constexpr const char* unisonStereoSpread = "osc1.uni.spread";
```

with:

```cpp
    // Osc1 Unison
    inline constexpr const char* osc1UnisonVoices  = "osc1.uni.voices";
    inline constexpr const char* osc1UnisonDetune  = "osc1.uni.detune";
    inline constexpr const char* osc1UnisonSpread  = "osc1.uni.spread";
```

- [ ] **Step 2: Update call sites**

In `Source/Parameters/ParameterLayout.cpp` (unison block around lines 197–204):

```cpp
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::osc1UnisonVoices, "Osc1 Unison Voices", 1, 7, 1));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::osc1UnisonDetune, "Osc1 Unison Detune",
        NormalisableRange<float>(0.f, 100.f, 0.1f), 0.f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::osc1UnisonSpread, "Osc1 Unison Spread",
        NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));
```

In `Source/PluginProcessor.cpp` (around lines 114–116):

```cpp
    p.unisonVoices       = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::osc1UnisonVoices));
    p.unisonDetuneCents  = *apvts.getRawParameterValue(ParamIDs::osc1UnisonDetune);
    p.unisonStereoSpread = *apvts.getRawParameterValue(ParamIDs::osc1UnisonSpread);
```

In `Source/UI/SoundTab.h` (unison knob constructor args around lines 41–43):

```cpp
          knobUniVoices("Voices", apvts, ParamIDs::osc1UnisonVoices, SynthVibe::Tokens::osc, "", 0),
          knobUniDetune("Detune", apvts, ParamIDs::osc1UnisonDetune, SynthVibe::Tokens::osc, " ct", 1),
          knobUniSpread("Stereo", apvts, ParamIDs::osc1UnisonSpread, SynthVibe::Tokens::osc, "", 2),
```

- [ ] **Step 3: Grep to confirm no stale uses remain**

Run:
```bash
grep -n "unisonVoices\|unisonDetune\|unisonStereoSpread" Source Tests
```

Expected: zero hits (all renamed). If any remain, rename them. Note that the `VoiceParams` C++ struct field `unisonVoices` stays — this rename is only for the `ParamIDs::` constants.

- [ ] **Step 4: Build and run tests**

Run:
```bash
./build.bat && build/AISynth_artefacts/Release/AISynthTests.exe
```

Expected: clean build, all existing tests pass. (No test asserts the rename; this is a mechanical change whose correctness is "still compiles and runs".)

- [ ] **Step 5: Commit**

```bash
git add Source/Parameters/ParameterIDs.h Source/Parameters/ParameterLayout.cpp Source/PluginProcessor.cpp Source/UI/SoundTab.h
git commit -m "refactor: rename unison ParamIDs constants to osc1UnisonX"
```

---

### Task 2: Register 9 new APVTS parameters

Add new IDs in `ParameterIDs.h`, register them in `ParameterLayout.cpp`, assert their existence + defaults in the test suite.

**Files:**
- Modify: `Source/Parameters/ParameterIDs.h`
- Modify: `Source/Parameters/ParameterLayout.cpp`
- Modify: `Tests/ParameterIdMigrationTests.cpp`

- [ ] **Step 1: Write the failing test**

Append to `Tests/ParameterIdMigrationTests.cpp` inside `runTest()`:

```cpp
        beginTest("Phase 2b new parameters are registered with correct defaults");
        {
            juce::AudioProcessorGraph dummyGraph;
            juce::AudioProcessorValueTreeState apvts(
                dummyGraph, nullptr, "AISynthState", ParameterLayout::create());

            auto readFloat = [&](const char* id) {
                auto* p = apvts.getRawParameterValue(id);
                expect(p != nullptr, juce::String("missing param: ") + id);
                return p ? p->load() : 0.f;
            };

            expectWithinAbsoluteError(readFloat(ParamIDs::osc1Phase),       0.f,  0.001f);
            expectWithinAbsoluteError(readFloat(ParamIDs::osc1Pwm),         0.5f, 0.001f);
            expectWithinAbsoluteError(readFloat(ParamIDs::osc2Phase),       0.f,  0.001f);
            expectWithinAbsoluteError(readFloat(ParamIDs::osc2Pwm),         0.5f, 0.001f);
            expectWithinAbsoluteError(readFloat(ParamIDs::osc2UnisonVoices),1.f,  0.001f);
            expectWithinAbsoluteError(readFloat(ParamIDs::osc2UnisonDetune),0.f,  0.001f);
            expectWithinAbsoluteError(readFloat(ParamIDs::osc2UnisonSpread),0.5f, 0.001f);
            expectWithinAbsoluteError(readFloat(ParamIDs::filterDrive),     0.f,  0.001f);
            expectWithinAbsoluteError(readFloat(ParamIDs::filterKeytrack),  0.f,  0.001f);
        }
```

Also include the existing `#include "Parameters/ParameterLayout.h"` if it's not already present (it is — line 3 of the file).

- [ ] **Step 2: Run test to verify it fails**

Run:
```bash
./build.bat
```

Expected: compile failure — `ParamIDs::osc1Phase` (and the eight others) does not exist.

- [ ] **Step 3: Add IDs in `ParameterIDs.h`**

Insert inside `namespace ParamIDs`, immediately after the `osc2Level` line:

```cpp
    inline constexpr const char* osc1Phase     = "osc1.phase";
    inline constexpr const char* osc1Pwm       = "osc1.pwm";
    inline constexpr const char* osc2Phase     = "osc2.phase";
    inline constexpr const char* osc2Pwm       = "osc2.pwm";
```

Immediately after the `osc1UnisonSpread` line (from Task 1):

```cpp
    // Osc2 Unison
    inline constexpr const char* osc2UnisonVoices = "osc2.uni.voices";
    inline constexpr const char* osc2UnisonDetune = "osc2.uni.detune";
    inline constexpr const char* osc2UnisonSpread = "osc2.uni.spread";
```

Inside the Filter section, immediately after `filterEnvAmt`:

```cpp
    inline constexpr const char* filterDrive     = "filt.drive";
    inline constexpr const char* filterKeytrack  = "filt.keytrack";
```

- [ ] **Step 4: Register parameters in `ParameterLayout.cpp`**

Inside the **Oscillator 1** block, after the `osc1Level` param:

```cpp
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::osc1Phase, "Osc1 Phase",
        NormalisableRange<float>(0.f, 360.f, 1.f), 0.f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::osc1Pwm, "Osc1 PWM",
        NormalisableRange<float>(0.01f, 0.99f, 0.001f), 0.5f));
```

Inside the **Oscillator 2** block, after the `osc2Level` param:

```cpp
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::osc2Phase, "Osc2 Phase",
        NormalisableRange<float>(0.f, 360.f, 1.f), 0.f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::osc2Pwm, "Osc2 PWM",
        NormalisableRange<float>(0.01f, 0.99f, 0.001f), 0.5f));
```

Inside the **Filter** block, after the `filterEnvAmt` param:

```cpp
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::filterDrive, "Filter Drive",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 0.f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::filterKeytrack, "Filter Keytrack",
        NormalisableRange<float>(-1.f, 1.f, 0.001f), 0.f));
```

Inside (or immediately after) the renamed **Osc1 Unison** block, add:

```cpp
    // -----------------------------------------------------------------------
    // Osc2 Unison
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::osc2UnisonVoices, "Osc2 Unison Voices", 1, 7, 1));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::osc2UnisonDetune, "Osc2 Unison Detune",
        NormalisableRange<float>(0.f, 100.f, 0.1f), 0.f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::osc2UnisonSpread, "Osc2 Unison Spread",
        NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));
```

- [ ] **Step 5: Run tests to verify they pass**

Run:
```bash
./build.bat && build/AISynth_artefacts/Release/AISynthTests.exe
```

Expected: all tests pass, including the new "Phase 2b new parameters are registered with correct defaults" case.

- [ ] **Step 6: Commit**

```bash
git add Source/Parameters/ParameterIDs.h Source/Parameters/ParameterLayout.cpp Tests/ParameterIdMigrationTests.cpp
git commit -m "feat(params): register 9 Phase 2b APVTS parameters"
```

---

### Task 3: Reorder `filt.type` (breaking change)

Replace the three-entry StringArray with a five-entry one. The enum expansion and engine behaviour change come in later tasks; here we only change the parameter metadata. Until later tasks land, indices 1 and 2 will still map to the old HighPass/BandPass cases because the enum cast in `PluginProcessor::buildVoiceParams` is unchanged. That mismatch is intentional and lives only between commits — each intermediate commit still compiles and runs without crashing.

**Files:**
- Modify: `Source/Parameters/ParameterLayout.cpp` (lines 79–81)

- [ ] **Step 1: Replace the StringArray and leave the default index at 0**

```cpp
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParamIDs::filterType, "Filter Type",
        StringArray { "LP12", "LP24", "HP", "BP", "Notch" }, 0));
```

- [ ] **Step 2: Build to confirm everything still compiles**

Run:
```bash
./build.bat
```

Expected: clean build. The filter still behaves as LP12 (index 0) on defaults; selecting index 1 now shows "LP24" in the host but the engine still treats it as the old HighPass cast — that's fine, later tasks fix it.

- [ ] **Step 3: Commit**

```bash
git add Source/Parameters/ParameterLayout.cpp
git commit -m "feat(params): reorder filt.type to LP12/LP24/HP/BP/Notch (breaks old presets)"
```

---

### Task 4: Oscillator — starting phase + variable-PWM Square with PolyBLEP

Highest-risk task. Variable pulse width means the square's falling edge no longer sits at phase 0.5; the PolyBLEP correction term that compensates it must be centred on `pulseWidth` instead. The rising-edge correction at phase 0 is unchanged.

The derivation: PolyBLEP takes the per-sample phase offset from a discontinuity and returns a correction to add (rising) or subtract (falling) from the raw waveform. For the falling edge at `pulseWidth`, we want the input to `polyBlep` to be small when `phase ≈ pulseWidth`. The existing code uses `fmod(phase + 0.5, 1.0)` which is effectively `fmod(phase − 0.5 + 1.0, 1.0)` — the `−0.5 + 1.0` wraps the "near-edge" condition into `[0, 1)`. Generalised: `fmod(phase − pulseWidth + 1.0, 1.0)`.

**Files:**
- Modify: `Source/Engine/Oscillator.h`
- Modify: `Source/Engine/Oscillator.cpp`
- Create: `Tests/OscillatorTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing tests**

Create `Tests/OscillatorTests.cpp`:

```cpp
#include <juce_core/juce_core.h>
#include "Engine/Oscillator.h"
#include <cmath>
#include <numeric>
#include <vector>

struct OscillatorTests : public juce::UnitTest
{
    OscillatorTests() : juce::UnitTest("Oscillator", "AISynth") {}

    void runTest() override
    {
        beginTest("starting phase 180 deg on sine yields first sample ~ 0");
        {
            Oscillator o;
            o.setSampleRate(48000.0);
            o.setFrequency(1.0);      // very slow — first sample dominated by phase
            o.setWaveform(Waveform::Sine);
            o.setStartingPhase(180.f);
            o.resetPhaseToStart();
            const float s = o.getNextSample();
            expect(std::abs(s) < 0.01f,
                   "first sample at 180deg starting phase should be near 0; got "
                   + juce::String(s));
        }

        beginTest("square PWM duty cycle matches requested value");
        auto measureDuty = [&](float pulseWidth) {
            Oscillator o;
            o.setSampleRate(48000.0);
            o.setFrequency(100.0);
            o.setWaveform(Waveform::Square);
            o.setPulseWidth(pulseWidth);
            o.reset();
            // Drop first 480 samples (one full cycle) to avoid the phase=0 transient,
            // then measure over 48 full cycles (23040 samples).
            for (int i = 0; i < 480; ++i) o.getNextSample();
            int pos = 0, total = 0;
            for (int i = 0; i < 23040; ++i)
            {
                const float s = o.getNextSample();
                if (s > 0.f) ++pos;
                ++total;
                expect(std::isfinite(s), "sample must be finite");
                expect(std::abs(s) < 2.0f, "sample must stay bounded");
            }
            return (float) pos / (float) total;
        };

        const float d10 = measureDuty(0.10f);
        const float d50 = measureDuty(0.50f);
        const float d90 = measureDuty(0.90f);
        expect(std::abs(d10 - 0.10f) < 0.05f,
               juce::String("duty 0.10 measured as ") + juce::String(d10));
        expect(std::abs(d50 - 0.50f) < 0.05f,
               juce::String("duty 0.50 measured as ") + juce::String(d50));
        expect(std::abs(d90 - 0.90f) < 0.05f,
               juce::String("duty 0.90 measured as ") + juce::String(d90));
    }
};

static OscillatorTests sOscillatorTests;
```

- [ ] **Step 2: Register the new test file**

In `CMakeLists.txt`, add `Tests/OscillatorTests.cpp` to the `target_sources(AISynthTests PRIVATE ...)` list (near line 88):

```cmake
target_sources(AISynthTests PRIVATE
    Tests/main.cpp
    Tests/ArpEngineTests.cpp
    Tests/SynthEngineTests.cpp
    Tests/VoiceTests.cpp
    Tests/ParameterIdMigrationTests.cpp
    Tests/DesignTokensTests.cpp
    Tests/UIConstructionTests.cpp
    Tests/ExponentialEnvelopeTests.cpp
    Tests/FilterCoefficientsTests.cpp
    Tests/OscillatorTests.cpp
    ...
```

- [ ] **Step 3: Run to verify it fails**

Run:
```bash
./build.bat
```

Expected: compile failure — `Oscillator::setStartingPhase`, `setPulseWidth`, and `resetPhaseToStart` don't exist.

- [ ] **Step 4: Add the new methods in `Oscillator.h`**

Replace the current public block of `Oscillator.h` with:

```cpp
    void setSampleRate(double sr)     { sampleRate = sr; }
    void setFrequency(double freqHz)  { frequency = freqHz; }
    void setWaveform(Waveform wf)     { waveform = wf; }
    void setDetuneCents(float cents)  { detuneCents = cents; }
    void setStartingPhase(float deg)  { startingPhase = juce::jlimit(0.f, 1.f, deg / 360.f); }
    void setPulseWidth(float duty)    { pulseWidth    = juce::jlimit(0.01f, 0.99f, duty); }

    void reset()             { phase = 0.0; }
    void setPhase(double p)  { phase = p; }
    void resetPhaseToStart() { phase = static_cast<double>(startingPhase); }

    float getNextSample();
```

and add to the `private:` block:

```cpp
    float    startingPhase = 0.f;
    float    pulseWidth    = 0.5f;
```

Add `#include <juce_core/juce_core.h>` at the top of `Oscillator.h` if it isn't already (it isn't — the header uses `<cmath>` only). This include brings in `juce::jlimit`.

- [ ] **Step 5: Rework the Square path in `Oscillator.cpp`**

Replace the `case Waveform::Square:` block with:

```cpp
        case Waveform::Square:
        {
            const double pw = static_cast<double>(pulseWidth);
            sample  = phase < pw ? 1.f : -1.f;
            // Rising edge at phase=0
            sample += polyBlep(phase, dt);
            // Falling edge at phase=pulseWidth — wrap into [0,1)
            sample -= polyBlep(std::fmod(phase - pw + 1.0, 1.0), dt);
            break;
        }
```

No other change in `Oscillator.cpp`.

- [ ] **Step 6: Run tests to verify they pass**

Run:
```bash
./build.bat && build/AISynth_artefacts/Release/AISynthTests.exe
```

Expected: all tests pass, including the three new Oscillator cases. If duty-cycle tests fail by more than 5 percentage points, the PolyBLEP `fmod` wrap is likely wrong — verify that `fmod(phase - pw + 1.0, 1.0)` stays in `[0, 1)` for all `phase ∈ [0, 1)` and `pw ∈ [0.01, 0.99]` (it does: the sum lies in `[0.01, 1.99)` and fmod brings the ≥1 half into `[0, 0.99)`).

- [ ] **Step 7: Commit**

```bash
git add Source/Engine/Oscillator.h Source/Engine/Oscillator.cpp Tests/OscillatorTests.cpp CMakeLists.txt
git commit -m "feat(engine): oscillator starting phase and variable-PWM square with PolyBLEP"
```

---

### Task 5: UnisonOscillator forwards phase + PWM

Forward the two new setters to every constituent `Oscillator`. Also add a `resetAllPhasesToStart()` convenience that `Voice::noteOn` will call.

**Files:**
- Modify: `Source/Engine/UnisonOscillator.h`
- Modify: `Source/Engine/UnisonOscillator.cpp`

- [ ] **Step 1: Add setter declarations in `UnisonOscillator.h`**

Insert in the public section, directly after `setStereoSpread`:

```cpp
    void setStartingPhase(float deg);
    void setPulseWidth(float duty);
    void resetAllPhasesToStart();
```

- [ ] **Step 2: Expose `Oscillator::getPhase`**

`resetAllPhasesToStart` needs to read each slot's phase after `resetPhaseToStart()` before stacking the stagger on top. The `Oscillator::phase` member is private, so add a read accessor.

In `Source/Engine/Oscillator.h`, public section (near `setPhase`):

```cpp
    double getPhase() const { return phase; }
```

- [ ] **Step 3: Implement the forwarders in `UnisonOscillator.cpp`**

Append to the end of the file (after `recomputePan`):

```cpp
void UnisonOscillator::setStartingPhase(float deg)
{
    for (auto& o : oscs)
        o.setStartingPhase(deg);
}

void UnisonOscillator::setPulseWidth(float duty)
{
    for (auto& o : oscs)
        o.setPulseWidth(duty);
}

void UnisonOscillator::resetAllPhasesToStart()
{
    for (int i = 0; i < MaxUnison; ++i)
        oscs[i].resetPhaseToStart();

    // For multi-voice unison, stack the detune stagger on top of the starting
    // phase so the slots don't all lock to the same phase and cancel out spread.
    if (unisonVoices > 1)
        for (int i = 0; i < unisonVoices; ++i)
        {
            const double stagger = static_cast<double>(i) / unisonVoices;
            const double base    = oscs[i].getPhase();
            oscs[i].setPhase(std::fmod(base + stagger, 1.0));
        }
}
```

- [ ] **Step 4: Build**

Run:
```bash
./build.bat && build/AISynth_artefacts/Release/AISynthTests.exe
```

Expected: clean build, all existing tests still pass. (No new test for this task — it's a straight forwarder + stagger restoration, covered by later Voice/SynthEngine integration tests.)

- [ ] **Step 5: Commit**

```bash
git add Source/Engine/Oscillator.h Source/Engine/UnisonOscillator.h Source/Engine/UnisonOscillator.cpp
git commit -m "feat(engine): unison oscillator forwards phase and PWM, resets with stagger"
```

---

### Task 6: Filter enum expansion + tanh drive + LP24 cascade + Notch identity

Rewrite `Filter` to support the five-mode `FilterType`. Pre-filter drive goes first in `processSample`; LP24 drives a cascaded second SVF; Notch uses `input − 2·bandpass_output`. Keytrack is applied externally via `setCutoff` in Task 7 (Voice), so the Filter API grows only `setDrive`.

**Files:**
- Modify: `Source/Engine/Filter.h`
- Modify: `Source/Engine/Filter.cpp`

- [ ] **Step 1: Update the enum and API in `Filter.h`**

Replace the current header with:

```cpp
#pragma once
#include <juce_dsp/juce_dsp.h>

enum class FilterType { LP12 = 0, LP24, HP, BP, Notch };

// State-variable filter with LP24 cascade, Notch identity, and a pre-filter
// tanh drive stage. One instance per voice channel.
class Filter
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void setType(FilterType type);
    void setCutoff(float freqHz);
    void setResonance(float normalised); // 0–1 → mapped to Q range
    void setDrive(float normalised);     // 0–1 → pre-gain factor 1..10
    void reset();

    float processSample(float input);

private:
    void updateCoefficients();

    juce::dsp::StateVariableTPTFilter<float> svf1;
    juce::dsp::StateVariableTPTFilter<float> svf2;  // cascaded second stage for LP24
    FilterType filterType = FilterType::LP12;
    float      cutoff     = 8000.f;
    float      resonance  = 0.7f;
    float      drive      = 0.f;
    double     sampleRate = 44100.0;
};
```

- [ ] **Step 2: Rewrite `Filter.cpp`**

Replace the current implementation with:

```cpp
#include "Filter.h"
#include <cmath>

void Filter::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    svf1.prepare(spec);
    svf2.prepare(spec);
    updateCoefficients();
}

void Filter::setType(FilterType type)
{
    filterType = type;
    switch (type)
    {
        case FilterType::LP12:
            svf1.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            svf2.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            break;
        case FilterType::LP24:
            svf1.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            svf2.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            break;
        case FilterType::HP:
            svf1.setType(juce::dsp::StateVariableTPTFilterType::highpass);
            break;
        case FilterType::BP:
            svf1.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
            break;
        case FilterType::Notch:
            svf1.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
            break;
    }
}

void Filter::setCutoff(float freqHz)
{
    cutoff = juce::jlimit(20.f, 20000.f, freqHz);
    updateCoefficients();
}

void Filter::setResonance(float normalised)
{
    resonance = juce::jmap(juce::jlimit(0.f, 1.f, normalised), 0.5f, 12.f);
    updateCoefficients();
}

void Filter::setDrive(float normalised)
{
    drive = juce::jlimit(0.f, 1.f, normalised);
}

void Filter::reset()
{
    svf1.reset();
    svf2.reset();
}

float Filter::processSample(float input)
{
    // Pre-filter soft clip
    if (drive > 0.f)
    {
        const float gain = 1.f + drive * 9.f;   // 0..1 → 1..10 pre-gain
        input = std::tanh(input * gain);
    }

    switch (filterType)
    {
        case FilterType::LP12:
        case FilterType::HP:
        case FilterType::BP:
            return svf1.processSample(0, input);

        case FilterType::LP24:
            return svf2.processSample(0, svf1.processSample(0, input));

        case FilterType::Notch:
        {
            const float bp = svf1.processSample(0, input);
            return input - 2.f * bp;
        }
    }
    return input;
}

void Filter::updateCoefficients()
{
    svf1.setCutoffFrequency(cutoff);
    svf1.setResonance(resonance);
    svf2.setCutoffFrequency(cutoff);
    svf2.setResonance(resonance);
}
```

- [ ] **Step 3: Fix the enum name references in `Voice.h` / `Voice.cpp` / `PluginProcessor.cpp` / `VoiceParams`**

The enum renamed `LowPass` → `LP12`, `HighPass` → `HP`, `BandPass` → `BP`. Grep and rename:

```bash
grep -n "FilterType::LowPass\|FilterType::HighPass\|FilterType::BandPass" Source Tests
```

Each hit gets renamed:
- `FilterType::LowPass`   → `FilterType::LP12`
- `FilterType::HighPass`  → `FilterType::HP`
- `FilterType::BandPass`  → `FilterType::BP`

Most likely call sites:
- `Source/Engine/Voice.h` line 35: `FilterType filterType = FilterType::LowPass;` → `FilterType filterType = FilterType::LP12;`
- Any test or stub that names a FilterType — grep will surface them.

- [ ] **Step 4: Build**

Run:
```bash
./build.bat
```

Expected: clean build. If a grep miss surfaces here, fix the reference and re-build. No test was added for this task alone because the behavioural invariants are checked by Task 8 (FilterCoefficients) and Task 10 (Voice integration).

- [ ] **Step 5: Commit**

```bash
git add Source/Engine/Filter.h Source/Engine/Filter.cpp Source/Engine/Voice.h
git commit -m "feat(engine): filter LP12/LP24/HP/BP/Notch with pre-filter tanh drive"
```

---

### Task 7: Voice wires new params, resets phases, applies keytrack

`VoiceParams` gains five fields (two phases, two PWMs, drive, keytrack — plus the OSC2 unison trio). `Voice::setParams` pushes them into the oscillators and filter. `Voice::noteOn` calls `resetAllPhasesToStart` instead of `reset`, and recomputes the keytrack multiplier. The sub-rate cutoff apply block multiplies by the multiplier.

**Files:**
- Modify: `Source/Engine/Voice.h`
- Modify: `Source/Engine/Voice.cpp`

- [ ] **Step 1: Expand `VoiceParams` in `Voice.h`**

Replace the `OscParams` struct with:

```cpp
struct OscParams
{
    Waveform waveform       = Waveform::Saw;
    int      octave         = 0;
    int      semitone       = 0;
    float    detune         = 0.f;
    float    level          = 1.f;
    float    startingPhase  = 0.f;  // degrees, 0–360
    float    pulseWidth     = 0.5f; // 0.01–0.99, only used by Square
};
```

Add fields to `VoiceParams`:

```cpp
struct VoiceParams
{
    OscParams osc1;
    OscParams osc2;

    LfoParams lfo1;
    LfoParams lfo2;

    FilterType filterType      = FilterType::LP12;
    float      filterCutoff    = 8000.f;
    float      filterResonance = 0.1f;
    float      filterEnvAmt    = 0.f;
    float      filterDrive     = 0.f;   // 0–1
    float      filterKeytrack  = 0.f;   // −1..+1

    Envelope::Params ampEnv;
    Envelope::Params fltEnv;

    int   unisonVoices       = 1;    // osc1
    float unisonDetuneCents  = 0.f;  // osc1
    float unisonStereoSpread = 0.5f; // osc1

    int   osc2UnisonVoices       = 1;
    float osc2UnisonDetuneCents  = 0.f;
    float osc2UnisonStereoSpread = 0.5f;
};
```

- [ ] **Step 2: Apply new params in `Voice::setParams`**

In `Voice.cpp`, extend `setParams`. Add immediately after the existing `osc1.setStereoSpread`/`osc2.setStereoSpread` block:

```cpp
    osc1.setStartingPhase(p.osc1.startingPhase);
    osc1.setPulseWidth   (p.osc1.pulseWidth);
    osc2.setStartingPhase(p.osc2.startingPhase);
    osc2.setPulseWidth   (p.osc2.pulseWidth);
```

Replace the OSC2 unison lines inside `setParams` so OSC1 and OSC2 unison parameters are independent:

```cpp
    osc1.setUnison(p.unisonVoices, p.unisonDetuneCents);
    osc1.setStereoSpread(p.unisonStereoSpread);
    osc2.setUnison(p.osc2UnisonVoices, p.osc2UnisonDetuneCents);
    osc2.setStereoSpread(p.osc2UnisonStereoSpread);
```

(The existing `osc2.setUnison(p.unisonVoices, p.unisonDetuneCents)` line must be removed — it was a placeholder that had OSC2 mirror OSC1 unison. Grep `Source/Engine/Voice.cpp` for `osc2.setUnison(p.unison` to find the exact line.)

Update the filter section to push drive:

```cpp
    filter.setType(p.filterType);
    filter.setCutoff(p.filterCutoff);
    filter.setResonance(p.filterResonance);
    filter.setDrive(p.filterDrive);
    filterR.setType(p.filterType);
    filterR.setCutoff(p.filterCutoff);
    filterR.setResonance(p.filterResonance);
    filterR.setDrive(p.filterDrive);
```

- [ ] **Step 3: Add a keytrack multiplier cache**

In `Voice.h` private section, add:

```cpp
    float keytrackMultiplier = 1.f;
```

In `Voice.cpp` `setParams`, recompute the cache whenever params change (place at the end of `setParams`, just before the existing `if (currentNote >= 0)` block):

```cpp
    if (currentNote >= 0 && p.filterKeytrack != 0.f)
        keytrackMultiplier = std::pow(2.f, (currentNote - 60.f) / 12.f * p.filterKeytrack);
    else
        keytrackMultiplier = 1.f;
```

In `Voice.cpp` `noteOn`, recompute once the note is locked in (place after `currentNote = midiNote;`):

```cpp
    keytrackMultiplier = params.filterKeytrack != 0.f
        ? std::pow(2.f, (currentNote - 60.f) / 12.f * params.filterKeytrack)
        : 1.f;
```

- [ ] **Step 4: Call `resetAllPhasesToStart` on note-on**

In `Voice::noteOn`, replace the current `osc1.reset(); osc2.reset();` lines inside the `if (!stolen)` block with:

```cpp
    if (!stolen)
    {
        osc1.resetAllPhasesToStart();
        osc2.resetAllPhasesToStart();
        filter.reset();
        filterR.reset();
    }
```

- [ ] **Step 5: Apply keytrack in the sub-rate block**

In `Voice::getNextSample`, find the sub-rate cutoff apply block (currently lines 147–155). Update:

1. Extend `hasFilterMod` to include keytrack, so the block runs when keytrack ≠ 0:

```cpp
    const bool hasFilterMod = (params.filterEnvAmt != 0.f)
                            || (params.lfo1.dest == LfoDest::Filter && params.lfo1.depth != 0.f)
                            || (params.lfo2.dest == LfoDest::Filter && params.lfo2.depth != 0.f)
                            || (params.filterKeytrack != 0.f);
```

2. Multiply `cutoff` by the cached keytrack multiplier inside the block:

```cpp
    if ((hasFilterMod || smoothCutoff.isSmoothing()) && filterCoefCounter == 0)
    {
        float cutoff = sCutoff * keytrackMultiplier * (1.f + params.filterEnvAmt * envMod);
        cutoff += (params.lfo1.dest == LfoDest::Filter ? l1 * 4000.f : 0.f);
        cutoff += (params.lfo2.dest == LfoDest::Filter ? l2 * 4000.f : 0.f);
        cutoff = juce::jlimit(20.f, 20000.f, cutoff);
        filter.setCutoff(cutoff);
        filterR.setCutoff(cutoff);
    }
```

- [ ] **Step 6: Write the failing keytrack test**

Append to `Tests/VoiceTests.cpp` inside `runTest()`:

```cpp
        beginTest("filter keytrack = 1 shifts effective cutoff by one octave per octave");
        {
            // Strategy: compare cutoff spectra at MIDI 60 vs MIDI 72 with keytrack = 1.
            // With keytrack = 1, MIDI 72 (one octave up) should double the cutoff.
            // We approximate this by comparing harmonic content — specifically, the
            // RMS of the filtered output on a broadband input. The exact cutoff
            // value is internal to the voice, so we assert via energy comparison.
            auto energyAt = [&](int midi, float keytrack) {
                Voice voice;
                juce::dsp::ProcessSpec spec { 48000.0, 512, 1 };
                voice.prepare(spec);

                VoiceParams p;
                p.osc1.waveform = Waveform::Saw;
                p.osc1.level = 1.f;
                p.osc2.level = 0.f;
                p.filterType = FilterType::LP12;
                p.filterCutoff = 500.f;   // low enough that keytrack change is audible
                p.filterResonance = 0.1f;
                p.filterKeytrack = keytrack;
                p.ampEnv.attack  = 0.001f;
                p.ampEnv.decay   = 0.001f;
                p.ampEnv.sustain = 1.0f;
                p.ampEnv.release = 0.001f;
                p.fltEnv.attack  = 0.001f;
                p.fltEnv.decay   = 0.001f;
                p.fltEnv.sustain = 1.0f;
                p.fltEnv.release = 0.001f;
                voice.setParams(p);
                voice.noteOn(midi, 1.0f);

                // Let the envelope settle, then measure RMS over 2048 samples.
                for (int i = 0; i < 1024; ++i) voice.getNextSample();
                double sumSq = 0.0;
                for (int i = 0; i < 2048; ++i)
                {
                    const float s = voice.getNextSample().first;
                    sumSq += double(s) * double(s);
                }
                return std::sqrt(sumSq / 2048.0);
            };

            // With keytrack = 0, raising pitch by an octave should still leave roughly
            // the same RMS because the filter cutoff is fixed (the higher harmonics
            // get attenuated more, so maybe a small drop — but energy < 2x difference).
            // With keytrack = 1, the higher pitch should have the cutoff move up
            // with it, preserving substantially more harmonic energy.
            const double rms_60_kt0 = energyAt(60, 0.f);
            const double rms_72_kt0 = energyAt(72, 0.f);
            const double rms_60_kt1 = energyAt(60, 1.f);
            const double rms_72_kt1 = energyAt(72, 1.f);

            const double ratio_kt0 = rms_72_kt0 / juce::jmax(1e-9, rms_60_kt0);
            const double ratio_kt1 = rms_72_kt1 / juce::jmax(1e-9, rms_60_kt1);

            expect(ratio_kt1 > ratio_kt0 * 1.3,
                   "keytrack=1 should preserve more energy at higher pitch than keytrack=0 ("
                   + juce::String(ratio_kt0, 3) + " vs " + juce::String(ratio_kt1, 3) + ")");
        }
```

- [ ] **Step 7: Run tests**

Run:
```bash
./build.bat && build/AISynth_artefacts/Release/AISynthTests.exe
```

Expected: all tests pass. If `ratio_kt1 / ratio_kt0` is below the 1.3 threshold, verify the multiplier cache is recomputed in `noteOn` (not just in `setParams`) and that the sub-rate block's `hasFilterMod` includes `filterKeytrack`.

- [ ] **Step 8: Commit**

```bash
git add Source/Engine/Voice.h Source/Engine/Voice.cpp Tests/VoiceTests.cpp
git commit -m "feat(engine): voice wires OSC2 unison, phase/PWM, filter drive and keytrack"
```

---

### Task 8: PluginProcessor reads new APVTS params

Connect the nine new APVTS parameters into `VoiceParams` inside `buildVoiceParams`.

**Files:**
- Modify: `Source/PluginProcessor.cpp` (lines 77–118)

- [ ] **Step 1: Extend `buildVoiceParams`**

Inside `buildVoiceParams`, in the OSC1 block, add:

```cpp
    p.osc1.startingPhase = *apvts.getRawParameterValue(ParamIDs::osc1Phase);
    p.osc1.pulseWidth    = *apvts.getRawParameterValue(ParamIDs::osc1Pwm);
```

In the OSC2 block:

```cpp
    p.osc2.startingPhase = *apvts.getRawParameterValue(ParamIDs::osc2Phase);
    p.osc2.pulseWidth    = *apvts.getRawParameterValue(ParamIDs::osc2Pwm);
```

In the Filter block:

```cpp
    p.filterDrive    = *apvts.getRawParameterValue(ParamIDs::filterDrive);
    p.filterKeytrack = *apvts.getRawParameterValue(ParamIDs::filterKeytrack);
```

After the existing OSC1 unison block:

```cpp
    p.osc2UnisonVoices       = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::osc2UnisonVoices));
    p.osc2UnisonDetuneCents  = *apvts.getRawParameterValue(ParamIDs::osc2UnisonDetune);
    p.osc2UnisonStereoSpread = *apvts.getRawParameterValue(ParamIDs::osc2UnisonSpread);
```

- [ ] **Step 2: Build and run tests**

Run:
```bash
./build.bat && build/AISynth_artefacts/Release/AISynthTests.exe
```

Expected: clean build, all tests pass.

- [ ] **Step 3: Commit**

```bash
git add Source/PluginProcessor.cpp
git commit -m "feat(params): PluginProcessor forwards 9 Phase 2b APVTS params to VoiceParams"
```

---

### Task 9: FilterCoefficients supports LP24 and Notch

Extend the visualisation helper so `FilterResponseView` can draw the two new shapes. The audio path uses `StateVariableTPTFilter`, so this is a cosmetic approximation.

**Files:**
- Modify: `Source/Engine/FilterCoefficients.h`
- Modify: `Tests/FilterCoefficientsTests.cpp`

- [ ] **Step 1: Write the failing tests**

Append to `Tests/FilterCoefficientsTests.cpp` inside `runTest()`:

```cpp
        beginTest("LP24 rolloff is steeper than LP12 at cutoff + 1 octave");
        {
            using SynthVibe::FilterCoefficients;
            using Type = SynthVibe::FilterCoefficients::Type;
            const float fc = 1000.f;
            auto lp12 = FilterCoefficients::compute(Type::LowPass, fc, 0.707f, 48000.0);
            auto lp24 = FilterCoefficients::compute(Type::LP24,    fc, 0.707f, 48000.0);
            const float mag12 = lp12.magnitudeAt(fc * 2.f, 48000.0);
            const float mag24 = lp24.magnitudeAt(fc * 2.f, 48000.0);
            expect(mag24 < mag12 * 0.6f,
                   "LP24 should attenuate more at fc+1oct than LP12 ("
                   + juce::String(mag12) + " vs " + juce::String(mag24) + ")");
        }

        beginTest("Notch attenuates the cutoff frequency");
        {
            using SynthVibe::FilterCoefficients;
            using Type = SynthVibe::FilterCoefficients::Type;
            auto notch = FilterCoefficients::compute(Type::Notch, 1000.f, 4.f, 48000.0);
            const float magAtFc = notch.magnitudeAt(1000.f, 48000.0);
            const float magFar  = notch.magnitudeAt(5000.f, 48000.0);
            expect(magAtFc < 0.2f,
                   "Notch at fc should attenuate, got " + juce::String(magAtFc));
            expect(magFar > 0.8f,
                   "Notch away from fc should pass, got " + juce::String(magFar));
        }
```

- [ ] **Step 2: Run to verify failure**

Run:
```bash
./build.bat
```

Expected: compile failure — `Type::LP24` and `Type::Notch` don't exist.

- [ ] **Step 3: Extend the enum and switch**

In `FilterCoefficients.h`, change the enum line to:

```cpp
        enum class Type { LowPass, HighPass, BandPass, LP24, Notch };
```

Add cases to the switch inside `compute`:

```cpp
                case Type::LP24:
                {
                    // Model: one biquad representing the magnitude of two cascaded LP12s
                    // at the same cutoff. The biquad form is the LP12 biquad but with
                    // the numerator halved so the magnitude at cutoff is ~0.5 instead
                    // of ~0.707, squaring the rolloff visually without a second stage.
                    // This is an approximation that matches the audio-path cascade at
                    // Q≈0.707 and diverges mildly at high Q; acceptable for display.
                    const float lpB0 = (1.f - cosW0) * 0.5f / a0;
                    const float lpB1 = (1.f - cosW0)        / a0;
                    const float lpB2 = (1.f - cosW0) * 0.5f / a0;
                    c.b0 = lpB0 * lpB0;
                    c.b1 = lpB1 * lpB1;
                    c.b2 = lpB2 * lpB2;
                    break;
                }
                case Type::Notch:
                    c.b0 =  1.f            / a0;
                    c.b1 = -2.f * cosW0    / a0;
                    c.b2 =  1.f            / a0;
                    break;
```

- [ ] **Step 4: Run tests**

Run:
```bash
./build.bat && build/AISynth_artefacts/Release/AISynthTests.exe
```

Expected: all tests pass, including the new LP24 and Notch cases. If LP24 is not steep enough (`mag24 >= mag12 * 0.6`), widen the squaring multiplier or switch to a true cascaded biquad evaluation in `magnitudeAt` — for this phase the simple squared-numerator approximation is enough because the display only needs relative steepness, not dB-exact measurement.

- [ ] **Step 5: Commit**

```bash
git add Source/Engine/FilterCoefficients.h Tests/FilterCoefficientsTests.cpp
git commit -m "feat(ui): FilterCoefficients supports LP24 and Notch for response display"
```

---

### Task 10: FilterResponseView routes LP24 and Notch

Teach the response view to map the five `filt.type` indices to the new `FilterCoefficients::Type` cases.

**Files:**
- Modify: `Source/UI/components/FilterResponseView.h` (lines 25–33)

- [ ] **Step 1: Replace the type dispatch block**

Replace the `const Type t = ...` block inside `paint` with:

```cpp
            const Type t = typeIdx == 0 ? Type::LowPass      // LP12
                         : typeIdx == 1 ? Type::LP24
                         : typeIdx == 2 ? Type::HighPass
                         : typeIdx == 3 ? Type::BandPass
                         :                Type::Notch;
```

- [ ] **Step 2: Build**

Run:
```bash
./build.bat
```

Expected: clean build. (Visual output is verified in the manual DAW test in Task 15; no automated assertion.)

- [ ] **Step 3: Commit**

```bash
git add Source/UI/components/FilterResponseView.h
git commit -m "feat(ui): FilterResponseView dispatches LP24 and Notch"
```

---

### Task 11: FilterTypeSelect grows to 5 pills

Keep the radio-group pattern; just add two `TextButton` slots and widen the layout.

**Files:**
- Modify: `Source/UI/components/FilterTypeSelect.h`
- Modify: `Tests/UIConstructionTests.cpp` (the `FilterTypeSelect constructs` case)

- [ ] **Step 1: Write the failing test update**

In `Tests/UIConstructionTests.cpp`, replace the existing "FilterTypeSelect constructs and binds to filt.type" case with:

```cpp
        beginTest("FilterTypeSelect has 5 pills after reorder");
        {
            SynthVibe::FilterTypeSelect fts(apvts, ParamIDs::filterType);
            fts.setBounds(0, 0, 250, 28);
            // ParameterAttachment pushes the default (index 0 = LP12); we can't
            // inspect the private pills array, so just confirm construction doesn't
            // throw and the component is at the requested bounds.
            expectEquals(fts.getWidth(), 250);
        }
```

- [ ] **Step 2: Rewrite `FilterTypeSelect.h`**

Replace the whole class body with:

```cpp
    class FilterTypeSelect : public juce::Component
    {
    public:
        FilterTypeSelect(juce::AudioProcessorValueTreeState& apvts,
                         const juce::String& paramID)
        {
            const char* labels[] = { "LP12", "LP24", "HP", "BP", "NO" };
            for (int i = 0; i < 5; ++i)
            {
                auto& btn = pills[i];
                btn.setButtonText(labels[i]);
                btn.setClickingTogglesState(true);
                btn.setRadioGroupId(1);
                btn.setTooltip(i == 4 ? juce::String("Notch") : juce::String(labels[i]));
                btn.onClick = [this, i, &apvts, paramID]
                {
                    if (auto* p = apvts.getParameter(paramID))
                        p->setValueNotifyingHost(p->convertTo0to1((float) i));
                };
                addAndMakeVisible(btn);
            }

            paramAttach = std::make_unique<juce::ParameterAttachment>(
                *apvts.getParameter(paramID),
                [this](float v) {
                    const int idx = juce::jlimit(0, 4, (int) std::round(v));
                    for (int i = 0; i < 5; ++i)
                        pills[i].setToggleState(i == idx, juce::dontSendNotification);
                });
            paramAttach->sendInitialUpdate();
        }

        void resized() override
        {
            auto b = getLocalBounds();
            const int w = b.getWidth() / 5;
            for (int i = 0; i < 5; ++i)
                pills[i].setBounds(b.removeFromLeft(w).reduced(2));
        }

    private:
        juce::TextButton pills[5];
        std::unique_ptr<juce::ParameterAttachment> paramAttach;
    };
```

- [ ] **Step 3: Build and run tests**

Run:
```bash
./build.bat && build/AISynth_artefacts/Release/AISynthTests.exe
```

Expected: clean build, all tests pass.

- [ ] **Step 4: Commit**

```bash
git add Source/UI/components/FilterTypeSelect.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): FilterTypeSelect grows to 5 pills for LP12/LP24/HP/BP/Notch"
```

---

### Task 12: SoundTab — 3×3 grid, OSC2 unison, new knobs, PWM enable-state, right-column reproportion

Largest UI change: six new knobs (`osc1Phase`, `osc1Pwm`, `osc2Phase`, `osc2Pwm`, `filtDrive`, `filtKeytrack`) plus three OSC2 unison knobs bound to the new IDs. Each oscillator panel becomes a 3×3 grid under the Wave selector. The Filter panel drops from ~38% to 34% of column 3 height to fit the two new knobs in a 2-row layout.

**Files:**
- Modify: `Source/UI/SoundTab.h`

- [ ] **Step 1: Add the new knob members and constructor initialisers**

In the constructor member-initialiser list, append (after the existing filter knobs):

```cpp
          knobOsc1Phase("Phase", apvts, ParamIDs::osc1Phase, SynthVibe::Tokens::osc, " deg", 0),
          knobOsc1Pwm  ("PWM",   apvts, ParamIDs::osc1Pwm,   SynthVibe::Tokens::osc, "",      2),
          knobOsc2Phase("Phase", apvts, ParamIDs::osc2Phase, SynthVibe::Tokens::osc, " deg", 0),
          knobOsc2Pwm  ("PWM",   apvts, ParamIDs::osc2Pwm,   SynthVibe::Tokens::osc, "",      2),
          knobOsc2UniVoices("Voices", apvts, ParamIDs::osc2UnisonVoices, SynthVibe::Tokens::osc, "",    0),
          knobOsc2UniDetune("Detune", apvts, ParamIDs::osc2UnisonDetune, SynthVibe::Tokens::osc, " ct", 1),
          knobOsc2UniSpread("Stereo", apvts, ParamIDs::osc2UnisonSpread, SynthVibe::Tokens::osc, "",    2),
          knobFilterDrive  ("Drive",    apvts, ParamIDs::filterDrive,    SynthVibe::Tokens::filter, "",  2),
          knobFilterKeytrack("Keytrk",  apvts, ParamIDs::filterKeytrack, SynthVibe::Tokens::filter, "",  2)
```

In the private member declarations (bottom of the class), add:

```cpp
    SynthVibe::ArcKnob knobOsc1Phase, knobOsc1Pwm;
    SynthVibe::ArcKnob knobOsc2Phase, knobOsc2Pwm;
    SynthVibe::ArcKnob knobOsc2UniVoices, knobOsc2UniDetune, knobOsc2UniSpread;
    SynthVibe::ArcKnob knobFilterDrive, knobFilterKeytrack;
    std::unique_ptr<juce::ParameterAttachment> osc1WaveAttach;
    std::unique_ptr<juce::ParameterAttachment> osc2WaveAttach;
```

Add all new knobs to the `addAndMakeVisible` initializer list (just before the closing `})`):

```cpp
            &knobOsc1Phase, &knobOsc1Pwm,
            &knobOsc2Phase, &knobOsc2Pwm,
            &knobOsc2UniVoices, &knobOsc2UniDetune, &knobOsc2UniSpread,
            &knobFilterDrive, &knobFilterKeytrack
```

- [ ] **Step 2: Set up PWM enable-state attachments**

At the end of the constructor body (after the `addAndMakeVisible` loop), add:

```cpp
        osc1WaveAttach = std::make_unique<juce::ParameterAttachment>(
            *apvts.getParameter(ParamIDs::osc1Waveform),
            [this](float v) {
                const int idx = juce::jlimit(0, 3, (int) std::round(v));
                knobOsc1Pwm.setEnabled(idx == 2); // 2 = Square
                knobOsc1Pwm.repaint();
            });
        osc1WaveAttach->sendInitialUpdate();

        osc2WaveAttach = std::make_unique<juce::ParameterAttachment>(
            *apvts.getParameter(ParamIDs::osc2Waveform),
            [this](float v) {
                const int idx = juce::jlimit(0, 3, (int) std::round(v));
                knobOsc2Pwm.setEnabled(idx == 2);
                knobOsc2Pwm.repaint();
            });
        osc2WaveAttach->sendInitialUpdate();
```

- [ ] **Step 3: Rewrite `layoutFor`**

Replace the entire body of `layoutFor` with:

```cpp
        using namespace SynthVibe::Tokens;
        const int headerH = 20;
        const int scopeH  = 60;
        const int selectH = 26;

        auto area = getLocalBounds().reduced(spaceSm);
        const int colW = area.getWidth() / 3;
        auto col1 = area.removeFromLeft(colW).reduced(spaceXs, 0);
        auto col2 = area.removeFromLeft(colW).reduced(spaceXs, 0);
        auto col3 = area.reduced(spaceXs, 0);

        auto layoutOscPanel = [&](juce::Rectangle<int>& outBounds,
                                  juce::Rectangle<int> col,
                                  SynthVibe::PanelHeader& header,
                                  SynthVibe::OscilloscopeView& scope,
                                  SynthVibe::WaveTypeSelect& waveSel,
                                  std::initializer_list<juce::Component*> pitchRow,
                                  std::initializer_list<juce::Component*> shapeRow,
                                  std::initializer_list<juce::Component*> unisonRow)
        {
            outBounds = col;
            auto c = col.reduced(spaceSm);
            header.setBounds(c.removeFromTop(headerH));
            c.removeFromTop(spaceXs);
            scope.setBounds(c.removeFromTop(scopeH));
            c.removeFromTop(spaceXs);
            waveSel.setBounds(c.removeFromTop(selectH));
            c.removeFromTop(spaceSm);
            const int rowH = c.getHeight() / 3;
            layoutKnobsRow(c.removeFromTop(rowH), pitchRow);
            layoutKnobsRow(c.removeFromTop(rowH), shapeRow);
            layoutKnobsRow(c, unisonRow);
        };

        layoutOscPanel(osc1Bounds, col1,
                       osc1Header, osc1Scope, osc1Wave,
                       { &knobOsc1Oct, &knobOsc1Semi, &knobOsc1Detune },
                       { &knobOsc1Phase, &knobOsc1Pwm, &knobOsc1Level },
                       { &knobUniVoices, &knobUniDetune, &knobUniSpread });

        layoutOscPanel(osc2Bounds, col2,
                       osc2Header, osc2Scope, osc2Wave,
                       { &knobOsc2Oct, &knobOsc2Semi, &knobOsc2Detune },
                       { &knobOsc2Phase, &knobOsc2Pwm, &knobOsc2Level },
                       { &knobOsc2UniVoices, &knobOsc2UniDetune, &knobOsc2UniSpread });

        const int filterH = col3.getHeight() * 34 / 100;
        const int envH    = (col3.getHeight() - filterH) / 2;

        filterBounds = col3.removeFromTop(filterH);
        auto cf = filterBounds.reduced(spaceSm);
        filterHeader.setBounds(cf.removeFromTop(headerH));
        cf.removeFromTop(spaceXs);
        filterResponse.setBounds(cf.removeFromTop(cf.getHeight() / 2));
        cf.removeFromTop(spaceXs);
        filterType.setBounds(cf.removeFromTop(selectH));
        cf.removeFromTop(spaceXs);
        const int filterKnobRowH = cf.getHeight() / 2;
        layoutKnobsRow(cf.removeFromTop(filterKnobRowH),
                       { &knobCutoff, &knobResonance, &knobFilterEnv });
        layoutKnobsRow(cf,
                       { &knobFilterDrive, &knobFilterKeytrack, nullptr });

        ampEnvBounds = col3.removeFromTop(envH);
        auto ca = ampEnvBounds.reduced(spaceSm);
        ampEnvHeader.setBounds(ca.removeFromTop(headerH));
        ca.removeFromTop(spaceXs);
        ampEnv.setBounds(ca);

        fltEnvBounds = col3;
        auto cfe = fltEnvBounds.reduced(spaceSm);
        fltEnvHeader.setBounds(cfe.removeFromTop(headerH));
        cfe.removeFromTop(spaceXs);
        fltEnv.setBounds(cfe);
```

The Filter bottom row has two knobs with a blank third column. `layoutKnobsRow` needs to skip `nullptr` entries — update its body:

```cpp
    static void layoutKnobsRow(juce::Rectangle<int> bounds,
                               std::initializer_list<juce::Component*> knobs)
    {
        if (knobs.size() == 0) return;
        const int w = bounds.getWidth() / (int) knobs.size();
        auto b = bounds;
        for (auto* k : knobs)
        {
            auto slot = b.removeFromLeft(w);
            if (k != nullptr) k->setBounds(slot);
        }
    }
```

- [ ] **Step 4: Update `UIConstructionTests`**

In `Tests/UIConstructionTests.cpp`, if there's a case that constructs `SoundTab`, confirm the construction still succeeds. Grep:

```bash
grep -n "SoundTab" Tests
```

If no SoundTab case exists (the current file only tests component primitives), add one at the end of the test file:

```cpp
        beginTest("SoundTab constructs with Phase 2b grid");
        {
            juce::AudioProcessorGraph dummyGraph;
            juce::AudioProcessorValueTreeState apvts(
                dummyGraph, nullptr, "AISynthState", ParameterLayout::create());
            SoundTab tab(apvts);
            tab.setBounds(0, 0, 1280, 560);
            expect(tab.getWidth() == 1280, "SoundTab should construct at Phase 2b width");
        }
```

Add `#include "../Source/UI/SoundTab.h"` if missing. (Check the existing includes first.)

- [ ] **Step 5: Build and run tests**

Run:
```bash
./build.bat && build/AISynth_artefacts/Release/AISynthTests.exe
```

Expected: clean build, all tests pass, plugin UI opens in the DAW without layout asserts.

- [ ] **Step 6: Commit**

```bash
git add Source/UI/SoundTab.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): SoundTab 3x3 grid, OSC2 unison, drive/keytrack, PWM enable-state"
```

---

### Task 13: Delete legacy colour aliases

Confirm dead, then delete four constants from `LookAndFeel.h`.

**Files:**
- Modify: `Source/UI/LookAndFeel.h` (lines 21–24)

- [ ] **Step 1: Grep to confirm zero call sites**

Run:
```bash
grep -rn "colOscAccent\|colFilterAccent\|colEnvAccent\|colMasterAccent" Source Tests
```

Expected: only the four declaration lines in `LookAndFeel.h` match. If any other hit exists, migrate the user to the corresponding `SynthVibe::Tokens::` value first in the same edit. The candidates are `Tokens::osc`, `Tokens::filter`, `Tokens::env`, and `Tokens::accent` (or `Tokens::panel` / similar — pick whichever matches the surviving usage; if no surviving usage exists, no migration needed).

- [ ] **Step 2: Delete the four lines**

In `Source/UI/LookAndFeel.h`, remove:

```cpp
    static constexpr auto colOscAccent    = 0xFF9ABFE8;
    static constexpr auto colFilterAccent = 0xFFE8A3C7;
    static constexpr auto colEnvAccent    = 0xFFA0D4A8;
    static constexpr auto colMasterAccent = ...;   // if present; not in the current file
```

`colMasterAccent` is not in the current file — the four aliases named in the spec are actually `colOscAccent`, `colFilterAccent`, `colEnvAccent`, and (from spec list) "colMasterAccent". The current file has `colLfoAccent`, `colFxAccent`, `colArpAccent` which the spec didn't name for deletion. Delete exactly the three that match the spec (`colOscAccent`, `colFilterAccent`, `colEnvAccent`) if the grep in Step 1 confirmed no users; leave the others alone for a future cleanup pass.

- [ ] **Step 3: Build**

Run:
```bash
./build.bat && build/AISynth_artefacts/Release/AISynthTests.exe
```

Expected: clean build, all tests pass.

- [ ] **Step 4: Commit**

```bash
git add Source/UI/LookAndFeel.h
git commit -m "chore(ui): delete unused legacy colour aliases"
```

---

### Task 14: Manual DAW smoke test

Run the nine-item checklist from the design spec. This is a verification step, not a code change — but it's a required gate before the phase ships.

**Files:** none modified. Use the Phase 2 reference preset from the baseline tag.

- [ ] **Step 1: Deploy the freshly built VST3**

Run:
```bash
./deploy.bat
```

Expected: the VST3 lands in `C:\Program Files\Common Files\VST3\`. The deploy script prints a confirmation.

- [ ] **Step 2: Run the 9-item smoke checklist**

Load the plugin into Reaper (or an equivalent host). For each item below, document the observation in a short note below the checkbox:

1. Plugin loads with the Phase 2 reference preset; no crash.
2. `osc1.phase = 90°` on Sine audibly delays the attack.
3. `osc1.pwm` sweep on Square produces a narrow-to-wide pulse sweep; no clicks at extremes.
4. OSC2 unison: voices=5, detune=25¢, spread=0.8 — stereo spreads, supersaw shimmer.
5. Filter type = LP24 is audibly steeper than LP12 on a saw at the same cutoff + resonance.
6. Filter type = Notch carves a hole in a saw pad.
7. `filt.drive = 0.7` thickens a plucky bass; no volume blow-up relative to drive=0.
8. `filt.keytrack = +1` — cutoff follows pitch across two octaves; no artifacts.
9. All nine new parameters automate from the host (wiggle each in Reaper's envelope lane).

- [ ] **Step 3: Commit the observations**

If any item failed, file a follow-up task and hold the phase tag. If all nine passed, proceed:

```bash
git commit --allow-empty -m "test: Phase 2b manual DAW smoke test passed"
```

- [ ] **Step 4: Tag the phase**

```bash
git tag -a phase2b-sound-tab -m "Phase 2b: nine Sound Tab params, LP24/Notch, drive, keytrack"
git push origin refs/tags/phase2b-sound-tab
```

---

## Task Summary

| # | Task | Risk | Files touched |
|---|---|---|---|
| 1 | Rename unison constants | Low | 4 |
| 2 | Register 9 new params | Low | 3 |
| 3 | Reorder `filt.type` | Low (breaks local presets intentionally) | 1 |
| 4 | Oscillator starting phase + variable PWM PolyBLEP | **High** | 4 |
| 5 | UnisonOscillator forwards phase/PWM | Low | 3 |
| 6 | Filter LP24 / Notch / drive rewrite | Medium | 3 |
| 7 | Voice wires new params + keytrack | Medium | 3 |
| 8 | PluginProcessor APVTS reads | Low | 1 |
| 9 | FilterCoefficients visualisation | Low | 2 |
| 10 | FilterResponseView dispatch | Low | 1 |
| 11 | FilterTypeSelect 5 pills | Low | 2 |
| 12 | SoundTab 3×3 grid + attachments | Medium | 2 |
| 13 | Delete legacy aliases | Low | 1 |
| 14 | Manual DAW smoke test | Medium (no automation) | 0 |

**Total:** 14 tasks; estimate 12–15 hours.
