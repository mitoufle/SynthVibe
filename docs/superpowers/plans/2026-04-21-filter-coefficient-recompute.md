# Filter Coefficient Sub-Rate Update (I-2) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reduce per-sample `std::tan` cost of filter coefficient updates by moving coefficient application from audio rate to control rate (every 16 samples) inside `Voice::getNextSample()`.

**Architecture:** Add a per-voice `int filterCoefCounter` and a `static constexpr int FilterCoefUpdateRate = 16` to `Voice`. Gate the existing cutoff-apply block with `counter == 0`. Advance the counter once per active-voice sample using a power-of-two bitmask. Counter resets in `Voice::prepare()` and `Voice::noteOn()` so each note starts the cadence fresh. `Filter.h` / `Filter.cpp` are not touched — the sub-rate boundary is a Voice concern.

**Tech Stack:** C++17, JUCE 7.0.9, MSVC (VS2022). Build: `cmake.exe --build build --config Release --target <target>`. Tests: `build/AISynthTests_artefacts/Release/AISynthTests.exe`.

cmake path: `C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe`

---

## File Map

| File | Change |
|---|---|
| `Source/Engine/Voice.h` | Add `FilterCoefUpdateRate` constant and `filterCoefCounter` member |
| `Source/Engine/Voice.cpp` | Reset counter in `prepare()` and `noteOn()`; gate apply block + advance counter in `getNextSample()` |

---

## Task 1 — Voice: add counter and resets

**Files:**
- Modify: `Source/Engine/Voice.h`
- Modify: `Source/Engine/Voice.cpp`

- [ ] **Step 1: Add constant and counter member to Voice.h**

In `Source/Engine/Voice.h`, in the `private:` section, add two declarations immediately before the existing `smoothCutoff` line:

```cpp
private:
    static constexpr int FilterCoefUpdateRate = 16;   // power of 2 → cheap bitmask; ~3 kHz control rate at 48 kHz
    int filterCoefCounter = 0;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothCutoff;
    // ... rest unchanged
```

- [ ] **Step 2: Reset counter in Voice::prepare()**

In `Source/Engine/Voice.cpp`, at the end of `Voice::prepare()` (after the four `setCurrentAndTargetValue` calls), add one line:

```cpp
    smoothOsc2Level.setCurrentAndTargetValue (0.f);
    filterCoefCounter = 0;
}
```

- [ ] **Step 3: Reset counter in Voice::noteOn()**

In `Source/Engine/Voice.cpp`, at the end of `Voice::noteOn()` (after `fltEnv.noteOn();`), add one line:

```cpp
    ampEnv.noteOn();
    fltEnv.noteOn();
    filterCoefCounter = 0;
}
```

- [ ] **Step 4: Build and run tests**

```bat
"C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" --build build --config Release --target AISynthTests
./build/AISynthTests_artefacts/Release/AISynthTests.exe
```

Expected: build succeeds, all tests pass, exit code 0. (The counter is declared and reset but not yet read — behavior unchanged.)

- [ ] **Step 5: Commit**

```bash
git add Source/Engine/Voice.h Source/Engine/Voice.cpp
git commit -m "feat: add filterCoefCounter infrastructure to Voice (I-2)"
```

---

## Task 2 — Voice: gate filter coefficient apply on counter

**Files:**
- Modify: `Source/Engine/Voice.cpp`

- [ ] **Step 1: Replace the filter-modulation apply block in getNextSample()**

In `Source/Engine/Voice.cpp`, in `Voice::getNextSample()`, find this block (currently around lines 141–153):

```cpp
    const float envMod = fltEnv.getNextSample();
    const bool hasFilterMod = (params.filterEnvAmt != 0.f)
                            || (params.lfo1.dest == LfoDest::Filter && params.lfo1.depth != 0.f)
                            || (params.lfo2.dest == LfoDest::Filter && params.lfo2.depth != 0.f);
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

Replace with:

```cpp
    const float envMod = fltEnv.getNextSample();
    const bool hasFilterMod = (params.filterEnvAmt != 0.f)
                            || (params.lfo1.dest == LfoDest::Filter && params.lfo1.depth != 0.f)
                            || (params.lfo2.dest == LfoDest::Filter && params.lfo2.depth != 0.f);
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

Two changes: (a) add `&& filterCoefCounter == 0` to the guard condition, (b) advance `filterCoefCounter` after the `if` block — unconditional on mod state, so the counter cadence stays deterministic for any active voice.

- [ ] **Step 2: Build and run tests**

```bat
"C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" --build build --config Release --target AISynthTests
./build/AISynthTests_artefacts/Release/AISynthTests.exe
```

Expected: all tests pass, exit code 0.

- [ ] **Step 3: Build VST3 for manual audio check**

```bat
"C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" --build build --config Release --target AISynth_VST3
```

Expected: VST3 built at `build/AISynth_artefacts/Release/VST3/AI Synth.vst3`.

- [ ] **Step 4: Manual audio verification**

Load the VST3 in a DAW. Use a patch with `filterEnvAmt = 1.0` and/or an LFO routed to Filter.

Check list:
- Play notes across the keyboard — filter sweep sounds identical to prior behavior.
- Sweep the cutoff knob while holding a note — no zipper, no stair-stepping.
- Sweep env-amount and LFO-depth — no audible artifacts.
- Hold a chord (multiple voices active simultaneously) — no degradation.

Expected: indistinguishable audibly from the pre-change build.

- [ ] **Step 5: Commit**

```bash
git add Source/Engine/Voice.cpp
git commit -m "feat: sub-rate filter coefficient update (every 16 samples) in Voice (I-2)"
```

---

## Self-Review

**Spec coverage (from `docs/superpowers/specs/2026-04-21-filter-coefficient-recompute-design.md`):**
- Add `FilterCoefUpdateRate` constant + `filterCoefCounter` member → Task 1 Step 1 ✓
- Reset counter in `prepare()` → Task 1 Step 2 ✓
- Reset counter in `noteOn()` → Task 1 Step 3 ✓
- Gate apply block with `counter == 0` → Task 2 Step 1 ✓
- Advance counter once per active-voice sample (after the `if`, unconditional on mod state) → Task 2 Step 1 ✓
- `Filter.h` / `Filter.cpp` unchanged → honored, neither file is modified ✓
- Existing `AISynthTests` must still pass → asserted in Task 1 Step 4 and Task 2 Step 2 ✓
- Manual audio check → Task 2 Step 4 ✓
- No new unit test → honored ✓

**Placeholder scan:** None — every step contains full code or full commands.

**Type consistency:**
- `FilterCoefUpdateRate` declared in Task 1 Step 1, used in Task 2 Step 1 (bitmask `& (FilterCoefUpdateRate - 1)`). 16 is a power of two so `- 1 = 15 = 0b1111` is the correct mask.
- `filterCoefCounter` declared in Task 1 Step 1; reset in Task 1 Steps 2–3; read+advanced in Task 2 Step 1.
- All existing symbols referenced (`smoothCutoff`, `fltEnv`, `filter`, `filterR`, `params`, `l1`, `l2`, `sCutoff`, `ampEnv`) are used verbatim from the current `Voice.cpp` / `Voice.h` — no renames proposed.
- Commit message scope `(I-2)` matches prior commit style (e.g., `(I-4)` in recent commits).
