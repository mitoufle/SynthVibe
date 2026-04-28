# Phase B Wavetable Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a wavetable oscillator path with a 5-table curated bank (Organ, EP, Bell, Vocal, Noise) so Claude can produce convincing instrument timbres that the current subtractive engine can't reach.

**Architecture:** New `Waveform::Wavetable` enum value (appended at index 4) routes to a new `WavetableBank` that holds `constexpr` mipmap arrays. New `oscN.table` Choice param picks which table. PolyBLEP path for Sine/Saw/Square/Triangle stays strictly intact. Bank tables are produced by a dev-time Python tool (`tools/wavetable/generate.py`) and the generated `Source/Engine/WavetableData.cpp` is committed; C++ build never runs Python.

**Tech Stack:** JUCE 7.0.9, C++17, JUCE UnitTest framework, CMake/Visual Studio 2022, Python 3 (dev-time only).

**Spec:** `docs/superpowers/specs/2026-04-28-phase-b-wavetable-design.md`

**Branch / worktree:** This plan is intended to be executed on a fresh branch `phase-b-wavetable` in a fresh git worktree. The plan itself is committed on `main`.

---

## Pre-flight

- Build the test target before starting any task to confirm baseline:
  ```bash
  cmake --build build --target AISynthTests --config Release
  ./build/AISynthTests_artefacts/Release/AISynth\ Tests.exe
  ```
- Expected: `127/127` tests pass.
- If baseline fails, stop and ask the user — do not start the plan over a broken baseline.

**Build verification rule:** After every task that touches C++, build BOTH `AISynth_VST3` and `AISynthTests` in Release. Verify by checking artifact mtime moved forward (per `memory/feedback_subagent_build_verify.md`). Do not claim "build OK" based on stdout alone.

---

## Task 1: Extend Waveform enum with Wavetable

**Files:**
- Modify: `Source/Engine/Oscillator.h`
- Modify: `Source/Engine/Oscillator.cpp`
- Test: `Tests/OscillatorTests.cpp`

**Goal:** Add the enum value and a quiet default switch path. No bank wiring yet — `Waveform::Wavetable` returns 0.f silently. This is the foundation that subsequent tasks build on without breaking anything.

- [ ] **Step 1: Write the failing test**

Append to `Tests/OscillatorTests.cpp` inside the existing `runTest()`:

```cpp
beginTest("Wavetable enum value exists and oscillator returns silence pre-bank");
{
    Oscillator o;
    o.setSampleRate(48000.0);
    o.setFrequency(440.0);
    o.setWaveform(Waveform::Wavetable);
    o.reset();
    float maxAbs = 0.f;
    for (int i = 0; i < 1024; ++i)
        maxAbs = std::max(maxAbs, std::abs(o.getNextSample()));
    expect(maxAbs == 0.f,
           "Waveform::Wavetable should return 0.f when no bank is wired; got max |sample| = "
           + juce::String(maxAbs));
}
```

- [ ] **Step 2: Run test to verify it fails**

```bash
cmake --build build --target AISynthTests --config Release
```

Expected: compile error — `Waveform::Wavetable` not defined.

- [ ] **Step 3: Add the enum value and silent default in source**

In `Source/Engine/Oscillator.h`, change:

```cpp
enum class Waveform { Sine = 0, Saw, Square, Triangle };
```

to:

```cpp
enum class Waveform { Sine = 0, Saw, Square, Triangle, Wavetable };
```

In `Source/Engine/Oscillator.cpp`, add a `default:` arm to the existing `switch (waveform)` block (just before the closing brace of the switch):

```cpp
        default:
            // Wavetable case — handled in Task 7 once WavetableBank is wired.
            // Silent fallthrough until then.
            break;
```

- [ ] **Step 4: Run tests to verify pass**

```bash
cmake --build build --target AISynthTests --config Release && \
  ./build/AISynthTests_artefacts/Release/AISynth\ Tests.exe
```

Expected: 128/128 pass (existing 127 + new test).

- [ ] **Step 5: Commit**

```bash
git add Source/Engine/Oscillator.h Source/Engine/Oscillator.cpp Tests/OscillatorTests.cpp
git commit -m "feat(engine): add Waveform::Wavetable enum value (silent default)"
```

---

## Task 2: Add `oscN.table` ParamID constants

**Files:**
- Modify: `Source/Parameters/ParameterIDs.h`

**Goal:** Constants only. No APVTS change yet — drift test must remain green after this task.

- [ ] **Step 1: Edit `Source/Parameters/ParameterIDs.h`**

Locate the `// Oscillator 1` block (around line 9–15). After `osc1Pwm` add:

```cpp
    inline constexpr const char* osc1Table     = "osc1.table";
```

Locate the `// Oscillator 2` block. After `osc2Pwm` add:

```cpp
    inline constexpr const char* osc2Table     = "osc2.table";
```

- [ ] **Step 2: Build to verify no compilation regression**

```bash
cmake --build build --target AISynthTests --config Release
```

Expected: build succeeds (constants are unused but the file is header-only inline constexpr — no link issues).

- [ ] **Step 3: Run tests**

Expected: 128/128 still pass.

- [ ] **Step 4: Commit**

```bash
git add Source/Parameters/ParameterIDs.h
git commit -m "feat(params): declare osc1.table / osc2.table param IDs"
```

---

## Task 3: Wire APVTS + ParamIdIndex in lockstep

**Files:**
- Modify: `Source/Parameters/ParameterLayout.cpp`
- Modify: `Source/AI/ParamIdIndex.cpp`

**Goal:** Extend `oscN.wave` choices to 5 entries, add `oscN.table` Choice params (5 entries), and update `ParamIdIndex` in the **same commit** so the drift test stays green throughout.

- [ ] **Step 1: Edit `Source/Parameters/ParameterLayout.cpp`** — Osc1 wave choice

Find:

```cpp
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParamIDs::osc1Waveform, "Osc1 Waveform",
        StringArray { "Sine", "Saw", "Square", "Triangle" }, 1));
```

Replace with:

```cpp
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParamIDs::osc1Waveform, "Osc1 Waveform",
        StringArray { "Sine", "Saw", "Square", "Triangle", "Wavetable" }, 1));
```

Same change for Osc2 (`ParamIDs::osc2Waveform`).

- [ ] **Step 2: Edit `Source/Parameters/ParameterLayout.cpp`** — add new table Choice params

After the Osc1 Pwm block (look for `ParamIDs::osc1Pwm`), insert:

```cpp
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParamIDs::osc1Table, "Osc1 Table",
        StringArray { "Organ", "EP", "Bell", "Vocal", "Noise" }, 0));
```

After the Osc2 Pwm block, insert:

```cpp
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParamIDs::osc2Table, "Osc2 Table",
        StringArray { "Organ", "EP", "Bell", "Vocal", "Noise" }, 0));
```

- [ ] **Step 3: Edit `Source/AI/ParamIdIndex.cpp`** — sync wave choice arrays and add table entries

Find:

```cpp
            { "osc1.wave",   "Osc1 Waveform", ParamEntry::Type::Choice, 0.0, 3.0, 1.0,
              { "Sine", "Saw", "Square", "Triangle" } },
```

Replace with:

```cpp
            { "osc1.wave",   "Osc1 Waveform", ParamEntry::Type::Choice, 0.0, 4.0, 1.0,
              { "Sine", "Saw", "Square", "Triangle", "Wavetable" } },
```

Same change for `osc2.wave` (max 4.0, choices length 5).

After the `osc1.pwm` entry, insert:

```cpp
            { "osc1.table",  "Osc1 Table",    ParamEntry::Type::Choice, 0.0, 4.0, 0.0,
              { "Organ", "EP", "Bell", "Vocal", "Noise" } },
```

After the `osc2.pwm` entry, insert:

```cpp
            { "osc2.table",  "Osc2 Table",    ParamEntry::Type::Choice, 0.0, 4.0, 0.0,
              { "Organ", "EP", "Bell", "Vocal", "Noise" } },
```

- [ ] **Step 4: Build and run tests**

```bash
cmake --build build --target AISynthTests --config Release && \
  ./build/AISynthTests_artefacts/Release/AISynth\ Tests.exe
```

Expected: 128/128 pass. The drift test must remain green; if it fails, an entry is misaligned.

- [ ] **Step 5: Commit**

```bash
git add Source/Parameters/ParameterLayout.cpp Source/AI/ParamIdIndex.cpp
git commit -m "feat(params): expose oscN.wave Wavetable choice + oscN.table Choice"
```

---

## Task 4: WavetableBank header (interface only)

**Files:**
- Create: `Source/Engine/WavetableBank.h`

**Goal:** Define the bank API. No implementation yet, no data yet. Header alone ensures the interface is locked in before code/tests depend on it.

- [ ] **Step 1: Create `Source/Engine/WavetableBank.h`**

```cpp
#pragma once
#include <cstddef>

// Read-only single-cycle wavetable bank with mipmapped (band-limited) tables.
//
// Construction sets up pointers to the constexpr arrays in WavetableData.cpp.
// Audio-thread API (`fetchSample`) is purely read-only and contains no
// allocation, locking, or I/O.
class WavetableBank
{
public:
    static constexpr int NumTables = 5;
    static constexpr int NumMips   = 10;
    static constexpr int Mip0Size  = 2048;   // mip[N] holds Mip0Size / (1 << N) samples

    WavetableBank();

    // Returns one band-limited sample.
    //
    // Preconditions (caller-enforced; bank does not validate):
    //   tableIdx ∈ [0, NumTables)
    //   phase    ∈ [0, 1)
    //   dt       > 0          // required so log2(dt * Mip0Size) is finite
    //
    // Calling with dt == 0 produces undefined behaviour (log2(0) = -∞).
    // Oscillator::getNextSample is the only intended caller and already
    // guarantees dt > 0 since frequency > 0 post-noteOn.
    float fetchSample(int tableIdx, double phase, double dt) const noexcept;

    // Returns a pointer to mip[0] (full-resolution single cycle) for UI
    // visualisation. Callers can read Mip0Size floats from the returned pointer.
    const float* getCycle(int tableIdx) const noexcept;

private:
    struct Entry
    {
        const float* mips[NumMips];   // mips[N] points at a band-limited array of (Mip0Size >> N) floats
    };

    Entry entries[NumTables];
};
```

- [ ] **Step 2: Build (header-only — does not compile alone)**

The header is unused until Task 5 wires the data. Build only to confirm no syntax errors:

```bash
cmake --build build --target AISynthTests --config Release
```

Expected: build succeeds (header is not yet `#include`-d anywhere).

- [ ] **Step 3: Commit**

```bash
git add Source/Engine/WavetableBank.h
git commit -m "feat(engine): WavetableBank.h — interface declaration"
```

---

## Task 5: WavetableData.cpp stub (5 sine entries) + CMake wiring

**Files:**
- Create: `Source/Engine/WavetableData.cpp`
- Modify: `CMakeLists.txt`

**Goal:** Stub data so the bank can compile. Real timbres come at Task 15. Stub uses `std::sin` baked into `constexpr` arrays via a generation lambda (run at static-init, not strictly `constexpr`, but kept private and computed once).

- [ ] **Step 1: Create `Source/Engine/WavetableData.cpp`**

```cpp
// AUTO-GENERATED PLACEHOLDER — replaced at Task 15 by tools/wavetable/generate.py.
//
// All 5 entries currently hold a 1-Hz sine across all 10 mip levels.
// This is *not* a usable bank for hearing instrument timbres — it's only
// here to prove the WavetableBank wiring compiles and passes mechanical tests.
#include "WavetableBank.h"
#include <array>
#include <cmath>

namespace
{
    constexpr int NumTables = WavetableBank::NumTables;
    constexpr int NumMips   = WavetableBank::NumMips;
    constexpr int Mip0Size  = WavetableBank::Mip0Size;

    // Total samples across all mips per table = Mip0Size * (1 - 0.5^NumMips) / (1 - 0.5)
    //   = Mip0Size * 2 * (1 - 1/2^NumMips) ≈ 2 * Mip0Size for large NumMips.
    constexpr int totalPerTable()
    {
        int total = 0;
        for (int n = 0; n < NumMips; ++n) total += Mip0Size >> n;
        return total;
    }

    // One contiguous storage block per table; mip[N] starts at offset
    //   Mip0Size * (1 - 0.5^N) / (1 - 0.5) = Mip0Size * 2 * (1 - 1/2^N)
    using TableStorage = std::array<float, totalPerTable()>;

    TableStorage makeSineTable()
    {
        TableStorage t {};
        int offset = 0;
        for (int n = 0; n < NumMips; ++n)
        {
            const int len = Mip0Size >> n;
            for (int i = 0; i < len; ++i)
                t[offset + i] = static_cast<float>(std::sin(2.0 * 3.14159265358979323846 * i / len));
            offset += len;
        }
        return t;
    }

    const TableStorage& storage(int /*idx*/)
    {
        static const TableStorage s = makeSineTable();
        return s;   // PLACEHOLDER: same data for all 5 entries.
    }
}

WavetableBank::WavetableBank()
{
    for (int t = 0; t < NumTables; ++t)
    {
        const float* base = storage(t).data();
        int offset = 0;
        for (int n = 0; n < NumMips; ++n)
        {
            entries[t].mips[n] = base + offset;
            offset += Mip0Size >> n;
        }
    }
}
```

> **Note for the implementer:** the static-init `s` is constructed once on first call, on the audio thread the first time `fetchSample` runs if the bank wasn't constructed earlier. To keep audio-thread RT-safety, ensure `WavetableBank` is constructed (which calls `storage(t)` for each table) on the *prepare* path, not lazily on first audio. Task 10 places the bank as a member of `SynthEngine`, which is constructed before `prepare`, so the static initialisers complete before audio runs.

- [ ] **Step 2: Add to CMakeLists.txt**

In `CMakeLists.txt`, find the `target_sources(AISynth PRIVATE` block (around line 40). Add inside the engine section (after `Source/Engine/SynthEngine.cpp`):

```cmake
    Source/Engine/WavetableBank.cpp
    Source/Engine/WavetableData.cpp
```

In the `target_sources(AISynthTests PRIVATE` block (around line 101), add the same two lines (after `Source/Engine/SynthEngine.cpp`):

```cmake
    Source/Engine/WavetableBank.cpp
    Source/Engine/WavetableData.cpp
```

> `WavetableBank.cpp` doesn't exist yet — added here so we can do a single CMake sync. The next task creates it.

- [ ] **Step 3: Verify CMake regenerates without error**

```bash
cmake --build build --target AISynthTests --config Release 2>&1 | tail -5
```

Expected: regeneration of project files; compilation will fail on missing `WavetableBank.cpp`. That's expected — fixed in Task 6.

- [ ] **Step 4: Commit**

```bash
git add Source/Engine/WavetableData.cpp CMakeLists.txt
git commit -m "feat(engine): WavetableData.cpp stub (5 sine entries) + CMake wiring"
```

---

## Task 6: WavetableBank.cpp implementation + tests

**Files:**
- Create: `Source/Engine/WavetableBank.cpp`
- Create: `Tests/WavetableBankTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing tests**

Create `Tests/WavetableBankTests.cpp`:

```cpp
#include <juce_core/juce_core.h>
#include "Engine/WavetableBank.h"
#include <cmath>

struct WavetableBankTests : public juce::UnitTest
{
    WavetableBankTests() : juce::UnitTest("WavetableBank", "AISynth") {}

    void runTest() override
    {
        WavetableBank bank;

        beginTest("getCycle returns non-null for every table index");
        {
            for (int t = 0; t < WavetableBank::NumTables; ++t)
                expect(bank.getCycle(t) != nullptr,
                       "getCycle(" + juce::String(t) + ") was nullptr");
        }

        beginTest("fetchSample stays bounded in [-1, 1] across the MIDI range");
        {
            const double sr = 48000.0;
            const std::array<double, 5> testFreqs { 27.5, 110.0, 440.0, 2093.0, 8372.0 };
            for (int t = 0; t < WavetableBank::NumTables; ++t)
                for (double f : testFreqs)
                {
                    const double dt = f / sr;
                    double phase = 0.0;
                    for (int i = 0; i < 4096; ++i)
                    {
                        const float s = bank.fetchSample(t, phase, dt);
                        expect(! std::isnan(s), "NaN at table=" + juce::String(t)
                               + " f=" + juce::String(f));
                        expect(std::abs(s) <= 1.001f,
                               "out-of-range sample " + juce::String(s)
                               + " at table=" + juce::String(t) + " f=" + juce::String(f));
                        phase += dt;
                        if (phase >= 1.0) phase -= 1.0;
                    }
                }
        }

        beginTest("mipmap selection drops aliasing-prone harmonics at high notes");
        {
            // At C8 (4186 Hz, dt ≈ 0.087 at 48kHz), dt*2048 ≈ 178 → ceil(log2) = 8
            // → mip[8] has 8 samples. Energy should be very low above mip Nyquist.
            // Smoke-check: the same fetch at very low notes vs very high notes
            // doesn't produce wildly different RMS (band-limiting works).
            const double sr = 48000.0;
            auto rms = [&](int tableIdx, double f) {
                const double dt = f / sr;
                double phase = 0.0, sumSq = 0.0;
                const int N = 8192;
                for (int i = 0; i < N; ++i)
                {
                    const float s = bank.fetchSample(tableIdx, phase, dt);
                    sumSq += s * s;
                    phase += dt;
                    if (phase >= 1.0) phase -= 1.0;
                }
                return std::sqrt(sumSq / N);
            };

            for (int t = 0; t < WavetableBank::NumTables; ++t)
            {
                const double rmsLow  = rms(t, 110.0);
                const double rmsHigh = rms(t, 4186.0);
                expect(rmsHigh > 0.05,
                       "table " + juce::String(t) + " mostly silent at high note (rms="
                       + juce::String(rmsHigh) + ")");
                expect(rmsHigh < rmsLow * 2.0,
                       "table " + juce::String(t) + " has more high-note RMS than expected — "
                       + "possible aliasing or mip selection bug");
            }
        }
    }
};

static WavetableBankTests _wavetableBankTests;
```

- [ ] **Step 2: Add test to CMakeLists.txt**

In the `target_sources(AISynthTests PRIVATE` block, add:

```cmake
    Tests/WavetableBankTests.cpp
```

- [ ] **Step 3: Run to verify failure**

```bash
cmake --build build --target AISynthTests --config Release
```

Expected: link error — `WavetableBank::WavetableBank` and `fetchSample`/`getCycle` undefined (they're declared in the header and used in the stub `WavetableData.cpp`'s constructor, but the methods aren't defined yet).

Note: `WavetableData.cpp` already defines the constructor in Task 5. So the missing symbols are `fetchSample` and `getCycle`. If link fails on those — good, proceed to step 4.

- [ ] **Step 4: Create `Source/Engine/WavetableBank.cpp`**

```cpp
#include "WavetableBank.h"
#include <algorithm>
#include <cmath>

float WavetableBank::fetchSample(int tableIdx, double phase, double dt) const noexcept
{
    // Precondition: dt > 0. See header.
    const int mipIdx = std::clamp(
        static_cast<int>(std::ceil(std::log2(dt * static_cast<double>(Mip0Size)))),
        0, NumMips - 1);

    const int mipLen = Mip0Size >> mipIdx;
    const float* mip = entries[tableIdx].mips[mipIdx];

    const double pos    = phase * mipLen;
    const int    i      = static_cast<int>(pos);
    const float  frac   = static_cast<float>(pos - i);
    const int    iNext  = (i + 1) >= mipLen ? 0 : (i + 1);

    return mip[i] + frac * (mip[iNext] - mip[i]);
}

const float* WavetableBank::getCycle(int tableIdx) const noexcept
{
    return entries[tableIdx].mips[0];
}
```

- [ ] **Step 5: Run tests**

```bash
cmake --build build --target AISynthTests --config Release && \
  ./build/AISynthTests_artefacts/Release/AISynth\ Tests.exe
```

Expected: all tests pass including new `WavetableBank` group.

- [ ] **Step 6: Commit**

```bash
git add Source/Engine/WavetableBank.cpp Tests/WavetableBankTests.cpp CMakeLists.txt
git commit -m "feat(engine): WavetableBank::fetchSample with mipmap + linear interp"
```

---

## Task 7: Wire Oscillator dispatch through the bank

**Files:**
- Modify: `Source/Engine/Oscillator.h`
- Modify: `Source/Engine/Oscillator.cpp`
- Modify: `Tests/OscillatorTests.cpp`

**Goal:** Oscillator gains `setBank` and `setTable`. The `Wavetable` switch case routes to `bank->fetchSample`. Existing PolyBLEP cases untouched.

- [ ] **Step 1: Update `Tests/OscillatorTests.cpp`**

Replace the test added in Task 1 ("Wavetable enum value exists and oscillator returns silence pre-bank") with two new tests:

```cpp
beginTest("Oscillator with bank renders Wavetable case non-silently");
{
    WavetableBank bank;
    Oscillator o;
    o.setSampleRate(48000.0);
    o.setFrequency(440.0);
    o.setBank(&bank);
    o.setTable(0);
    o.setWaveform(Waveform::Wavetable);
    o.reset();
    float maxAbs = 0.f;
    for (int i = 0; i < 1024; ++i)
        maxAbs = std::max(maxAbs, std::abs(o.getNextSample()));
    expect(maxAbs > 0.01f,
           "Wavetable case should produce audible output; max |sample| = "
           + juce::String(maxAbs));
}

beginTest("Oscillator with no bank pointer returns silence on Wavetable case");
{
    Oscillator o;
    o.setSampleRate(48000.0);
    o.setFrequency(440.0);
    o.setWaveform(Waveform::Wavetable);
    o.reset();
    float maxAbs = 0.f;
    for (int i = 0; i < 1024; ++i)
        maxAbs = std::max(maxAbs, std::abs(o.getNextSample()));
    expect(maxAbs == 0.f,
           "Without bank pointer, Wavetable should fall through silently; got "
           + juce::String(maxAbs));
}
```

Add `#include "Engine/WavetableBank.h"` at the top of the test file if not already present.

- [ ] **Step 2: Build to verify failure**

Expected: compile error — `setBank` / `setTable` not defined.

- [ ] **Step 3: Edit `Source/Engine/Oscillator.h`**

Add forward declaration above the class:

```cpp
class WavetableBank;
```

Inside `class Oscillator`, add to the `public` section:

```cpp
    void setBank(const WavetableBank* b) noexcept { bank = b; }
    void setTable(int idx)               noexcept { tableIdx = idx; }
```

In the `private` section, add:

```cpp
    const WavetableBank* bank     = nullptr;
    int                  tableIdx = 0;
```

- [ ] **Step 4: Edit `Source/Engine/Oscillator.cpp`**

Add include at the top:

```cpp
#include "WavetableBank.h"
```

Replace the `default:` arm added in Task 1 with an explicit `case`:

```cpp
        case Waveform::Wavetable:
            if (bank != nullptr)
                sample = bank->fetchSample(tableIdx, phase, dt);
            // else: silent fallthrough (sample remains 0.f)
            break;
```

- [ ] **Step 5: Run tests**

Expected: all tests pass, both new tests included.

- [ ] **Step 6: Commit**

```bash
git add Source/Engine/Oscillator.h Source/Engine/Oscillator.cpp Tests/OscillatorTests.cpp
git commit -m "feat(engine): Oscillator dispatches Wavetable case to WavetableBank"
```

---

## Task 8: Forward bank/table through UnisonOscillator

**Files:**
- Modify: `Source/Engine/UnisonOscillator.h`
- Modify: `Source/Engine/UnisonOscillator.cpp`

**Goal:** UnisonOscillator wraps up to 7 `Oscillator` instances. It must forward `setBank` and `setTable` to all of them so each unison voice plays the same wavetable at its own detuned frequency.

- [ ] **Step 1: Edit `Source/Engine/UnisonOscillator.h`**

Add forward declaration above the class:

```cpp
class WavetableBank;
```

Inside `class UnisonOscillator`, add to the `public` section (near `setWaveform`):

```cpp
    void setBank(const WavetableBank* b);
    void setTable(int tableIdx);
```

- [ ] **Step 2: Edit `Source/Engine/UnisonOscillator.cpp`**

Append the two methods (mirror the pattern of `setWaveform`, which iterates over `oscs[0..MaxUnison]`):

```cpp
void UnisonOscillator::setBank(const WavetableBank* b)
{
    for (int i = 0; i < MaxUnison; ++i)
        oscs[i].setBank(b);
}

void UnisonOscillator::setTable(int tableIdx)
{
    for (int i = 0; i < MaxUnison; ++i)
        oscs[i].setTable(tableIdx);
}
```

- [ ] **Step 3: Build and run all tests**

Expected: 130/130 tests pass (no UnisonOscillator-specific tests yet — covered transitively in `VoiceTests` after Task 9).

- [ ] **Step 4: Commit**

```bash
git add Source/Engine/UnisonOscillator.h Source/Engine/UnisonOscillator.cpp
git commit -m "feat(engine): UnisonOscillator forwards setBank/setTable to sub-oscillators"
```

---

## Task 9: Voice / OscParams.tableIdx + apply in setParams

**Files:**
- Modify: `Source/Engine/Voice.h`
- Modify: `Source/Engine/Voice.cpp`
- Modify: `Tests/VoiceTests.cpp`

**Goal:** `OscParams` gains a `tableIdx` field. `Voice::setParams` calls `osc1.setTable(p.osc1.tableIdx)` and the same for osc2. New `Voice::setBankPointer` API for SynthEngine to inject the shared bank pointer at prepare.

- [ ] **Step 1: Write the failing test**

In `Tests/VoiceTests.cpp`, add a new test inside `runTest()`:

```cpp
beginTest("Voice with Wavetable wave produces audible output when bank is wired");
{
    WavetableBank bank;
    Voice v;
    juce::dsp::ProcessSpec spec { 48000.0, 256u, 2u };
    v.prepare(spec);
    v.setBankPointer(&bank);

    VoiceParams p;
    p.osc1.waveform   = Waveform::Wavetable;
    p.osc1.tableIdx   = 0;     // Organ
    p.osc1.level      = 1.f;
    p.ampEnv.attack   = 0.001f;
    p.ampEnv.decay    = 0.05f;
    p.ampEnv.sustain  = 0.8f;
    p.ampEnv.release  = 0.05f;
    v.setParams(p);
    v.noteOn(60, 1.f);

    float maxAbs = 0.f;
    for (int i = 0; i < 4096; ++i)
    {
        auto [l, r] = v.getNextSample();
        maxAbs = std::max(maxAbs, std::max(std::abs(l), std::abs(r)));
    }
    expect(maxAbs > 0.01f,
           "Voice should render Wavetable audibly; max |sample| = "
           + juce::String(maxAbs));
}
```

Add `#include "Engine/WavetableBank.h"` at the top of `Tests/VoiceTests.cpp` if not already present.

- [ ] **Step 2: Build to verify failure**

Expected: compile error — `OscParams::tableIdx` and `Voice::setBankPointer` not defined.

- [ ] **Step 3: Edit `Source/Engine/Voice.h`**

In `struct OscParams`, after `pulseWidth`:

```cpp
    int      tableIdx       = 0;    // valid only when waveform == Waveform::Wavetable
```

In `class Voice`, add a forward declaration above the class:

```cpp
class WavetableBank;
```

Inside `class Voice`, add to the `public` section:

```cpp
    void setBankPointer(const WavetableBank* bank) noexcept;
```

- [ ] **Step 4: Edit `Source/Engine/Voice.cpp`**

Add include at the top:

```cpp
#include "WavetableBank.h"
```

In `Voice::setParams`, after the existing `osc1.setWaveform(p.osc1.waveform);` and `osc2.setWaveform(p.osc2.waveform);` calls (around line 31-33), add:

```cpp
    osc1.setTable(p.osc1.tableIdx);
    osc2.setTable(p.osc2.tableIdx);
```

Append the new method:

```cpp
void Voice::setBankPointer(const WavetableBank* bank) noexcept
{
    osc1.setBank(bank);
    osc2.setBank(bank);
}
```

- [ ] **Step 5: Run tests**

Expected: 131/131 tests pass.

- [ ] **Step 6: Commit**

```bash
git add Source/Engine/Voice.h Source/Engine/Voice.cpp Tests/VoiceTests.cpp
git commit -m "feat(engine): Voice routes oscN.tableIdx + bank pointer to oscillators"
```

---

## Task 10: SynthEngine owns the bank, injects in prepare

**Files:**
- Modify: `Source/Engine/SynthEngine.h`
- Modify: `Source/Engine/SynthEngine.cpp`

**Goal:** `WavetableBank` lives as a `SynthEngine` member. `SynthEngine::prepare` propagates `&bank` to every voice. No new public API on SynthEngine.

- [ ] **Step 1: Edit `Source/Engine/SynthEngine.h`**

Add include near the existing `#include "Voice.h"`:

```cpp
#include "WavetableBank.h"
```

Inside `class SynthEngine`, in the `private` section, add:

```cpp
    WavetableBank wavetableBank;
```

- [ ] **Step 2: Edit `Source/Engine/SynthEngine.cpp`**

Locate `SynthEngine::prepare`. After the existing voice prepare loop (look for `voices[i].prepare(spec)` or similar), add:

```cpp
    for (auto& v : voices)
        v.setBankPointer(&wavetableBank);
```

- [ ] **Step 3: Build and run tests**

Expected: 131/131 still pass. Existing `SynthEngineTests` exercise the prepare path; the bank pointer is injected without observable behaviour change yet (no test currently sets `oscN.waveform = Wavetable` through SynthEngine).

- [ ] **Step 4: Commit**

```bash
git add Source/Engine/SynthEngine.h Source/Engine/SynthEngine.cpp
git commit -m "feat(engine): SynthEngine owns WavetableBank and injects on prepare"
```

---

## Task 11: PluginProcessor.buildVoiceParams reads new params

**Files:**
- Modify: `Source/PluginProcessor.cpp`
- Modify: `Tests/PatchApplierTests.cpp`

**Goal:** APVTS → `OscParams.tableIdx` mapping. Verified through the PatchApplier round-trip path.

- [ ] **Step 1: Write the failing test**

Add to `Tests/PatchApplierTests.cpp` inside `runTest()`:

```cpp
beginTest("PatchApplier sets osc1.wave=4 and osc1.table=2 → params land in APVTS");
{
    DummyProcessor proc;
    PatchApplier::Patch patch;
    patch.params.set("osc1.wave", 4);   // Wavetable
    patch.params.set("osc1.table", 2);  // Bell
    PatchApplier::apply(proc.apvts, patch);

    expectEquals(static_cast<int>(*proc.apvts.getRawParameterValue("osc1.wave")),  4);
    expectEquals(static_cast<int>(*proc.apvts.getRawParameterValue("osc1.table")), 2);
}
```

> Adjust the patch-construction syntax to match how `PatchApplier::Patch::params` is actually declared — read `Source/AI/PatchApplier.h` first if the field name differs from `params`.

- [ ] **Step 2: Build to verify failure**

Expected: assertion failure — `osc1.table` exists in APVTS but `buildVoiceParams` doesn't yet read it, so it's irrelevant to *this* test but the test should still pass mechanically since `PatchApplier::apply` writes to APVTS directly. If the test passes immediately, it means the wiring already works at the APVTS layer; the missing piece is `buildVoiceParams`. Move on to step 3 either way.

- [ ] **Step 3: Edit `Source/PluginProcessor.cpp`**

In `buildVoiceParams` (around line 83), find the Osc1 block. After the existing `p.osc1.pulseWidth = ...` line, add:

```cpp
    p.osc1.tableIdx = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::osc1Table));
```

Same in the Osc2 block, after `p.osc2.pulseWidth`:

```cpp
    p.osc2.tableIdx = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::osc2Table));
```

- [ ] **Step 4: Run tests**

Expected: 132/132 tests pass. The drift test confirms APVTS ↔ ParamIdIndex stays in sync.

- [ ] **Step 5: Commit**

```bash
git add Source/PluginProcessor.cpp Tests/PatchApplierTests.cpp
git commit -m "feat(processor): buildVoiceParams reads osc1.table / osc2.table"
```

---

## Task 12: SystemPrompt caveat sentence

**Files:**
- Modify: `Source/AI/SystemPrompt.cpp`
- Modify: `Tests/SystemPromptTests.cpp`

**Goal:** Add the V1-limitation paragraph for Bell and Vocal so Claude doesn't promise timbres the bank can't deliver.

- [ ] **Step 1: Write the failing test**

Add to `Tests/SystemPromptTests.cpp` inside `runTest()`:

```cpp
beginTest("system prompt warns Claude about Bell single-cycle limitation");
{
    const auto p = SystemPrompt::build();
    expect(p.contains("Bell"),       "system prompt must mention Bell");
    expect(p.contains("not")
        && (p.contains("evolving") || p.contains("decay")),
           "system prompt must caveat Bell as not a full evolving bell decay");
    expect(p.contains("Vocal"),      "system prompt must mention Vocal");
    expect(p.contains("Aa"),         "system prompt must specify the single Aa vowel");
}
```

- [ ] **Step 2: Build to verify failure**

Expected: assertion failures — keywords missing.

- [ ] **Step 3: Edit `Source/AI/SystemPrompt.cpp`**

Replace:

```cpp
          << "- For multiple variations, emit set_patch as parallel tool_use blocks.\n\n"
          << "Available parameters:\n\n"
```

with:

```cpp
          << "- For multiple variations, emit set_patch as parallel tool_use blocks.\n\n"
          << "Wavetable timbre rules:\n"
          << "- To use sampled instrument timbres, set oscN.wave to 4 (Wavetable) AND\n"
          << "  oscN.table to the desired index. The table param is ignored when wave is\n"
          << "  Sine/Saw/Square/Triangle.\n"
          << "- V1 timbre limitations: 'Bell' is a tonal harmonic snapshot — works as a\n"
          << "  bright pluck through filter+envelope, not as a full evolving bell decay.\n"
          << "  'Vocal' is a single 'Aa' vowel — does not synthesize 'Ooh', 'Eh', or other\n"
          << "  phonemes. Choose accordingly when matching prompts.\n\n"
          << "Available parameters:\n\n"
```

- [ ] **Step 4: Run tests**

Expected: 133/133 tests pass.

- [ ] **Step 5: Commit**

```bash
git add Source/AI/SystemPrompt.cpp Tests/SystemPromptTests.cpp
git commit -m "feat(ai): SystemPrompt explains wavetable coupling and V1 timbre limits"
```

---

## Task 13: TableSelect UI component + SoundTab integration

**Files:**
- Create: `Source/UI/components/TableSelect.h`
- Modify: `Source/UI/SoundTab.h`
- Modify: `Tests/UIConstructionTests.cpp`

**Goal:** Drop-in combo selector adossé à `oscN.table`. Conditionally dimmed when `oscN.wave != 4`, using the same `ParameterAttachment` pattern as the existing PWM dim logic.

- [ ] **Step 1: Create `Source/UI/components/TableSelect.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"

namespace SynthVibe
{
    class TableSelect : public juce::Component
    {
    public:
        TableSelect(juce::AudioProcessorValueTreeState& apvts,
                    const juce::String& paramID)
        {
            combo.addItemList({ "Organ", "EP", "Bell", "Vocal", "Noise" }, 1);
            combo.setColour(juce::ComboBox::backgroundColourId, Tokens::panel2);
            combo.setColour(juce::ComboBox::textColourId,        Tokens::ink);
            combo.setColour(juce::ComboBox::outlineColourId,     Tokens::edge);
            combo.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(combo);

            attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                apvts, paramID, combo);
        }

        void resized() override { combo.setBounds(getLocalBounds()); }

        juce::ComboBox& getCombo() { return combo; }

    private:
        juce::ComboBox combo;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    };
}
```

- [ ] **Step 2: Edit `Source/UI/SoundTab.h`**

Add the include near the other component includes:

```cpp
#include "components/TableSelect.h"
```

In the constructor initializer list, after `osc1Wave(...)` and `osc2Wave(...)`:

```cpp
          osc1Table(apvts, ParamIDs::osc1Table),
          osc2Table(apvts, ParamIDs::osc2Table),
```

In the `addAndMakeVisible` initializer list, add `&osc1Table, &osc2Table` to the `std::initializer_list<juce::Component*>`.

In the existing `osc1WaveAttach` lambda, after the existing `knobOsc1Pwm.setEnabled(idx == 2);` line, add:

```cpp
                osc1Table.setEnabled(idx == 4);
                osc1Table.repaint();
```

Same for `osc2WaveAttach`:

```cpp
                osc2Table.setEnabled(idx == 4);
                osc2Table.repaint();
```

In the private member declarations of `SoundTab`, after `osc1Wave` and `osc2Wave`, add:

```cpp
    SynthVibe::TableSelect osc1Table;
    SynthVibe::TableSelect osc2Table;
```

In `layoutFor`, place each `TableSelect` directly below its matching `WaveTypeSelect`. Use the same height as `selectH` (= 26 px). Approximate placement:

```cpp
    // After laying out osc1Wave at some bounds `r`:
    // osc1Table.setBounds(r.translated(0, selectH + 4));
```

> The exact layout integration depends on the existing `layoutFor` body. Read it first; place `TableSelect` so it visually sits under the wave dropdown in each OSC panel. Keep height = `selectH`.

- [ ] **Step 3: Update `Tests/UIConstructionTests.cpp`**

If `UIConstructionTests` exercises `SoundTab` construction (it does — the file lists `SoundTab` in its test cases), the existing tests should pass once the constructor compiles. No new test required unless `SoundTab`'s test setup hardcodes a constructor signature — read the test first.

- [ ] **Step 4: Build the VST3 and run tests**

```bash
cmake --build build --target AISynth_VST3 --config Release && \
cmake --build build --target AISynthTests --config Release && \
  ./build/AISynthTests_artefacts/Release/AISynth\ Tests.exe
```

Expected: 133/133 tests pass; VST3 builds.

- [ ] **Step 5: Commit**

```bash
git add Source/UI/components/TableSelect.h Source/UI/SoundTab.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): TableSelect component + SoundTab conditional dim wiring"
```

---

## Task 14: OscilloscopeView extension — bank pointer + 5th case

**Files:**
- Modify: `Source/UI/components/OscilloscopeView.h`
- Modify: `Source/UI/SoundTab.h` (callers)
- Modify: `Tests/UIConstructionTests.cpp` (if it constructs OscilloscopeView directly)

**Goal:** The scope visual reads from `WavetableBank::getCycle` when `wave == 4`. New constructor param: `WavetableBank*`. Existing `sample()` method gains a 5th case that interpolates into the bank cycle.

- [ ] **Step 1: Edit `Source/UI/components/OscilloscopeView.h`**

Add forward declaration near top:

```cpp
class WavetableBank;
```

Add the include in the `.h`:

```cpp
#include "../../Engine/WavetableBank.h"
```

(Or forward-declare and include in implementation — but since `OscilloscopeView` is header-only, include is required.)

Update the constructor signature:

```cpp
        OscilloscopeView(juce::AudioProcessorValueTreeState& apvts,
                         const juce::String& waveformParamID,
                         const juce::String& tableParamID,
                         const WavetableBank& bankRef,
                         juce::Colour lineColour = Tokens::osc)
            : param(apvts.getParameter(waveformParamID)),
              tableParam(apvts.getParameter(tableParamID)),
              bank(bankRef),
              colour(lineColour)
        {
            // ... existing waveAttach setup ...

            tableAttach = std::make_unique<juce::ParameterAttachment>(
                *tableParam, [this](float v) {
                    tableIndex = juce::jlimit(0, WavetableBank::NumTables - 1, (int) std::round(v));
                    repaint();
                });
            tableAttach->sendInitialUpdate();

            startTimerHz(30);
        }
```

In the existing `sample()` method, change the `switch` from 4 cases to 5:

```cpp
        float sample(float t) const
        {
            switch (waveIndex)
            {
                case 0: return std::sin(2.f * 3.14159265358979323846f * t);
                case 1: return 2.f * t - 1.f;
                case 2: return t < 0.5f ? 1.f : -1.f;
                case 3: return 4.f * std::abs(t - 0.5f) - 1.f;
                case 4:
                {
                    const float* cycle = bank.getCycle(tableIndex);
                    const int N = WavetableBank::Mip0Size;
                    const float pos = t * N;
                    const int   i   = juce::jlimit(0, N - 1, (int) pos);
                    const float frac = pos - i;
                    const int iNext = (i + 1) >= N ? 0 : (i + 1);
                    return cycle[i] + frac * (cycle[iNext] - cycle[i]);
                }
            }
            return 0.f;
        }
```

In the private members, add:

```cpp
        juce::RangedAudioParameter* tableParam;
        const WavetableBank&  bank;
        int                   tableIndex = 0;
        std::unique_ptr<juce::ParameterAttachment> tableAttach;
```

Update `waveIndex` clamp from `0..3` to `0..4`:

```cpp
                    waveIndex = juce::jlimit(0, 4, (int) std::round(v));
```

- [ ] **Step 2: Update callers**

In `Source/UI/SoundTab.h`, the existing scopes `osc1Scope` and `osc2Scope` are constructed with `(apvts, ParamIDs::oscNWaveform)`. Change them.

The bank reference comes from `AISynthProcessor`. Add a public accessor:

In `Source/PluginProcessor.h`, add to `class AISynthProcessor`:

```cpp
    const WavetableBank& getWavetableBank() const noexcept { return synth.getWavetableBank(); }
```

In `Source/Engine/SynthEngine.h`, add a const accessor:

```cpp
    const WavetableBank& getWavetableBank() const noexcept { return wavetableBank; }
```

Then in `SoundTab`, gain a `processor` reference parameter (or the bank reference directly).

> **Implementer note:** if `SoundTab`'s constructor doesn't currently take a processor reference, the cleanest fix is to plumb the `WavetableBank&` through the editor's constructor chain. Read `Source/PluginEditor.cpp` for the existing wiring. Update SoundTab's signature, OscilloscopeView's call sites, and the editor's instantiation in lockstep.

- [ ] **Step 3: Update tests in `Tests/UIConstructionTests.cpp`**

If the test constructs `OscilloscopeView` directly, update the call to pass the new args. If the test only constructs `SoundTab` end-to-end, the test will compile once `SoundTab`'s signature is consistent with its caller (the editor).

- [ ] **Step 4: Build VST3 and run tests**

Expected: 133/133 tests pass; VST3 builds.

- [ ] **Step 5: Commit**

```bash
git add Source/UI/components/OscilloscopeView.h Source/UI/SoundTab.h \
        Source/PluginProcessor.h Source/Engine/SynthEngine.h \
        Tests/UIConstructionTests.cpp
git commit -m "feat(ui): OscilloscopeView renders wavetable cycle when wave == 4"
```

---

## Task 15: Generate.py + replace stub WavetableData.cpp with hand-coded tables

**Files:**
- Create: `tools/wavetable/generate.py`
- Modify: `Source/Engine/WavetableData.cpp` (replaced wholesale by Python output)

**Goal:** Real Organ/Vocal/Noise tables. EP and Bell remain temporary sines until Task 16 (AKWF curation). Python script emits all 5 entries × 10 mips of band-limited data.

- [ ] **Step 1: Create `tools/wavetable/generate.py`**

```python
#!/usr/bin/env python3
"""Generates Source/Engine/WavetableData.cpp from formulas + AKWF wavs.

Run from repo root:
    python tools/wavetable/generate.py

Hand-coded tables (no external file required):
  - Organ: Hammond drawbar 888000000 (9 partials, integer ratios).
  - Vocal: formant comb F1=730/F2=1090/F3=2440 ('Aa' vowel).
  - Noise: deterministic white noise (fixed seed).

AKWF tables (wav file required at the listed path):
  - EP:   tools/wavetable/akwf-sources/<chosen-ep>.wav
  - Bell: tools/wavetable/akwf-sources/<chosen-bell>.wav

If the AKWF wavs are missing, the script falls back to a sine placeholder
for that entry and prints a warning. This is the default state until Task 16
selects real source files.
"""
from __future__ import annotations
import math
import random
import struct
import sys
import wave
from pathlib import Path

ROOT       = Path(__file__).resolve().parent.parent.parent
OUT_PATH   = ROOT / "Source" / "Engine" / "WavetableData.cpp"
AKWF_DIR   = ROOT / "tools" / "wavetable" / "akwf-sources"
NUM_TABLES = 5
NUM_MIPS   = 10
MIP0_SIZE  = 2048

def normalize(samples, peak=0.95):
    m = max(abs(s) for s in samples) or 1.0
    return [s * peak / m for s in samples]

def make_organ():
    """Drawbar 888000000: equal weight on first 9 harmonics (16', 5-1/3', 8',
    4', 2-2/3', 2', 1-3/5', 1-1/3', 1') normalized."""
    weights = [1.0] * 9
    cycle = []
    for i in range(MIP0_SIZE):
        t = i / MIP0_SIZE
        s = sum(w * math.sin(2 * math.pi * (k + 1) * t) for k, w in enumerate(weights))
        cycle.append(s)
    return normalize(cycle)

def make_vocal():
    """Formant comb at F1=730 Hz, F2=1090 Hz, F3=2440 Hz ('Aa' vowel) over a
    single cycle of fundamental. We synthesize via sum of harmonics with
    envelope shaped by gaussian peaks at the formant frequencies, evaluated
    against an arbitrary fundamental f0=200 Hz."""
    f0 = 200.0
    formants = [(730.0, 80.0, 1.0), (1090.0, 90.0, 0.6), (2440.0, 120.0, 0.3)]
    n_harm = MIP0_SIZE // 2  # up to Nyquist of cycle
    amplitudes = []
    for k in range(1, n_harm + 1):
        f = k * f0
        a = 0.0
        for (fc, bw, weight) in formants:
            a += weight * math.exp(-0.5 * ((f - fc) / bw) ** 2)
        amplitudes.append(a)
    cycle = []
    for i in range(MIP0_SIZE):
        t = i / MIP0_SIZE
        s = sum(a * math.sin(2 * math.pi * (k + 1) * t) for k, a in enumerate(amplitudes))
        cycle.append(s)
    return normalize(cycle)

def make_noise():
    """Deterministic white noise — fixed seed so the table is reproducible."""
    rng = random.Random(0xA1)
    return normalize([rng.uniform(-1.0, 1.0) for _ in range(MIP0_SIZE)])

def load_wav_resampled(path: Path):
    """Load a single-cycle wav and resample to MIP0_SIZE via linear interp.
    Returns None if the file doesn't exist."""
    if not path.exists():
        return None
    with wave.open(str(path), 'rb') as w:
        n_frames = w.getnframes()
        n_chans  = w.getnchannels()
        sw       = w.getsampwidth()
        raw      = w.readframes(n_frames)
    if sw == 2:
        fmt = '<' + 'h' * (n_frames * n_chans)
        ints = struct.unpack(fmt, raw)
        mono = [ints[i] / 32768.0 for i in range(0, n_frames * n_chans, n_chans)]
    elif sw == 3:
        # 24-bit packed
        mono = []
        for i in range(0, len(raw), 3 * n_chans):
            b = raw[i:i+3]
            v = int.from_bytes(b, 'little', signed=True)
            mono.append(v / (1 << 23))
    else:
        raise RuntimeError(f"unsupported wav sample width {sw} in {path}")
    # Linear-interp resample to MIP0_SIZE
    out = []
    for i in range(MIP0_SIZE):
        pos = i * (len(mono) - 1) / MIP0_SIZE
        a = int(pos)
        f = pos - a
        b = min(a + 1, len(mono) - 1)
        out.append(mono[a] + f * (mono[b] - mono[a]))
    return normalize(out)

def make_ep():
    candidate = AKWF_DIR / "ep.wav"
    cycle = load_wav_resampled(candidate)
    if cycle is None:
        print(f"WARN: {candidate} missing — EP entry falling back to sine placeholder.",
              file=sys.stderr)
        return [math.sin(2 * math.pi * i / MIP0_SIZE) for i in range(MIP0_SIZE)]
    return cycle

def make_bell():
    candidate = AKWF_DIR / "bell.wav"
    cycle = load_wav_resampled(candidate)
    if cycle is None:
        print(f"WARN: {candidate} missing — Bell entry falling back to sine placeholder.",
              file=sys.stderr)
        return [math.sin(2 * math.pi * i / MIP0_SIZE) for i in range(MIP0_SIZE)]
    return cycle

def fft_real(samples):
    """Naive DFT; OK for MIP0_SIZE=2048. Returns list of complex bins."""
    N = len(samples)
    # Use Python's built-in cmath via a simple DFT — for speed in real generation,
    # swap to numpy if available. Kept stdlib-only here so the script has zero deps.
    import cmath
    return [sum(samples[n] * cmath.exp(-2j * math.pi * k * n / N) for n in range(N))
            for k in range(N)]

def ifft_real(bins):
    N = len(bins)
    import cmath
    return [
        (sum(bins[k] * cmath.exp(2j * math.pi * k * n / N) for k in range(N)) / N).real
        for n in range(N)
    ]

def bandlimit_to_mip(cycle, mip_idx):
    """Zero bins above the band-limit appropriate for mip[mip_idx], then IFFT
    and downsample by 2^mip_idx."""
    if mip_idx == 0:
        return cycle[:]
    N = len(cycle)
    bins = fft_real(cycle)
    half = N // 2
    cutoff = half >> mip_idx
    for k in range(cutoff + 1, N - cutoff):
        bins[k] = 0
    full = ifft_real(bins)
    step = 1 << mip_idx
    return [full[i * step] for i in range(N // step)]

def emit_array(name, samples):
    floats_per_line = 8
    lines = [f"static constexpr float {name}[{len(samples)}] = {{"]
    for i in range(0, len(samples), floats_per_line):
        chunk = samples[i:i+floats_per_line]
        lines.append("    " + ", ".join(f"{x:.7f}f" for x in chunk) + ",")
    lines.append("};")
    return "\n".join(lines)

def main():
    tables = {
        "organ": make_organ(),
        "ep":    make_ep(),
        "bell":  make_bell(),
        "vocal": make_vocal(),
        "noise": make_noise(),
    }
    out = []
    out.append("// AUTO-GENERATED by tools/wavetable/generate.py — do not edit by hand.")
    out.append("// Re-run that script after changing source data.")
    out.append("//")
    out.append("// Source files (where applicable):")
    out.append(f"//   ep:   tools/wavetable/akwf-sources/ep.wav")
    out.append(f"//   bell: tools/wavetable/akwf-sources/bell.wav")
    out.append("")
    out.append('#include "WavetableBank.h"')
    out.append("")
    out.append("namespace {")
    for name, cycle in tables.items():
        for mip in range(NUM_MIPS):
            band = bandlimit_to_mip(cycle, mip)
            out.append(emit_array(f"{name}_mip{mip}", band))
            out.append("")
    out.append("} // namespace")
    out.append("")
    out.append("WavetableBank::WavetableBank() {")
    for idx, name in enumerate(["organ", "ep", "bell", "vocal", "noise"]):
        for mip in range(NUM_MIPS):
            out.append(f"    entries[{idx}].mips[{mip}] = {name}_mip{mip};")
    out.append("}")
    OUT_PATH.write_text("\n".join(out) + "\n")
    print(f"wrote {OUT_PATH}")

if __name__ == "__main__":
    main()
```

- [ ] **Step 2: Run the generator**

```bash
python tools/wavetable/generate.py
```

Expected output:
```
WARN: tools/wavetable/akwf-sources/ep.wav missing — EP entry falling back to sine placeholder.
WARN: tools/wavetable/akwf-sources/bell.wav missing — Bell entry falling back to sine placeholder.
wrote .../Source/Engine/WavetableData.cpp
```

The new `WavetableData.cpp` contains real Organ/Vocal/Noise tables and sine placeholders for EP and Bell.

> Naive DFT/IDFT for 2048-point signals × 10 mips × 5 tables is slow but acceptable as a one-off. If runtime exceeds ~5 minutes, swap `fft_real`/`ifft_real` for `numpy.fft.rfft`/`irfft`. The script imports numpy lazily only if you change it; the stdlib version is the default.

- [ ] **Step 3: Build and run tests**

```bash
cmake --build build --target AISynthTests --config Release && \
  ./build/AISynthTests_artefacts/Release/AISynth\ Tests.exe
```

Expected: 133/133 tests pass. The bank tests now exercise real-data tables (energy/RMS thresholds may need fine-tuning if Vocal or Noise sit outside the heuristic — adjust the test bound, not the data).

- [ ] **Step 4: Commit**

```bash
git add tools/wavetable/generate.py Source/Engine/WavetableData.cpp
git commit -m "feat(wt): generate.py + WavetableData with Organ/Vocal/Noise (EP/Bell stubbed)"
```

---

## Task 16: AKWF cherry-pick — EP and Bell

**Files:**
- Add: `tools/wavetable/akwf-sources/ep.wav`
- Add: `tools/wavetable/akwf-sources/bell.wav`
- Modify: `Source/Engine/WavetableData.cpp` (regenerated)

**Goal:** Replace the EP and Bell sine placeholders with audited real AKWF cycles per the spec's curation criteria (pitch-detectable, tonal, zero-crossing-aligned, distinct).

- [ ] **Step 1: Download or locate AKWF candidates**

Source: https://www.adventurekid.se/akrt/waveforms/adventure-kid-waveforms/

For EP: candidates in `AKWF_epiano/`.
For Bell: candidates in `AKWF_bell/`.

Place the chosen `.wav` files at:
- `tools/wavetable/akwf-sources/ep.wav`
- `tools/wavetable/akwf-sources/bell.wav`

- [ ] **Step 2: Audition (manual)**

For each candidate (pre-selected to a shortlist of 3–4 per category):
1. Load in Reaper as a single-cycle wave (loop the file).
2. Play through SynthVibe's filter + amp envelope at C3 (130 Hz).
3. Apply the spec's 4 criteria:
   - Pitch-detectable
   - Tonal & musically usable
   - Symmetric / zero-crossing-aligned at boundaries
   - Distinct from existing tables
4. Pick the strongest candidate per category.

> Budget: half a day. If no candidate meets all 4 criteria for a given category after auditioning the AKWF shortlist, see fallback in the spec's Risks section: synthesize EP from `bell-component × ratio 2.76 + tine partial decay` additive formula.

- [ ] **Step 3: Update generate.py with the chosen filenames**

In `tools/wavetable/generate.py`, the file paths are already `ep.wav` and `bell.wav` — no edit needed. The header comment in the generated `WavetableData.cpp` should reflect the actual chosen AKWF source filenames; edit `generate.py`'s "Source files" comment block to record the chosen filenames before regenerating:

```python
out.append("// Source files (where applicable):")
out.append(f"//   ep:   tools/wavetable/akwf-sources/ep.wav  (from AKWF_epiano/<chosen>)")
out.append(f"//   bell: tools/wavetable/akwf-sources/bell.wav (from AKWF_bell/<chosen>)")
```

Replace `<chosen>` with the actual filename you picked.

- [ ] **Step 4: Regenerate WavetableData.cpp**

```bash
python tools/wavetable/generate.py
```

Expected: no warnings (both wavs found). New `WavetableData.cpp` written.

- [ ] **Step 5: Build and run tests**

```bash
cmake --build build --target AISynth_VST3 --config Release && \
cmake --build build --target AISynthTests --config Release && \
  ./build/AISynthTests_artefacts/Release/AISynth\ Tests.exe
```

Expected: 133/133 pass.

- [ ] **Step 6: Commit**

```bash
git add tools/wavetable/akwf-sources/ep.wav tools/wavetable/akwf-sources/bell.wav \
        tools/wavetable/generate.py Source/Engine/WavetableData.cpp
git commit -m "feat(wt): AKWF EP and Bell cherry-picked, real WavetableData regenerated"
```

---

## Task 17: THIRDPARTY.md attribution

**Files:**
- Create: `THIRDPARTY.md`

- [ ] **Step 1: Create `THIRDPARTY.md`**

```markdown
# Third-Party Resources

## Wavetables

The "EP" and "Bell" wavetable entries (`Source/Engine/WavetableData.cpp`) are
adapted from single-cycle waveforms in the **Adventure Kid Waveforms (AKWF)**
collection by Kristoffer Ekstrand.

- Source: https://www.adventurekid.se/akrt/waveforms/adventure-kid-waveforms/
- License: CC0 / public domain (no attribution required, but acknowledged here).

The original `.wav` source files are committed under
`tools/wavetable/akwf-sources/` for reproducibility.
```

- [ ] **Step 2: Commit**

```bash
git add THIRDPARTY.md
git commit -m "docs: THIRDPARTY.md — AKWF attribution"
```

---

## Task 18: Manual smoke test in Reaper

**Files:** none modified — verification step only.

**Goal:** End-to-end audible verification that the engine + UI + AI integration all work in production.

- [ ] **Step 1: Deploy the build**

```bash
./deploy.ps1
```

Expected: new `AI Synth` VST3 lands in `C:\Program Files\Common Files\VST3\` with an incremented version number.

- [ ] **Step 2: Reaper smoke test — timbre distinctness**

1. Load AI Synth on a track.
2. Set Osc1.wave = Wavetable.
3. Cycle through Osc1.table = Organ → EP → Bell → Vocal → Noise. Each must sound audibly distinct.
4. For each table, verify there's no audible aliasing at high notes (play C7, C8) and no DC offset (no thump on note-on).

- [ ] **Step 3: Reaper smoke test — AI prompt round-trip**

1. Open the AI prompt modal.
2. Send the prompt: `make a clean organ lead`.
3. Verify the response: at least one of the 1–4 variations should set `osc1.wave = 4` (Wavetable) and `osc1.table = 0` (Organ).
4. Apply the variation. The resulting sound should be recognizably an organ lead, not a generic saw with a filter.

- [ ] **Step 4: Reaper smoke test — preset compatibility**

1. Load a preset saved before this branch (any of the existing factory presets).
2. The preset should sound identical to before — `osc1.table` defaults to 0 (Organ), but `osc1.wave` is 0–3 so the table is ignored.
3. No clicks, no NaN, no level changes.

- [ ] **Step 5: If all 3 smoke tests pass**

This is the merge gate. Document any issues found before declaring done. If the AI prompt test fails (Claude doesn't pick Wavetable for "organ lead"), iterate on the SystemPrompt wording — but expect this round-trip to work given the V1 limitations are documented.

- [ ] **Step 6: Document smoke-test result**

Append to the plan a short post-test note in a follow-up commit:

```bash
git add docs/superpowers/plans/2026-04-28-phase-b-wavetable.md
git commit -m "docs(plans): Phase B smoke-test result — N/N tests pass, ready to merge"
```

---

## Final state

- Tests: 133/133 (or higher) green.
- Plugin: `AI Synth v<next>.vst3` deployed and verified in Reaper.
- AI integration: prompt `clean organ lead` produces a recognizable organ via Wavetable.
- Migration: existing presets load unchanged.

Merge to `main` only after the smoke test passes — confirm with the user before merging per the project's branch-merge convention.
