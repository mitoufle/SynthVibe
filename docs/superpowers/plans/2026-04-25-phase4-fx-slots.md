# Phase 4 — FX Tab refactor (slot-driven runner) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the hardcoded `FXChain` (Drive → Chorus → Delay → Reverb) with a 10-slot generic FX runner. Each slot has a type, bypass, mix, and 4 generic params (p1..p4). UI rewrites `FxTab` around a `FxChainStrip` of 10 `FxSlotCard`s. Legacy `fx.delay/chorus/drive/reverb.*` parameter IDs are deleted (presets that referenced them lose their FX values — known break).

**Architecture:** Each slot pre-allocates one instance of every concrete effect class (Drive, Chorus, Delay, Reverb, Eq3) — `~7 MB` extra at 44.1 kHz / 10 slots. `FxSlot::process()` dispatches by stored `Type` enum (no virtual calls, no audio-thread allocation). `FxTypeMap` table holds per-type label + the p1..p4 → struct field mapping consumed by both the audio engine and the UI labeling.

**Tech Stack:** C++17, JUCE 7.0.9, `juce::AudioProcessorValueTreeState`, `juce::UnitTest`. Five DSP classes (Drive/Chorus/Delay/Reverb already shipped; Eq3 added in Task 4).

**Build:** `./build-with-vs.bat` (loads vcvars64 + runs cmake/msbuild).
**Tests:** `./build/AISynthTests_artefacts/Release/AISynth Tests.exe`
**Deploy:** `./deploy.ps1` (PowerShell only, not bash).

**Branch:** stay on `phase2-sound-tab`. No new branch needed.

---

## Open decision: stretch effect (EQ3 or Comp)

You picked "Stretch" for the FX V1 — meaning four wrappers (Drive, Chorus, Delay, Reverb already shipped) **plus one new DSP block**. The plan assumes **EQ3** and writes the EQ implementation in Task 4. The two candidates:

- **EQ3 (assumed)** — 3-band EQ: low shelf @ 250 Hz, peak @ p4 freq, high shelf @ 5000 Hz. p1=low gain dB, p2=mid gain dB, p3=high gain dB, p4=mid freq. ~40 lines of biquad math (RBJ Audio EQ Cookbook). Bullet-proof at edge values.
- **Comp** — feedback compressor: p1=threshold dB, p2=ratio, p3=attack ms, p4=release ms. JUCE ships `juce::dsp::Compressor` so DSP is free, but parameter mapping is more sensitive (bad attack/release combos audibly pump).

If you want **Comp** instead of EQ3, only Task 4 changes (different DSP class) and Task 5's `FxTypeMap` row for the new type. All other tasks are identical. Tell me to flip and I'll rewrite Tasks 4 + 5.

---

## What this costs

- **Memory:** every slot pre-allocates one `Delay` (≈700 KB at 44.1 kHz) + four other effects. 10 slots ≈ **7 MB at 44.1 kHz**, ≈15 MB at 96 kHz, ≈30 MB at 192 kHz. Acceptable for a synth plugin.
- **Preset break:** any preset saved before this change loses its delay/chorus/drive/reverb values (their param IDs are gone from APVTS). Synth params (osc/filter/env/master/mod) are unaffected. The user accepted this trade-off when they picked Migration option `1.A`.
- **Stub effects:** types `Phaser / Flanger / Bitcrush / Filter / Comp` (or `Eq3` if you pick Comp) appear in the type dropdown but pass the buffer through unchanged. Each gets a one-line stub case in `FxSlot::process()`. No UI difference.

---

## File Structure

| File | Status | Responsibility |
|---|---|---|
| `Source/Parameters/ParameterIDs.h` | modify | add `fx1Type` … `fx10P4` (70 IDs); **remove** `delayTime`, `delayFeedback`, `delayMix`, `chorusRate`, `chorusDepth`, `chorusMix`, `reverbRoom`, `reverbDamp`, `reverbMix`, `driveType`, `driveAmount`, `driveMix` |
| `Source/Parameters/ParameterLayout.cpp` | modify | register 10 × 7 = 70 fx params via a loop; **remove** the four hardcoded fx sections (Delay, Chorus, Reverb, Drive) |
| `Source/FX/Eq3.h` | create | 3-band EQ DSP class — interface mirrors existing FX classes (`prepare`/`setParams`/`process`/`reset`) |
| `Source/FX/Eq3.cpp` | create | RBJ biquad coefficients for low shelf + peak + high shelf, processed in series |
| `Source/FX/FxParams.h` | create | `FxSlotParams` struct (`type`, `bypass`, `mix`, `p1..p4`) + `FxType` enum + `kFxTypeCount` |
| `Source/FX/FxTypeMap.h` | create | declares `kFxTypeNames[]` (UI labels for the 11 types) and `kFxParamLabels[][]` (per-type p1..p4 labels) |
| `Source/FX/FxTypeMap.cpp` | create | defines those two tables |
| `Source/FX/FxSlot.h` | create | owns one Drive/Chorus/Delay/Reverb/Eq3 instance; dispatches `process()` via `switch (type)` |
| `Source/FX/FxSlot.cpp` | create | implements `prepare`/`setParams`/`process`/`reset`; tracks last-active type and resets the newly-active effect on type change |
| `Source/FX/FxRunner.h` | create | holds 10 `FxSlot`s and walks them in order |
| `Source/FX/FxRunner.cpp` | create | implements `prepare`/`setSnapshot`/`process`/`reset` |
| `Source/FX/FXChain.h` | delete | superseded by `FxRunner` |
| `Source/FX/FXChain.cpp` | delete | superseded by `FxRunner` |
| `Source/PluginProcessor.h` | modify | replace `FXChain fxChain;` field with `FxRunner fxRunner;`; remove `buildDelay/Chorus/Drive/ReverbParams` decls; add `buildFxSnapshot` |
| `Source/PluginProcessor.cpp` | modify | drop the four `build*Params()` impls; add `buildFxSnapshot()`; rewrite the `processBlock` FX dispatch line |
| `Source/UI/components/FxSlotTypePicker.h` | create | combo of 11 types bound to `fx.N.type` |
| `Source/UI/components/FxSlotCard.h` | create | one slot UI: header (idx + type combo + bypass dot + mix knob) + 4 generic param knobs |
| `Source/UI/components/FxChainStrip.h` | create | 2 × 5 grid of `FxSlotCard` |
| `Source/UI/FXTab.h` | rewrite | replace 4-column hardcoded panel with `FxChainStrip` |
| `Tests/ParameterIdMigrationTests.cpp` | modify | add fx slot ID format tests, fx slot defaults tests, **negative legacy tests** |
| `Tests/UIConstructionTests.cpp` | modify | construction tests for `FxSlotTypePicker`, `FxSlotCard`, `FxChainStrip`, refactored `FxTab` |
| `Tests/FxRunnerTests.cpp` | create | signal-flow invariants: empty chain = passthrough, all-bypassed = passthrough, drive at amount=0 = ~passthrough |
| `Tests/Eq3Tests.cpp` | create | flat EQ (all gains 0 dB) ≈ passthrough; large positive low gain raises a 100-Hz sine peak |
| `CMakeLists.txt` | modify | drop `FXChain.cpp`; add `Eq3.cpp`, `FxTypeMap.cpp`, `FxSlot.cpp`, `FxRunner.cpp`; add test files |

---

## Task 1: Declare `fx.N.*` parameter IDs (and remove legacy IDs)

**Files:**
- Modify: `Source/Parameters/ParameterIDs.h`
- Modify: `Tests/ParameterIdMigrationTests.cpp`

This is the **destructive removal** task — both adds new IDs *and* deletes the four legacy fx blocks in one commit. Confining the legacy delete to a single, clearly-titled commit makes git-bisect cleaner if a preset bug surfaces later.

- [ ] **Step 1: Write the failing tests**

Append to the body of `runTest()` in `Tests/ParameterIdMigrationTests.cpp`, after the existing `"Phase 2b new parameters are registered with correct defaults"` block:

```cpp
        beginTest("FX slot IDs follow fx.N.suffix scheme");
        expectEquals(juce::String(ParamIDs::fx1Type),   juce::String("fx.1.type"));
        expectEquals(juce::String(ParamIDs::fx1Bypass), juce::String("fx.1.bypass"));
        expectEquals(juce::String(ParamIDs::fx1Mix),    juce::String("fx.1.mix"));
        expectEquals(juce::String(ParamIDs::fx1P1),     juce::String("fx.1.p1"));
        expectEquals(juce::String(ParamIDs::fx1P4),     juce::String("fx.1.p4"));
        expectEquals(juce::String(ParamIDs::fx10Type),  juce::String("fx.10.type"));
        expectEquals(juce::String(ParamIDs::fx10P4),    juce::String("fx.10.p4"));

        beginTest("Legacy hardcoded fx.* IDs are removed (preset break is intentional)");
        {
            juce::AudioProcessorGraph dummyGraph;
            juce::AudioProcessorValueTreeState apvts(
                dummyGraph, nullptr, "AISynthState", ParameterLayout::create());

            // Each of these used to exist in Phase 1; Phase 4 deletes them
            // as part of the slot migration. APVTS should report nullptr.
            for (auto* legacyId : {
                "fx.delay.time", "fx.delay.feedback", "fx.delay.mix",
                "fx.chorus.rate", "fx.chorus.depth", "fx.chorus.mix",
                "fx.reverb.room", "fx.reverb.damp", "fx.reverb.mix",
                "fx.drive.type", "fx.drive.amount", "fx.drive.mix" })
            {
                expect(apvts.getParameter(legacyId) == nullptr,
                       juce::String("legacy id should be gone: ") + legacyId);
            }
        }
```

- [ ] **Step 2: Build and confirm tests fail**

```
./build-with-vs.bat
```

Expected: compile errors — `ParamIDs::fx1Type` (etc.) not declared, **and** the legacy negative-test will not compile yet either because `ParamIDs::delayTime` is still referenced from `PluginProcessor.cpp` and `FXTab.h`. The compile error from the *negative test alone* (which only uses string literals like `"fx.delay.time"`) will be the new IDs missing.

- [ ] **Step 3: Edit `Source/Parameters/ParameterIDs.h`**

**(a)** **Delete** the four legacy fx blocks (lines 60–80 in the current file — `// Delay`, `// Chorus`, `// Reverb`, `// Drive`):

```cpp
    // Delay (interim — Phase 3 migrates these into the generic fx.N slot scheme)
    inline constexpr const char* delayTime       = "fx.delay.time";
    // ... through ...
    inline constexpr const char* driveMix        = "fx.drive.mix";
```

→ remove all 12 of those `inline constexpr` lines (and their three section-header comments).

**(b)** Append before the closing `}` of `namespace ParamIDs`:

```cpp
    // FX Chain — 10 generic slots, each carrying type+bypass+mix+p1..p4 (= 70 params).
    // The meaning of p1..p4 depends on the slot's `type` choice index. See
    // Source/FX/FxTypeMap.cpp for the per-type p1..p4 labels and DSP mapping.
    inline constexpr const char* fx1Type   = "fx.1.type";
    inline constexpr const char* fx1Bypass = "fx.1.bypass";
    inline constexpr const char* fx1Mix    = "fx.1.mix";
    inline constexpr const char* fx1P1     = "fx.1.p1";
    inline constexpr const char* fx1P2     = "fx.1.p2";
    inline constexpr const char* fx1P3     = "fx.1.p3";
    inline constexpr const char* fx1P4     = "fx.1.p4";
    inline constexpr const char* fx2Type   = "fx.2.type";
    inline constexpr const char* fx2Bypass = "fx.2.bypass";
    inline constexpr const char* fx2Mix    = "fx.2.mix";
    inline constexpr const char* fx2P1     = "fx.2.p1";
    inline constexpr const char* fx2P2     = "fx.2.p2";
    inline constexpr const char* fx2P3     = "fx.2.p3";
    inline constexpr const char* fx2P4     = "fx.2.p4";
    inline constexpr const char* fx3Type   = "fx.3.type";
    inline constexpr const char* fx3Bypass = "fx.3.bypass";
    inline constexpr const char* fx3Mix    = "fx.3.mix";
    inline constexpr const char* fx3P1     = "fx.3.p1";
    inline constexpr const char* fx3P2     = "fx.3.p2";
    inline constexpr const char* fx3P3     = "fx.3.p3";
    inline constexpr const char* fx3P4     = "fx.3.p4";
    inline constexpr const char* fx4Type   = "fx.4.type";
    inline constexpr const char* fx4Bypass = "fx.4.bypass";
    inline constexpr const char* fx4Mix    = "fx.4.mix";
    inline constexpr const char* fx4P1     = "fx.4.p1";
    inline constexpr const char* fx4P2     = "fx.4.p2";
    inline constexpr const char* fx4P3     = "fx.4.p3";
    inline constexpr const char* fx4P4     = "fx.4.p4";
    inline constexpr const char* fx5Type   = "fx.5.type";
    inline constexpr const char* fx5Bypass = "fx.5.bypass";
    inline constexpr const char* fx5Mix    = "fx.5.mix";
    inline constexpr const char* fx5P1     = "fx.5.p1";
    inline constexpr const char* fx5P2     = "fx.5.p2";
    inline constexpr const char* fx5P3     = "fx.5.p3";
    inline constexpr const char* fx5P4     = "fx.5.p4";
    inline constexpr const char* fx6Type   = "fx.6.type";
    inline constexpr const char* fx6Bypass = "fx.6.bypass";
    inline constexpr const char* fx6Mix    = "fx.6.mix";
    inline constexpr const char* fx6P1     = "fx.6.p1";
    inline constexpr const char* fx6P2     = "fx.6.p2";
    inline constexpr const char* fx6P3     = "fx.6.p3";
    inline constexpr const char* fx6P4     = "fx.6.p4";
    inline constexpr const char* fx7Type   = "fx.7.type";
    inline constexpr const char* fx7Bypass = "fx.7.bypass";
    inline constexpr const char* fx7Mix    = "fx.7.mix";
    inline constexpr const char* fx7P1     = "fx.7.p1";
    inline constexpr const char* fx7P2     = "fx.7.p2";
    inline constexpr const char* fx7P3     = "fx.7.p3";
    inline constexpr const char* fx7P4     = "fx.7.p4";
    inline constexpr const char* fx8Type   = "fx.8.type";
    inline constexpr const char* fx8Bypass = "fx.8.bypass";
    inline constexpr const char* fx8Mix    = "fx.8.mix";
    inline constexpr const char* fx8P1     = "fx.8.p1";
    inline constexpr const char* fx8P2     = "fx.8.p2";
    inline constexpr const char* fx8P3     = "fx.8.p3";
    inline constexpr const char* fx8P4     = "fx.8.p4";
    inline constexpr const char* fx9Type   = "fx.9.type";
    inline constexpr const char* fx9Bypass = "fx.9.bypass";
    inline constexpr const char* fx9Mix    = "fx.9.mix";
    inline constexpr const char* fx9P1     = "fx.9.p1";
    inline constexpr const char* fx9P2     = "fx.9.p2";
    inline constexpr const char* fx9P3     = "fx.9.p3";
    inline constexpr const char* fx9P4     = "fx.9.p4";
    inline constexpr const char* fx10Type   = "fx.10.type";
    inline constexpr const char* fx10Bypass = "fx.10.bypass";
    inline constexpr const char* fx10Mix    = "fx.10.mix";
    inline constexpr const char* fx10P1     = "fx.10.p1";
    inline constexpr const char* fx10P2     = "fx.10.p2";
    inline constexpr const char* fx10P3     = "fx.10.p3";
    inline constexpr const char* fx10P4     = "fx.10.p4";
```

- [ ] **Step 4: Update the existing `IDs use dot-separated scheme` test (it still references `delayTime`)**

In `Tests/ParameterIdMigrationTests.cpp`, find and replace this line in the existing first test block:

```cpp
        expectEquals(juce::String(ParamIDs::delayTime),     juce::String("fx.delay.time"));
```

with:

```cpp
        expectEquals(juce::String(ParamIDs::fx1Mix),        juce::String("fx.1.mix"));
```

This keeps the spirit of the test (verifies dot-separated scheme) but uses an ID that still exists.

- [ ] **Step 5: Build — DO NOT expect everything to compile yet**

```
./build-with-vs.bat
```

Expected: `Tests/ParameterIdMigrationTests.cpp` compiles. The PLUGIN target (`AISynth`) will fail because `PluginProcessor.cpp` and `FXTab.h` still reference `ParamIDs::delayTime` etc. Tasks 8 and 12 fix those. The TEST target (`AISynthTests`) doesn't include `PluginProcessor.cpp` or `FXTab.h` — so it should build successfully.

If the test target fails for any reason other than missing param registrations, stop and reread Step 3.

- [ ] **Step 6: Run the tests target — `fx slot IDs` block passes; legacy negative test should also pass (the IDs are gone from the layout because the layout never registered them under the new names yet, but more importantly because Task 1 only declared constants — they are not yet registered in `ParameterLayout.cpp`)**

Wait — at this point Task 2 hasn't run, so neither legacy nor new fx slot IDs are registered. The legacy negative test will pass (correct: `apvts.getParameter("fx.delay.time")` returns `nullptr`). But there's a catch: `ParameterLayout.cpp` still has the old fx blocks referencing the removed `ParamIDs::delayTime` constants — so the test build also fails at this point. We need to fix `ParameterLayout.cpp` too in this same task.

Re-do this — go to Step 7.

- [ ] **Step 7: Edit `Source/Parameters/ParameterLayout.cpp` — delete the four legacy fx sections**

Remove lines 159–211 (the `// Delay`, `// Chorus`, `// Reverb`, `// Drive` blocks). Specifically delete this entire span:

```cpp
    // -----------------------------------------------------------------------
    // Delay
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::delayTime, "Delay Time",
        NormalisableRange<float>(1.f, 2000.f, 0.1f, 0.4f), 250.f));
    // ... through ...
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::driveMix, "Drive Mix",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 0.f));
```

(All four blocks contiguous; ~50 lines.) Do **not** add the new fx slot registrations here yet — that's Task 2.

- [ ] **Step 8: Build the test target**

```
./build-with-vs.bat
```

Expected: `AISynthTests` compiles. `AISynth` plugin target still fails (still uses removed IDs). That's fine — Tasks 8 and 12 close that gap.

- [ ] **Step 9: Run tests**

```
"./build/AISynthTests_artefacts/Release/AISynth Tests.exe"
```

Expected: both new test blocks (`FX slot IDs follow fx.N.suffix scheme`, `Legacy hardcoded fx.* IDs are removed`) pass. The Phase 2b/Mod tests still pass (none of them touched fx params).

- [ ] **Step 10: Commit**

```
git add Source/Parameters/ParameterIDs.h Source/Parameters/ParameterLayout.cpp Tests/ParameterIdMigrationTests.cpp
git commit -m "feat(params)!: declare fx.1..10.{type,bypass,mix,p1..p4}; drop legacy fx.delay/chorus/drive/reverb IDs"
```

The `!` after `feat(params)` signals a breaking change (presets touching old IDs will silently lose their FX values).

---

## Task 2: Register `fx.N.*` parameters in ParameterLayout

**Files:**
- Modify: `Source/Parameters/ParameterLayout.cpp`
- Modify: `Tests/ParameterIdMigrationTests.cpp`

- [ ] **Step 1: Write the failing test**

Append a new block to `runTest()` after the `Legacy hardcoded fx.* IDs are removed` block:

```cpp
        beginTest("FX slots register with correct defaults");
        {
            juce::AudioProcessorGraph dummyGraph;
            juce::AudioProcessorValueTreeState apvts(
                dummyGraph, nullptr, "AISynthState", ParameterLayout::create());

            auto readFloat = [&](const char* id) {
                auto* p = apvts.getRawParameterValue(id);
                expect(p != nullptr, juce::String("missing param: ") + id);
                return p ? p->load() : 0.f;
            };

            // Type default = 0 ("None")
            expectEquals(readFloat(ParamIDs::fx1Type), 0.f);
            // Bypass default = false (0)
            expectEquals(readFloat(ParamIDs::fx1Bypass), 0.f);
            // Mix default = 1.0 (full wet — slot is bypassed via type=None or bypass flag)
            expectWithinAbsoluteError(readFloat(ParamIDs::fx1Mix), 1.f, 0.001f);
            // p1..p4 default = 0.5 (mid range)
            expectWithinAbsoluteError(readFloat(ParamIDs::fx1P1), 0.5f, 0.001f);
            expectWithinAbsoluteError(readFloat(ParamIDs::fx1P4), 0.5f, 0.001f);

            // Slot 10 also present
            expect(apvts.getRawParameterValue(ParamIDs::fx10P4) != nullptr,
                   "slot 10 p4 must be registered");

            // 11 type choices: None + 10 effect types
            auto* typeParam = dynamic_cast<juce::AudioParameterChoice*>(
                apvts.getParameter(ParamIDs::fx1Type));
            expect(typeParam != nullptr, "fx.1.type must be a choice param");
            if (typeParam != nullptr)
                expectEquals(typeParam->choices.size(), 11);
        }
```

- [ ] **Step 2: Build and confirm fail**

```
./build-with-vs.bat
```

Expected: test build fails inside `runTest` — `apvts.getRawParameterValue("fx.1.type")` returns nullptr because Task 2's registration hasn't happened. The lambda's `expect(p != nullptr, ...)` triggers.

- [ ] **Step 3: Append the fx-slot registration block to `Source/Parameters/ParameterLayout.cpp`**

Insert just before `return { params.begin(), params.end() };` at the end of `ParameterLayout::create()`:

```cpp
    // -----------------------------------------------------------------------
    // FX Chain — 10 generic slots
    // -----------------------------------------------------------------------
    {
        const StringArray typeLabels {
            "None", "Drive", "Chorus", "Delay", "Reverb",
            "EQ3", "Comp", "Phaser", "Flanger", "Bitcrush", "Filter"
        };

        struct SlotIds { const char* type; const char* bypass; const char* mix;
                         const char* p1; const char* p2; const char* p3; const char* p4; };
        const SlotIds slots[10] = {
            { ParamIDs::fx1Type,  ParamIDs::fx1Bypass,  ParamIDs::fx1Mix,
              ParamIDs::fx1P1,    ParamIDs::fx1P2,      ParamIDs::fx1P3,    ParamIDs::fx1P4  },
            { ParamIDs::fx2Type,  ParamIDs::fx2Bypass,  ParamIDs::fx2Mix,
              ParamIDs::fx2P1,    ParamIDs::fx2P2,      ParamIDs::fx2P3,    ParamIDs::fx2P4  },
            { ParamIDs::fx3Type,  ParamIDs::fx3Bypass,  ParamIDs::fx3Mix,
              ParamIDs::fx3P1,    ParamIDs::fx3P2,      ParamIDs::fx3P3,    ParamIDs::fx3P4  },
            { ParamIDs::fx4Type,  ParamIDs::fx4Bypass,  ParamIDs::fx4Mix,
              ParamIDs::fx4P1,    ParamIDs::fx4P2,      ParamIDs::fx4P3,    ParamIDs::fx4P4  },
            { ParamIDs::fx5Type,  ParamIDs::fx5Bypass,  ParamIDs::fx5Mix,
              ParamIDs::fx5P1,    ParamIDs::fx5P2,      ParamIDs::fx5P3,    ParamIDs::fx5P4  },
            { ParamIDs::fx6Type,  ParamIDs::fx6Bypass,  ParamIDs::fx6Mix,
              ParamIDs::fx6P1,    ParamIDs::fx6P2,      ParamIDs::fx6P3,    ParamIDs::fx6P4  },
            { ParamIDs::fx7Type,  ParamIDs::fx7Bypass,  ParamIDs::fx7Mix,
              ParamIDs::fx7P1,    ParamIDs::fx7P2,      ParamIDs::fx7P3,    ParamIDs::fx7P4  },
            { ParamIDs::fx8Type,  ParamIDs::fx8Bypass,  ParamIDs::fx8Mix,
              ParamIDs::fx8P1,    ParamIDs::fx8P2,      ParamIDs::fx8P3,    ParamIDs::fx8P4  },
            { ParamIDs::fx9Type,  ParamIDs::fx9Bypass,  ParamIDs::fx9Mix,
              ParamIDs::fx9P1,    ParamIDs::fx9P2,      ParamIDs::fx9P3,    ParamIDs::fx9P4  },
            { ParamIDs::fx10Type, ParamIDs::fx10Bypass, ParamIDs::fx10Mix,
              ParamIDs::fx10P1,   ParamIDs::fx10P2,     ParamIDs::fx10P3,   ParamIDs::fx10P4 },
        };

        for (int i = 0; i < 10; ++i)
        {
            const auto& s = slots[i];
            const auto label = "FX " + String(i + 1) + " ";

            params.push_back(std::make_unique<AudioParameterChoice>(
                s.type, label + "Type", typeLabels, 0));
            params.push_back(std::make_unique<AudioParameterBool>(
                s.bypass, label + "Bypass", false));
            params.push_back(std::make_unique<AudioParameterFloat>(
                s.mix, label + "Mix",
                NormalisableRange<float>(0.f, 1.f, 0.001f), 1.f));
            params.push_back(std::make_unique<AudioParameterFloat>(
                s.p1, label + "P1",
                NormalisableRange<float>(0.f, 1.f, 0.001f), 0.5f));
            params.push_back(std::make_unique<AudioParameterFloat>(
                s.p2, label + "P2",
                NormalisableRange<float>(0.f, 1.f, 0.001f), 0.5f));
            params.push_back(std::make_unique<AudioParameterFloat>(
                s.p3, label + "P3",
                NormalisableRange<float>(0.f, 1.f, 0.001f), 0.5f));
            params.push_back(std::make_unique<AudioParameterFloat>(
                s.p4, label + "P4",
                NormalisableRange<float>(0.f, 1.f, 0.001f), 0.5f));
        }
    }
```

- [ ] **Step 4: Build and run tests**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynth Tests.exe"
```

Expected: `FX slots register with correct defaults` passes (alongside everything else). The `AISynth` plugin target still fails to build — that's OK, Task 8 fixes that.

- [ ] **Step 5: Commit**

```
git add Source/Parameters/ParameterLayout.cpp Tests/ParameterIdMigrationTests.cpp
git commit -m "feat(params): register 10 fx slots with type/bypass/mix/p1..p4"
```

---

## Task 3: `FxParams` struct + `FxType` enum

**Files:**
- Create: `Source/FX/FxParams.h`
- Modify: `CMakeLists.txt` (just to make sure the file is reachable; header-only so no source needed yet)

This task is header-only — no test of its own; the next tasks consume it. We split it out so the type system stabilizes before any DSP touches it.

- [ ] **Step 1: Create `Source/FX/FxParams.h`**

```cpp
#pragma once

namespace SynthVibe
{
    // Order MUST match the choice array in ParameterLayout.cpp's fx-slot
    // registration block. Persisted as a choice index — only ever append.
    enum class FxType : int
    {
        None      = 0,
        Drive     = 1,
        Chorus    = 2,
        Delay     = 3,
        Reverb    = 4,
        Eq3       = 5,
        Comp      = 6,    // stub in Phase 4
        Phaser    = 7,    // stub
        Flanger   = 8,    // stub
        Bitcrush  = 9,    // stub
        Filter    = 10,   // stub
    };

    inline constexpr int kFxTypeCount = 11;

    // One slot's worth of state, snapshotted from APVTS once per audio block.
    struct FxSlotParams
    {
        FxType type   = FxType::None;
        bool   bypass = false;
        float  mix    = 1.f;   // 0 = dry, 1 = wet
        float  p1     = 0.5f;
        float  p2     = 0.5f;
        float  p3     = 0.5f;
        float  p4     = 0.5f;
    };
}
```

- [ ] **Step 2: Sanity build**

```
./build-with-vs.bat
```

Expected: nothing changes (header is unused). Test target still builds.

- [ ] **Step 3: Commit**

```
git add Source/FX/FxParams.h
git commit -m "feat(fx): FxType enum + FxSlotParams struct"
```

---

## Task 4: `Eq3` — 3-band EQ DSP

**Files:**
- Create: `Source/FX/Eq3.h`
- Create: `Source/FX/Eq3.cpp`
- Create: `Tests/Eq3Tests.cpp`
- Modify: `CMakeLists.txt`

> If you flipped the open decision to **Comp**, replace this whole task with one that wraps `juce::dsp::Compressor` instead — same external interface (`prepare`/`setParams`/`process`/`reset`).

- [ ] **Step 1: Add `Tests/Eq3Tests.cpp`**

```cpp
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "FX/Eq3.h"
#include <cmath>

struct Eq3Tests : public juce::UnitTest
{
    Eq3Tests() : juce::UnitTest("Eq3", "AISynth") {}

    static void fillSine(juce::AudioBuffer<float>& buf, float freqHz, double sr)
    {
        const int n = buf.getNumSamples();
        for (int i = 0; i < n; ++i)
        {
            const float v = std::sin(2.0f * juce::MathConstants<float>::pi
                                     * freqHz * (float) i / (float) sr);
            for (int ch = 0; ch < buf.getNumChannels(); ++ch)
                buf.setSample(ch, i, v);
        }
    }

    static float rms(const juce::AudioBuffer<float>& buf)
    {
        const int n = buf.getNumSamples();
        double s = 0.0;
        const float* d = buf.getReadPointer(0);
        for (int i = 0; i < n; ++i) s += (double) d[i] * d[i];
        return (float) std::sqrt(s / (double) n);
    }

    void runTest() override
    {
        const double sr = 48000.0;
        const int blocks = 4;     // ramp up settled state
        const int blockN = 1024;

        beginTest("Eq3 with all gains 0 dB approximates passthrough");
        {
            Eq3 eq;
            eq.prepare(sr, blockN);
            Eq3::Params p;
            p.lowGainDb = 0.f;
            p.midGainDb = 0.f;
            p.highGainDb = 0.f;
            p.midFreq = 1000.f;
            eq.setParams(p);

            juce::AudioBuffer<float> buf(2, blockN);
            for (int b = 0; b < blocks; ++b)
            {
                fillSine(buf, 1000.f, sr);
                const float before = rms(buf);
                eq.process(buf);
                const float after = rms(buf);
                if (b == blocks - 1)  // only check last block (filters settled)
                    expectWithinAbsoluteError(after, before, 0.05f);
            }
        }

        beginTest("Eq3 with low +12 dB amplifies a 100 Hz sine");
        {
            Eq3 eq;
            eq.prepare(sr, blockN);
            Eq3::Params p;
            p.lowGainDb = 12.f;
            p.midGainDb = 0.f;
            p.highGainDb = 0.f;
            p.midFreq = 1000.f;
            eq.setParams(p);

            juce::AudioBuffer<float> buf(2, blockN);
            float lastBoost = 0.f;
            for (int b = 0; b < blocks; ++b)
            {
                fillSine(buf, 100.f, sr);
                const float before = rms(buf);
                eq.process(buf);
                const float after = rms(buf);
                lastBoost = after / juce::jmax(before, 1e-6f);
            }
            // +12 dB ≈ 4× linear; allow generous slack because shelf rolloff at 100 Hz
            // doesn't hit the full +12 — we just want to confirm boost direction.
            expect(lastBoost > 1.5f,
                   "+12 dB low shelf should boost 100 Hz sine by >1.5×, got "
                   + juce::String(lastBoost, 3));
        }
    }
};

static Eq3Tests sEq3Tests;
```

- [ ] **Step 2: Add to `CMakeLists.txt`**

In `target_sources(AISynth PRIVATE …)`, after `Source/FX/Reverb.cpp`, add:

```cmake
    Source/FX/Eq3.cpp
```

In `target_sources(AISynthTests PRIVATE …)`, after the `Source/Engine/SynthEngine.cpp` line, add:

```cmake
    Source/FX/Eq3.cpp
```

And add the test file to the test sources list (after `Tests/OscillatorTests.cpp`):

```cmake
    Tests/Eq3Tests.cpp
```

- [ ] **Step 3: Build and confirm test fails**

```
./build-with-vs.bat
```

Expected: compile error — `FX/Eq3.h` missing.

- [ ] **Step 4: Create `Source/FX/Eq3.h`**

```cpp
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

// 3-band EQ: low shelf @ 250 Hz · peak @ midFreq · high shelf @ 5000 Hz.
// Coefficients derived from the RBJ Audio EQ Cookbook. Each band is a
// stereo biquad processed in series (state per channel).
class Eq3
{
public:
    struct Params
    {
        float lowGainDb  = 0.f;     // -12..+12 dB
        float midGainDb  = 0.f;
        float highGainDb = 0.f;
        float midFreq    = 1000.f;  // 200..5000 Hz
    };

    void prepare(double sampleRate, int maxBlockSize);
    void setParams(const Params& p);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    struct Biquad
    {
        // direct-form-1 coefficients
        float b0 = 1.f, b1 = 0.f, b2 = 0.f;
        float a1 = 0.f, a2 = 0.f;     // a0 normalised to 1
        float x1[2] {}, x2[2] {};     // per channel
        float y1[2] {}, y2[2] {};

        void  reset() noexcept { x1[0]=x1[1]=x2[0]=x2[1]=0.f; y1[0]=y1[1]=y2[0]=y2[1]=0.f; }
        float process(float x, int ch) noexcept;
    };

    Biquad lowShelf, peak, highShelf;
    Params params;
    double sampleRate = 44100.0;

    static void designLowShelf (Biquad& b, double sr, float fc, float gainDb) noexcept;
    static void designPeak     (Biquad& b, double sr, float fc, float gainDb,
                                float Q = 0.7f) noexcept;
    static void designHighShelf(Biquad& b, double sr, float fc, float gainDb) noexcept;
};
```

- [ ] **Step 5: Create `Source/FX/Eq3.cpp`**

```cpp
#include "Eq3.h"
#include <cmath>

void Eq3::prepare(double sr, int /*maxBlockSize*/)
{
    sampleRate = sr;
    reset();
    setParams(params);   // re-design with current params
}

void Eq3::setParams(const Params& p)
{
    params = p;
    designLowShelf (lowShelf,  sampleRate, 250.f,
                    juce::jlimit(-12.f, 12.f, p.lowGainDb));
    designPeak     (peak,      sampleRate, juce::jlimit(20.f, 20000.f, p.midFreq),
                    juce::jlimit(-12.f, 12.f, p.midGainDb));
    designHighShelf(highShelf, sampleRate, 5000.f,
                    juce::jlimit(-12.f, 12.f, p.highGainDb));
}

void Eq3::reset()
{
    lowShelf.reset();
    peak.reset();
    highShelf.reset();
}

void Eq3::process(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
    const int numSamples  = buffer.getNumSamples();
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            float v = data[i];
            v = lowShelf.process (v, ch);
            v = peak.process     (v, ch);
            v = highShelf.process(v, ch);
            data[i] = v;
        }
    }
}

float Eq3::Biquad::process(float x, int ch) noexcept
{
    const float y = b0 * x + b1 * x1[ch] + b2 * x2[ch] - a1 * y1[ch] - a2 * y2[ch];
    x2[ch] = x1[ch]; x1[ch] = x;
    y2[ch] = y1[ch]; y1[ch] = y;
    return y;
}

// RBJ Audio EQ Cookbook — low shelf, slope S = 1 (Butterworth-ish).
void Eq3::designLowShelf(Biquad& b, double sr, float fc, float gainDb) noexcept
{
    const double A     = std::pow(10.0, gainDb / 40.0);
    const double w0    = 2.0 * juce::MathConstants<double>::pi * fc / sr;
    const double cosw0 = std::cos(w0);
    const double sinw0 = std::sin(w0);
    const double S     = 1.0;
    const double alpha = sinw0 / 2.0 * std::sqrt((A + 1.0/A) * (1.0/S - 1.0) + 2.0);
    const double sqrtA = std::sqrt(A);

    const double b0 =        A * ((A + 1) - (A - 1) * cosw0 + 2 * sqrtA * alpha);
    const double b1 =  2.0 * A * ((A - 1) - (A + 1) * cosw0);
    const double b2 =        A * ((A + 1) - (A - 1) * cosw0 - 2 * sqrtA * alpha);
    const double a0 =             (A + 1) + (A - 1) * cosw0 + 2 * sqrtA * alpha;
    const double a1 = -2.0      * ((A - 1) + (A + 1) * cosw0);
    const double a2 =             (A + 1) + (A - 1) * cosw0 - 2 * sqrtA * alpha;

    b.b0 = (float)(b0 / a0);
    b.b1 = (float)(b1 / a0);
    b.b2 = (float)(b2 / a0);
    b.a1 = (float)(a1 / a0);
    b.a2 = (float)(a2 / a0);
}

// RBJ Audio EQ Cookbook — peaking EQ.
void Eq3::designPeak(Biquad& b, double sr, float fc, float gainDb, float Q) noexcept
{
    const double A     = std::pow(10.0, gainDb / 40.0);
    const double w0    = 2.0 * juce::MathConstants<double>::pi * fc / sr;
    const double cosw0 = std::cos(w0);
    const double alpha = std::sin(w0) / (2.0 * Q);

    const double b0 = 1 + alpha * A;
    const double b1 = -2 * cosw0;
    const double b2 = 1 - alpha * A;
    const double a0 = 1 + alpha / A;
    const double a1 = -2 * cosw0;
    const double a2 = 1 - alpha / A;

    b.b0 = (float)(b0 / a0);
    b.b1 = (float)(b1 / a0);
    b.b2 = (float)(b2 / a0);
    b.a1 = (float)(a1 / a0);
    b.a2 = (float)(a2 / a0);
}

// RBJ Audio EQ Cookbook — high shelf, slope S = 1.
void Eq3::designHighShelf(Biquad& b, double sr, float fc, float gainDb) noexcept
{
    const double A     = std::pow(10.0, gainDb / 40.0);
    const double w0    = 2.0 * juce::MathConstants<double>::pi * fc / sr;
    const double cosw0 = std::cos(w0);
    const double sinw0 = std::sin(w0);
    const double S     = 1.0;
    const double alpha = sinw0 / 2.0 * std::sqrt((A + 1.0/A) * (1.0/S - 1.0) + 2.0);
    const double sqrtA = std::sqrt(A);

    const double b0 =        A * ((A + 1) + (A - 1) * cosw0 + 2 * sqrtA * alpha);
    const double b1 = -2.0 * A * ((A - 1) + (A + 1) * cosw0);
    const double b2 =        A * ((A + 1) + (A - 1) * cosw0 - 2 * sqrtA * alpha);
    const double a0 =             (A + 1) - (A - 1) * cosw0 + 2 * sqrtA * alpha;
    const double a1 =  2.0      * ((A - 1) - (A + 1) * cosw0);
    const double a2 =             (A + 1) - (A - 1) * cosw0 - 2 * sqrtA * alpha;

    b.b0 = (float)(b0 / a0);
    b.b1 = (float)(b1 / a0);
    b.b2 = (float)(b2 / a0);
    b.a1 = (float)(a1 / a0);
    b.a2 = (float)(a2 / a0);
}
```

- [ ] **Step 6: Build and run tests**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynth Tests.exe"
```

Expected: both `Eq3` tests pass (`with all gains 0 dB approximates passthrough`, `with low +12 dB amplifies a 100 Hz sine`).

- [ ] **Step 7: Commit**

```
git add Source/FX/Eq3.h Source/FX/Eq3.cpp Tests/Eq3Tests.cpp CMakeLists.txt
git commit -m "feat(fx): Eq3 3-band EQ via RBJ biquads"
```

---

## Task 5: `FxTypeMap` — per-type label + p1..p4 → struct mapping

**Files:**
- Create: `Source/FX/FxTypeMap.h`
- Create: `Source/FX/FxTypeMap.cpp`
- Modify: `CMakeLists.txt`

This module is the single source of truth for "what does p1 mean when type==Drive?" Both the audio engine (Task 6) and the UI knob labels (Task 10) read from it.

- [ ] **Step 1: Create `Source/FX/FxTypeMap.h`**

```cpp
#pragma once
#include <array>
#include "FxParams.h"
#include "Drive.h"
#include "Chorus.h"
#include "Delay.h"
#include "Reverb.h"
#include "Eq3.h"

namespace SynthVibe
{
    // Display name shown in the FxSlot type-picker dropdown.
    // Index = (int) FxType.  Keep aligned with ParameterLayout's typeLabels.
    extern const std::array<const char*, kFxTypeCount> kFxTypeNames;

    // Per-type knob labels for p1..p4. "" means "unused" — UI hides the knob.
    // [type][param 0..3]
    extern const std::array<std::array<const char*, 4>, kFxTypeCount> kFxParamLabels;

    // p1..p4 are all in [0..1]. These functions remap them onto the existing
    // effect-class Params struct.  Slot.mix is consumed separately (it's the
    // wet/dry blend already built into each effect class).
    Drive::Params  toDriveParams (const FxSlotParams& s) noexcept;
    Chorus::Params toChorusParams(const FxSlotParams& s) noexcept;
    Delay::Params  toDelayParams (const FxSlotParams& s) noexcept;
    Reverb::Params toReverbParams(const FxSlotParams& s) noexcept;
    Eq3::Params    toEq3Params   (const FxSlotParams& s) noexcept;
}
```

- [ ] **Step 2: Create `Source/FX/FxTypeMap.cpp`**

```cpp
#include "FxTypeMap.h"
#include <juce_core/juce_core.h>

namespace SynthVibe
{
    const std::array<const char*, kFxTypeCount> kFxTypeNames = {{
        "None", "Drive", "Chorus", "Delay", "Reverb",
        "EQ3", "Comp", "Phaser", "Flanger", "Bitcrush", "Filter"
    }};

    const std::array<std::array<const char*, 4>, kFxTypeCount> kFxParamLabels = {{
        /* None     */ { "", "", "", "" },
        /* Drive    */ { "Type", "Amount", "", "" },
        /* Chorus   */ { "Rate", "Depth", "", "" },
        /* Delay    */ { "Time", "Fbk",   "", "" },
        /* Reverb   */ { "Size", "Damp",  "", "" },
        /* EQ3      */ { "Low",  "Mid",   "High", "Mid Hz" },
        /* Comp     */ { "Thr",  "Ratio", "Atk",  "Rel" },     // stub
        /* Phaser   */ { "Rate", "Depth", "Fbk",  "" },        // stub
        /* Flanger  */ { "Rate", "Depth", "Fbk",  "" },        // stub
        /* Bitcrush */ { "Bits", "Rate",  "",     "" },        // stub
        /* Filter   */ { "Cutoff","Reso", "Type", "" }         // stub
    }};

    Drive::Params toDriveParams(const FxSlotParams& s) noexcept
    {
        Drive::Params p;
        // p1 in [0..1] → 3 type slots {Soft=0, Hard=1, Fold=2}
        p.type    = static_cast<Drive::Type>(juce::jlimit(0, 2,
                        (int) std::round(s.p1 * 2.f)));
        p.driveDb = s.p2 * 24.f;       // [0..1] → [0..24] dB
        p.mix     = s.mix;             // wet/dry handled by Drive itself
        return p;
    }

    Chorus::Params toChorusParams(const FxSlotParams& s) noexcept
    {
        Chorus::Params p;
        p.rate  = 0.1f + s.p1 * (5.0f  - 0.1f);          // [0.1..5] Hz
        p.depth = 0.001f + s.p2 * (0.015f - 0.001f);     // [0.001..0.015] s
        p.mix   = s.mix;
        return p;
    }

    Delay::Params toDelayParams(const FxSlotParams& s) noexcept
    {
        Delay::Params p;
        p.timeMs   = 1.f + s.p1 * (2000.f - 1.f);        // [1..2000] ms
        p.feedback = s.p2 * 0.95f;                        // [0..0.95]
        p.mix      = s.mix;
        return p;
    }

    Reverb::Params toReverbParams(const FxSlotParams& s) noexcept
    {
        Reverb::Params p;
        p.room = s.p1;                                    // [0..1]
        p.damp = s.p2;                                    // [0..1]
        p.mix  = s.mix;
        return p;
    }

    Eq3::Params toEq3Params(const FxSlotParams& s) noexcept
    {
        Eq3::Params p;
        p.lowGainDb  = (s.p1 - 0.5f) * 24.f;              // [-12..+12] dB
        p.midGainDb  = (s.p2 - 0.5f) * 24.f;
        p.highGainDb = (s.p3 - 0.5f) * 24.f;
        p.midFreq    = 200.f + s.p4 * (5000.f - 200.f);   // [200..5000] Hz
        return p;
    }
}
```

- [ ] **Step 3: Add `Source/FX/FxTypeMap.cpp` to `CMakeLists.txt`**

In both `target_sources(AISynth PRIVATE …)` and `target_sources(AISynthTests PRIVATE …)`, add after `Source/FX/Eq3.cpp`:

```cmake
    Source/FX/FxTypeMap.cpp
```

- [ ] **Step 4: Build**

```
./build-with-vs.bat
```

Expected: test target builds. Plugin target still fails (still references removed legacy IDs).

- [ ] **Step 5: Commit**

```
git add Source/FX/FxTypeMap.h Source/FX/FxTypeMap.cpp CMakeLists.txt
git commit -m "feat(fx): FxTypeMap labels + p1..p4 → effect-params mapping"
```

---

## Task 6: `FxSlot` — owns one of each effect, dispatches by type

**Files:**
- Create: `Source/FX/FxSlot.h`
- Create: `Source/FX/FxSlot.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Create `Source/FX/FxSlot.h`**

```cpp
#pragma once
#include "FxParams.h"
#include "Drive.h"
#include "Chorus.h"
#include "Delay.h"
#include "Reverb.h"
#include "Eq3.h"

namespace SynthVibe
{
    // Owns one instance of every concrete effect class so type changes never
    // allocate on the audio thread. Memory footprint: ~700 KB at 44.1 kHz
    // (the Delay buffer dominates), × 10 slots in the runner.
    class FxSlot
    {
    public:
        void prepare(double sampleRate, int maxBlockSize);
        void setParams(const FxSlotParams& p);
        void process(juce::AudioBuffer<float>& buffer);
        void reset();

    private:
        FxType   activeType = FxType::None;
        bool     bypass     = false;

        Drive  drive;
        Chorus chorus;
        Delay  delay;
        Reverb reverb;
        Eq3    eq3;
    };
}
```

- [ ] **Step 2: Create `Source/FX/FxSlot.cpp`**

```cpp
#include "FxSlot.h"
#include "FxTypeMap.h"

namespace SynthVibe
{
    void FxSlot::prepare(double sr, int maxBlock)
    {
        drive .prepare(sr, maxBlock);
        chorus.prepare(sr, maxBlock);
        delay .prepare(sr, maxBlock);
        reverb.prepare(sr, maxBlock);
        eq3   .prepare(sr, maxBlock);
        activeType = FxType::None;
        bypass     = false;
    }

    void FxSlot::setParams(const FxSlotParams& p)
    {
        // On a type change, reset the newly-active effect so its internal
        // state (delay buffers, biquad memory) starts clean rather than
        // leaking the previous effect's residue.
        if (p.type != activeType)
        {
            activeType = p.type;
            switch (activeType)
            {
                case FxType::Drive:  drive.reset();  break;
                case FxType::Chorus: chorus.reset(); break;
                case FxType::Delay:  delay.reset();  break;
                case FxType::Reverb: reverb.reset(); break;
                case FxType::Eq3:    eq3.reset();    break;
                default: break;
            }
        }
        bypass = p.bypass;

        switch (activeType)
        {
            case FxType::Drive:  drive .setParams(toDriveParams (p)); break;
            case FxType::Chorus: chorus.setParams(toChorusParams(p)); break;
            case FxType::Delay:  delay .setParams(toDelayParams (p)); break;
            case FxType::Reverb: reverb.setParams(toReverbParams(p)); break;
            case FxType::Eq3:    eq3   .setParams(toEq3Params   (p)); break;
            default: break;       // None + stubs: nothing to set
        }
    }

    void FxSlot::process(juce::AudioBuffer<float>& buffer)
    {
        if (bypass) return;
        switch (activeType)
        {
            case FxType::Drive:  drive .process(buffer); break;
            case FxType::Chorus: chorus.process(buffer); break;
            case FxType::Delay:  delay .process(buffer); break;
            case FxType::Reverb: reverb.process(buffer); break;
            case FxType::Eq3:    eq3   .process(buffer); break;
            default: break;       // None + stubs: passthrough
        }
    }

    void FxSlot::reset()
    {
        drive.reset();
        chorus.reset();
        delay.reset();
        reverb.reset();
        eq3.reset();
    }
}
```

- [ ] **Step 3: Add to `CMakeLists.txt`**

In both `target_sources(AISynth PRIVATE …)` and `target_sources(AISynthTests PRIVATE …)`, after `Source/FX/FxTypeMap.cpp`, add:

```cmake
    Source/FX/FxSlot.cpp
```

- [ ] **Step 4: Build**

```
./build-with-vs.bat
```

Expected: test target builds. No new tests yet — `FxSlot` is exercised through `FxRunnerTests` in Task 7.

- [ ] **Step 5: Commit**

```
git add Source/FX/FxSlot.h Source/FX/FxSlot.cpp CMakeLists.txt
git commit -m "feat(fx): FxSlot dispatches process() by stored FxType"
```

---

## Task 7: `FxRunner` — 10 slots, replaces FXChain

**Files:**
- Create: `Source/FX/FxRunner.h`
- Create: `Source/FX/FxRunner.cpp`
- Create: `Tests/FxRunnerTests.cpp`
- Modify: `CMakeLists.txt`
- Delete: `Source/FX/FXChain.h`, `Source/FX/FXChain.cpp`

- [ ] **Step 1: Create `Tests/FxRunnerTests.cpp`**

```cpp
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "FX/FxRunner.h"

struct FxRunnerTests : public juce::UnitTest
{
    FxRunnerTests() : juce::UnitTest("FxRunner", "AISynth") {}

    static void fillSine(juce::AudioBuffer<float>& buf, float freqHz, double sr)
    {
        const int n = buf.getNumSamples();
        for (int i = 0; i < n; ++i)
        {
            const float v = std::sin(2.0f * juce::MathConstants<float>::pi
                                     * freqHz * (float) i / (float) sr);
            for (int ch = 0; ch < buf.getNumChannels(); ++ch)
                buf.setSample(ch, i, v);
        }
    }

    static float maxAbs(const juce::AudioBuffer<float>& buf)
    {
        float m = 0.f;
        const int n = buf.getNumSamples();
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        {
            const float* d = buf.getReadPointer(ch);
            for (int i = 0; i < n; ++i) m = juce::jmax(m, std::abs(d[i]));
        }
        return m;
    }

    void runTest() override
    {
        const double sr = 48000.0;
        const int blockN = 512;

        beginTest("Empty chain (all None) is exact passthrough");
        {
            SynthVibe::FxRunner runner;
            runner.prepare(sr, blockN);
            std::array<SynthVibe::FxSlotParams, 10> snap{};   // all type=None, mix=1
            runner.setSnapshot(snap);

            juce::AudioBuffer<float> buf(2, blockN);
            fillSine(buf, 1000.f, sr);
            const float before = maxAbs(buf);
            runner.process(buf);
            const float after = maxAbs(buf);
            expectWithinAbsoluteError(after, before, 0.0001f);
        }

        beginTest("All-bypassed chain is passthrough even with non-None types");
        {
            SynthVibe::FxRunner runner;
            runner.prepare(sr, blockN);
            std::array<SynthVibe::FxSlotParams, 10> snap{};
            for (auto& s : snap)
            {
                s.type   = SynthVibe::FxType::Drive;
                s.bypass = true;
                s.p1     = 1.f;   // would be Fold @ max amount otherwise
                s.p2     = 1.f;
            }
            runner.setSnapshot(snap);

            juce::AudioBuffer<float> buf(2, blockN);
            fillSine(buf, 1000.f, sr);
            const float before = maxAbs(buf);
            runner.process(buf);
            const float after = maxAbs(buf);
            expectWithinAbsoluteError(after, before, 0.0001f);
        }

        beginTest("Stub types (Phaser/Flanger/Bitcrush/Filter/Comp) are passthrough");
        {
            SynthVibe::FxRunner runner;
            runner.prepare(sr, blockN);
            std::array<SynthVibe::FxSlotParams, 10> snap{};
            const SynthVibe::FxType stubs[] = {
                SynthVibe::FxType::Comp,    SynthVibe::FxType::Phaser,
                SynthVibe::FxType::Flanger, SynthVibe::FxType::Bitcrush,
                SynthVibe::FxType::Filter
            };
            for (size_t i = 0; i < std::size(stubs); ++i)
                snap[i].type = stubs[i];
            runner.setSnapshot(snap);

            juce::AudioBuffer<float> buf(2, blockN);
            fillSine(buf, 1000.f, sr);
            const float before = maxAbs(buf);
            runner.process(buf);
            const float after = maxAbs(buf);
            expectWithinAbsoluteError(after, before, 0.0001f);
        }
    }
};

static FxRunnerTests sFxRunnerTests;
```

- [ ] **Step 2: Add test file to `CMakeLists.txt`**

In `target_sources(AISynthTests PRIVATE …)`, after `Tests/Eq3Tests.cpp`:

```cmake
    Tests/FxRunnerTests.cpp
```

- [ ] **Step 3: Build, confirm fail**

```
./build-with-vs.bat
```

Expected: compile error — `FX/FxRunner.h` missing.

- [ ] **Step 4: Create `Source/FX/FxRunner.h`**

```cpp
#pragma once
#include <array>
#include "FxSlot.h"

namespace SynthVibe
{
    // 10 serial slots driven by per-block snapshots from APVTS.
    class FxRunner
    {
    public:
        static constexpr int kNumSlots = 10;

        void prepare(double sampleRate, int maxBlockSize);
        void setSnapshot(const std::array<FxSlotParams, kNumSlots>& snap);
        void process(juce::AudioBuffer<float>& buffer);
        void reset();

    private:
        std::array<FxSlot, kNumSlots> slots;
    };
}
```

- [ ] **Step 5: Create `Source/FX/FxRunner.cpp`**

```cpp
#include "FxRunner.h"

namespace SynthVibe
{
    void FxRunner::prepare(double sr, int maxBlock)
    {
        for (auto& s : slots) s.prepare(sr, maxBlock);
    }

    void FxRunner::setSnapshot(const std::array<FxSlotParams, kNumSlots>& snap)
    {
        for (int i = 0; i < kNumSlots; ++i) slots[i].setParams(snap[i]);
    }

    void FxRunner::process(juce::AudioBuffer<float>& buffer)
    {
        for (auto& s : slots) s.process(buffer);
    }

    void FxRunner::reset()
    {
        for (auto& s : slots) s.reset();
    }
}
```

- [ ] **Step 6: Add to `CMakeLists.txt`**

In both `target_sources(AISynth PRIVATE …)` and `target_sources(AISynthTests PRIVATE …)`, after `Source/FX/FxSlot.cpp`:

```cmake
    Source/FX/FxRunner.cpp
```

And in `target_sources(AISynth PRIVATE …)`, **remove** this existing line (FxRunner replaces it):

```cmake
    Source/FX/FXChain.cpp
```

- [ ] **Step 7: Build and run tests**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynth Tests.exe"
```

Expected: all three FxRunner tests pass. The plugin target still fails to build (Task 8 fixes that).

- [ ] **Step 8: Delete the old FXChain files**

```
git rm Source/FX/FXChain.h Source/FX/FXChain.cpp
```

- [ ] **Step 9: Commit**

```
git add Source/FX/FxRunner.h Source/FX/FxRunner.cpp Tests/FxRunnerTests.cpp CMakeLists.txt
git commit -m "feat(fx): FxRunner walks 10 slots; supersedes FXChain"
```

---

## Task 8: Wire `FxRunner` into `PluginProcessor`

**Files:**
- Modify: `Source/PluginProcessor.h`
- Modify: `Source/PluginProcessor.cpp`

This is the moment the plugin target builds again.

- [ ] **Step 1: Edit `Source/PluginProcessor.h`**

Replace `#include "FX/FXChain.h"` with:

```cpp
#include "FX/FxRunner.h"
```

In the private section, replace the `FXChain fxChain;` field with:

```cpp
    SynthVibe::FxRunner fxRunner;
```

Replace the four method declarations:

```cpp
    Delay::Params     buildDelayParams()  const;
    Chorus::Params    buildChorusParams() const;
    Drive::Params     buildDriveParams()  const;
    Reverb::Params    buildReverbParams() const;
```

with this single one:

```cpp
    std::array<SynthVibe::FxSlotParams, SynthVibe::FxRunner::kNumSlots>
        buildFxSnapshot() const;
```

You'll need `#include <array>` near the top of the file if not already present.

- [ ] **Step 2: Edit `Source/PluginProcessor.cpp`**

**(a)** Replace the `prepareToPlay` body's `fxChain.prepare(...)` line:

```cpp
    fxChain.prepare(sampleRate, samplesPerBlock);
```

with:

```cpp
    fxRunner.prepare(sampleRate, samplesPerBlock);
```

**(b)** Replace the FX dispatch lines in `processBlock`:

```cpp
    // FX chain en premier, PUIS master volume (post-fader correct)
    fxChain.setParams(buildDelayParams(), buildChorusParams(), buildDriveParams(), buildReverbParams());
    fxChain.process(buffer);
```

with:

```cpp
    // FX runner first, THEN master volume (post-fader)
    fxRunner.setSnapshot(buildFxSnapshot());
    fxRunner.process(buffer);
```

**(c)** Delete the four `buildDelayParams`/`buildChorusParams`/`buildDriveParams`/`buildReverbParams` method bodies (lines 151–185 in the current file).

**(d)** Add the new `buildFxSnapshot` after `buildArpParams`:

```cpp
std::array<SynthVibe::FxSlotParams, SynthVibe::FxRunner::kNumSlots>
AISynthProcessor::buildFxSnapshot() const
{
    using SynthVibe::FxSlotParams;
    using SynthVibe::FxType;

    struct SlotIds { const char* type; const char* bypass; const char* mix;
                     const char* p1; const char* p2; const char* p3; const char* p4; };
    static constexpr SlotIds slots[10] = {
        { ParamIDs::fx1Type,  ParamIDs::fx1Bypass,  ParamIDs::fx1Mix,
          ParamIDs::fx1P1,    ParamIDs::fx1P2,      ParamIDs::fx1P3,    ParamIDs::fx1P4  },
        { ParamIDs::fx2Type,  ParamIDs::fx2Bypass,  ParamIDs::fx2Mix,
          ParamIDs::fx2P1,    ParamIDs::fx2P2,      ParamIDs::fx2P3,    ParamIDs::fx2P4  },
        { ParamIDs::fx3Type,  ParamIDs::fx3Bypass,  ParamIDs::fx3Mix,
          ParamIDs::fx3P1,    ParamIDs::fx3P2,      ParamIDs::fx3P3,    ParamIDs::fx3P4  },
        { ParamIDs::fx4Type,  ParamIDs::fx4Bypass,  ParamIDs::fx4Mix,
          ParamIDs::fx4P1,    ParamIDs::fx4P2,      ParamIDs::fx4P3,    ParamIDs::fx4P4  },
        { ParamIDs::fx5Type,  ParamIDs::fx5Bypass,  ParamIDs::fx5Mix,
          ParamIDs::fx5P1,    ParamIDs::fx5P2,      ParamIDs::fx5P3,    ParamIDs::fx5P4  },
        { ParamIDs::fx6Type,  ParamIDs::fx6Bypass,  ParamIDs::fx6Mix,
          ParamIDs::fx6P1,    ParamIDs::fx6P2,      ParamIDs::fx6P3,    ParamIDs::fx6P4  },
        { ParamIDs::fx7Type,  ParamIDs::fx7Bypass,  ParamIDs::fx7Mix,
          ParamIDs::fx7P1,    ParamIDs::fx7P2,      ParamIDs::fx7P3,    ParamIDs::fx7P4  },
        { ParamIDs::fx8Type,  ParamIDs::fx8Bypass,  ParamIDs::fx8Mix,
          ParamIDs::fx8P1,    ParamIDs::fx8P2,      ParamIDs::fx8P3,    ParamIDs::fx8P4  },
        { ParamIDs::fx9Type,  ParamIDs::fx9Bypass,  ParamIDs::fx9Mix,
          ParamIDs::fx9P1,    ParamIDs::fx9P2,      ParamIDs::fx9P3,    ParamIDs::fx9P4  },
        { ParamIDs::fx10Type, ParamIDs::fx10Bypass, ParamIDs::fx10Mix,
          ParamIDs::fx10P1,   ParamIDs::fx10P2,     ParamIDs::fx10P3,   ParamIDs::fx10P4 },
    };

    std::array<FxSlotParams, SynthVibe::FxRunner::kNumSlots> out;
    for (int i = 0; i < SynthVibe::FxRunner::kNumSlots; ++i)
    {
        const auto& s = slots[i];
        FxSlotParams& p = out[i];
        p.type   = static_cast<FxType>(static_cast<int>(*apvts.getRawParameterValue(s.type)));
        p.bypass = *apvts.getRawParameterValue(s.bypass) > 0.5f;
        p.mix    = *apvts.getRawParameterValue(s.mix);
        p.p1     = *apvts.getRawParameterValue(s.p1);
        p.p2     = *apvts.getRawParameterValue(s.p2);
        p.p3     = *apvts.getRawParameterValue(s.p3);
        p.p4     = *apvts.getRawParameterValue(s.p4);
    }
    return out;
}
```

- [ ] **Step 3: Build the plugin target — should succeed for the first time since Task 1**

```
./build-with-vs.bat
```

Expected: `AISynth.vst3` builds successfully. **Note:** at this point `Source/UI/FXTab.h` still references the deleted legacy `ParamIDs::delayTime` etc. The build can fail there. If so, jump to Task 12 first (rewriting FxTab) and revisit Task 8 only after verifying the runner wiring compiles in isolation. Since FxTab is included via PluginEditor, the most pragmatic order is: Task 12 immediately after Task 8.

If you hit the FxTab error: do Task 12, then return here for Step 4.

- [ ] **Step 4: Run tests**

```
"./build/AISynthTests_artefacts/Release/AISynth Tests.exe"
```

Expected: every test still passes. No regressions. (Tests don't include `FXTab.h`, so they should build and pass even if Task 12 hasn't happened yet.)

- [ ] **Step 5: Commit**

```
git add Source/PluginProcessor.h Source/PluginProcessor.cpp
git commit -m "feat(processor): drive FxRunner from per-block APVTS snapshot"
```

---

## Task 9: `FxSlotTypePicker` UI component

**Files:**
- Create: `Source/UI/components/FxSlotTypePicker.h`
- Modify: `Tests/UIConstructionTests.cpp`

- [ ] **Step 1: Write the failing test**

Add to the `#include` block at the top of `Tests/UIConstructionTests.cpp`:

```cpp
#include "UI/components/FxSlotTypePicker.h"
```

Append a new test block in `runTest()` after the existing `ModTab` block:

```cpp
        beginTest("FxSlotTypePicker binds and lists 11 types");
        {
            SynthVibe::FxSlotTypePicker picker(apvts, ParamIDs::fx1Type);
            picker.setBounds(0, 0, 110, 24);
            // Default fx.1.type index = 0 ("None"); ComboBox IDs are 1-based → 1.
            expectEquals(picker.getCombo().getSelectedId(), 1);
            expectEquals(picker.getCombo().getNumItems(), 11);

            // Round-trip
            auto* p = apvts.getParameter(ParamIDs::fx1Type);
            p->setValueNotifyingHost(p->convertTo0to1(3.f));   // index 3 = "Delay"
            expectEquals(picker.getCombo().getSelectedId(), 4);
            p->setValueNotifyingHost(p->convertTo0to1(0.f));   // restore
        }
```

- [ ] **Step 2: Build, confirm fail**

```
./build-with-vs.bat
```

Expected: compile error — `FxSlotTypePicker.h` missing.

- [ ] **Step 3: Create `Source/UI/components/FxSlotTypePicker.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"
#include "../../FX/FxTypeMap.h"

namespace SynthVibe
{
    class FxSlotTypePicker : public juce::Component
    {
    public:
        FxSlotTypePicker(juce::AudioProcessorValueTreeState& apvts,
                         const juce::String& paramID)
        {
            for (int i = 0; i < kFxTypeCount; ++i)
                combo.addItem(kFxTypeNames[(size_t) i], i + 1);

            attach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                apvts, paramID, combo);

            combo.setColour(juce::ComboBox::backgroundColourId, Tokens::panel2);
            combo.setColour(juce::ComboBox::outlineColourId,    Tokens::edge);
            combo.setColour(juce::ComboBox::textColourId,       Tokens::ink);
            combo.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(combo);
        }

        juce::ComboBox& getCombo() noexcept { return combo; }

        void resized() override { combo.setBounds(getLocalBounds()); }

    private:
        juce::ComboBox combo;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attach;
    };
}
```

- [ ] **Step 4: Build and run tests**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynth Tests.exe"
```

Expected: `FxSlotTypePicker binds and lists 11 types` passes.

- [ ] **Step 5: Commit**

```
git add Source/UI/components/FxSlotTypePicker.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): FxSlotTypePicker dropdown of 11 FX types"
```

---

## Task 10: `FxSlotCard` UI component

**Files:**
- Create: `Source/UI/components/FxSlotCard.h`
- Modify: `Tests/UIConstructionTests.cpp`

A single slot's UI: header (slot # · type combo · bypass dot · mix knob) and a row of 4 generic param knobs labeled by current type.

- [ ] **Step 1: Write the failing test**

Add to includes:

```cpp
#include "UI/components/FxSlotCard.h"
```

Append a new test block:

```cpp
        beginTest("FxSlotCard constructs and binds slot 1");
        {
            SynthVibe::FxSlotCard card(apvts,
                ParamIDs::fx1Type, ParamIDs::fx1Bypass, ParamIDs::fx1Mix,
                ParamIDs::fx1P1,   ParamIDs::fx1P2,    ParamIDs::fx1P3, ParamIDs::fx1P4,
                /*slotIndex=*/1);
            card.setBounds(0, 0, 220, 180);
            expect(card.getWidth() == 220);
            expectEquals(card.getSlotIndex(), 1);
        }
```

- [ ] **Step 2: Build, confirm fail**

```
./build-with-vs.bat
```

Expected: compile error — `FxSlotCard.h` missing.

- [ ] **Step 3: Create `Source/UI/components/FxSlotCard.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "ArcKnob.h"
#include "FxSlotTypePicker.h"
#include "../DesignTokens.h"
#include "../Fonts.h"
#include "../../FX/FxTypeMap.h"

namespace SynthVibe
{
    // One FX slot: a small card showing slot number, type-picker, bypass dot,
    // mix knob, and 4 generic p1..p4 knobs. The knob LABELS update when the
    // type changes (subscribed via ParameterAttachment); knobs whose label
    // is empty (e.g. p3 of Drive) are hidden.
    class FxSlotCard : public juce::Component
    {
    public:
        FxSlotCard(juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& typeId,
                   const juce::String& bypassId,
                   const juce::String& mixId,
                   const juce::String& p1Id,
                   const juce::String& p2Id,
                   const juce::String& p3Id,
                   const juce::String& p4Id,
                   int slotIndex)
            : slotIdx(slotIndex),
              typePicker(apvts, typeId),
              mixKnob ("Mix", apvts, mixId, Tokens::fx, "", 2),
              p1Knob  ("",    apvts, p1Id,  Tokens::fx, "", 2),
              p2Knob  ("",    apvts, p2Id,  Tokens::fx, "", 2),
              p3Knob  ("",    apvts, p3Id,  Tokens::fx, "", 2),
              p4Knob  ("",    apvts, p4Id,  Tokens::fx, "", 2)
        {
            addAndMakeVisible(typePicker);
            addAndMakeVisible(mixKnob);
            addAndMakeVisible(p1Knob);
            addAndMakeVisible(p2Knob);
            addAndMakeVisible(p3Knob);
            addAndMakeVisible(p4Knob);

            // Bypass: a dot toggle in the card header
            bypassButton.setClickingTogglesState(true);
            bypassButton.setButtonText("");
            bypassAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                apvts, bypassId, bypassButton);
            addAndMakeVisible(bypassButton);

            // Watch the type param: relabel the knobs (and hide empty ones).
            typeAttach = std::make_unique<juce::ParameterAttachment>(
                *apvts.getParameter(typeId),
                [this](float v) {
                    const int idx = juce::jlimit(0, kFxTypeCount - 1,
                                                 (int) std::round(v));
                    applyTypeLabels(idx);
                });
            typeAttach->sendInitialUpdate();
        }

        int  getSlotIndex() const noexcept { return slotIdx; }

        void paint(juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat();
            g.setColour(Tokens::panel2);
            g.fillRoundedRectangle(b, Tokens::radiusMd);
            g.setColour(Tokens::edge);
            g.drawRoundedRectangle(b.reduced(0.5f), Tokens::radiusMd, 1.f);

            // Slot index badge in the header
            g.setColour(Tokens::ink3);
            g.setFont(juce::Font(Tokens::Font::tiny));
            g.drawText(juce::String(slotIdx),
                       getLocalBounds().withSize(18, 18).reduced(2),
                       juce::Justification::centred);
        }

        void resized() override
        {
            using namespace Tokens;
            auto area = getLocalBounds().reduced(spaceSm);

            // Top header strip: badge | type combo | bypass dot
            auto header = area.removeFromTop(24);
            header.removeFromLeft(18);                                      // badge
            const int bypassW = 16;
            bypassButton.setBounds(header.removeFromRight(bypassW)
                                         .withSizeKeepingCentre(12, 12));
            header.removeFromRight(spaceXs);
            typePicker.setBounds(header.reduced(0, 0));

            area.removeFromTop(spaceSm);

            // Mix on its own row
            mixKnob.setBounds(area.removeFromTop(48));
            area.removeFromTop(spaceXs);

            // 4 p-knobs in a 2x2 grid
            const int half = area.getHeight() / 2;
            auto rowTop    = area.removeFromTop(half);
            auto rowBottom = area;
            const int kw = rowTop.getWidth() / 2;
            p1Knob.setBounds(rowTop   .removeFromLeft(kw));
            p2Knob.setBounds(rowTop);
            p3Knob.setBounds(rowBottom.removeFromLeft(kw));
            p4Knob.setBounds(rowBottom);
        }

    private:
        void applyTypeLabels(int typeIdx)
        {
            const auto& labels = kFxParamLabels[(size_t) typeIdx];
            auto applyOne = [](ArcKnob& k, const char* lbl)
            {
                const juce::String s(lbl);
                k.setLabelText(s);
                k.setVisible(s.isNotEmpty());
            };
            applyOne(p1Knob, labels[0]);
            applyOne(p2Knob, labels[1]);
            applyOne(p3Knob, labels[2]);
            applyOne(p4Knob, labels[3]);
            // Mix is always visible; type=None just makes the slot a no-op,
            // but the user can still pre-set the mix for later activation.
        }

        int slotIdx;
        FxSlotTypePicker typePicker;
        juce::ToggleButton bypassButton;
        ArcKnob mixKnob, p1Knob, p2Knob, p3Knob, p4Knob;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttach;
        std::unique_ptr<juce::ParameterAttachment> typeAttach;
    };
}
```

- [ ] **Step 4: Confirm `ArcKnob::setLabelText` exists; if not, add it**

```
./build-with-vs.bat
```

If the build fails inside `applyTypeLabels` because `ArcKnob` lacks `setLabelText`, open `Source/UI/components/ArcKnob.h` and add a public method:

```cpp
void setLabelText(const juce::String& s) { label.setText(s, juce::dontSendNotification); }
```

(Where `label` is the existing `juce::Label` member of `ArcKnob`. If the member is named differently — e.g. `nameLabel` — adjust accordingly.) Commit this micro-change as part of this task's commit.

- [ ] **Step 5: Build and run tests**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynth Tests.exe"
```

Expected: `FxSlotCard constructs and binds slot 1` passes.

- [ ] **Step 6: Commit**

```
git add Source/UI/components/FxSlotCard.h Source/UI/components/ArcKnob.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): FxSlotCard with type-picker, bypass, mix, 4 generic knobs"
```

---

## Task 11: `FxChainStrip` UI component

**Files:**
- Create: `Source/UI/components/FxChainStrip.h`
- Modify: `Tests/UIConstructionTests.cpp`

A 2-row × 5-column grid of `FxSlotCard` instances, one per slot.

- [ ] **Step 1: Write the failing test**

Add to includes:

```cpp
#include "UI/components/FxChainStrip.h"
```

Append a new test block:

```cpp
        beginTest("FxChainStrip renders 10 slots");
        {
            SynthVibe::FxChainStrip strip(apvts);
            strip.setBounds(0, 0, 1200, 380);
            expectEquals(strip.getNumSlots(), 10);
        }
```

- [ ] **Step 2: Build, confirm fail**

```
./build-with-vs.bat
```

Expected: compile error — `FxChainStrip.h` missing.

- [ ] **Step 3: Create `Source/UI/components/FxChainStrip.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "FxSlotCard.h"
#include "../DesignTokens.h"
#include "../../Parameters/ParameterIDs.h"

namespace SynthVibe
{
    class FxChainStrip : public juce::Component
    {
    public:
        static constexpr int kNumSlots = 10;

        explicit FxChainStrip(juce::AudioProcessorValueTreeState& apvts)
        {
            struct SlotIds { const char* type; const char* bypass; const char* mix;
                             const char* p1; const char* p2; const char* p3; const char* p4; };
            const SlotIds slots[kNumSlots] = {
                { ParamIDs::fx1Type,  ParamIDs::fx1Bypass,  ParamIDs::fx1Mix,
                  ParamIDs::fx1P1,    ParamIDs::fx1P2,      ParamIDs::fx1P3,    ParamIDs::fx1P4  },
                { ParamIDs::fx2Type,  ParamIDs::fx2Bypass,  ParamIDs::fx2Mix,
                  ParamIDs::fx2P1,    ParamIDs::fx2P2,      ParamIDs::fx2P3,    ParamIDs::fx2P4  },
                { ParamIDs::fx3Type,  ParamIDs::fx3Bypass,  ParamIDs::fx3Mix,
                  ParamIDs::fx3P1,    ParamIDs::fx3P2,      ParamIDs::fx3P3,    ParamIDs::fx3P4  },
                { ParamIDs::fx4Type,  ParamIDs::fx4Bypass,  ParamIDs::fx4Mix,
                  ParamIDs::fx4P1,    ParamIDs::fx4P2,      ParamIDs::fx4P3,    ParamIDs::fx4P4  },
                { ParamIDs::fx5Type,  ParamIDs::fx5Bypass,  ParamIDs::fx5Mix,
                  ParamIDs::fx5P1,    ParamIDs::fx5P2,      ParamIDs::fx5P3,    ParamIDs::fx5P4  },
                { ParamIDs::fx6Type,  ParamIDs::fx6Bypass,  ParamIDs::fx6Mix,
                  ParamIDs::fx6P1,    ParamIDs::fx6P2,      ParamIDs::fx6P3,    ParamIDs::fx6P4  },
                { ParamIDs::fx7Type,  ParamIDs::fx7Bypass,  ParamIDs::fx7Mix,
                  ParamIDs::fx7P1,    ParamIDs::fx7P2,      ParamIDs::fx7P3,    ParamIDs::fx7P4  },
                { ParamIDs::fx8Type,  ParamIDs::fx8Bypass,  ParamIDs::fx8Mix,
                  ParamIDs::fx8P1,    ParamIDs::fx8P2,      ParamIDs::fx8P3,    ParamIDs::fx8P4  },
                { ParamIDs::fx9Type,  ParamIDs::fx9Bypass,  ParamIDs::fx9Mix,
                  ParamIDs::fx9P1,    ParamIDs::fx9P2,      ParamIDs::fx9P3,    ParamIDs::fx9P4  },
                { ParamIDs::fx10Type, ParamIDs::fx10Bypass, ParamIDs::fx10Mix,
                  ParamIDs::fx10P1,   ParamIDs::fx10P2,     ParamIDs::fx10P3,   ParamIDs::fx10P4 },
            };

            for (int i = 0; i < kNumSlots; ++i)
            {
                const auto& s = slots[i];
                cards[i] = std::make_unique<FxSlotCard>(apvts,
                    s.type, s.bypass, s.mix,
                    s.p1, s.p2, s.p3, s.p4,
                    i + 1);
                addAndMakeVisible(*cards[i]);
            }
        }

        int getNumSlots() const noexcept { return kNumSlots; }

        void resized() override
        {
            using namespace Tokens;
            // 2 rows × 5 cols. Cards are flexible; gaps are spaceSm.
            auto area = getLocalBounds().reduced(spaceMd);
            const int rowH = (area.getHeight() - spaceSm) / 2;

            auto layoutRow = [&](juce::Rectangle<int> row, int firstSlot)
            {
                const int colW = (row.getWidth() - 4 * spaceSm) / 5;
                for (int c = 0; c < 5; ++c)
                {
                    auto cell = row.removeFromLeft(colW);
                    cards[firstSlot + c]->setBounds(cell);
                    if (c < 4) row.removeFromLeft(spaceSm);
                }
            };

            layoutRow(area.removeFromTop(rowH), 0);
            area.removeFromTop(spaceSm);
            layoutRow(area, 5);
        }

    private:
        std::array<std::unique_ptr<FxSlotCard>, kNumSlots> cards;
    };
}
```

- [ ] **Step 4: Build and run tests**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynth Tests.exe"
```

Expected: `FxChainStrip renders 10 slots` passes.

- [ ] **Step 5: Commit**

```
git add Source/UI/components/FxChainStrip.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): FxChainStrip 2x5 grid of FxSlotCards"
```

---

## Task 12: Rewrite `FxTab` around `FxChainStrip`

**Files:**
- Modify: `Source/UI/FXTab.h`
- Modify: `Tests/UIConstructionTests.cpp`

- [ ] **Step 1: Write the failing test**

Append a new test block to `runTest()` after the `FxChainStrip` block:

```cpp
        beginTest("FxTab constructs with chain strip attached");
        {
            FxTab tab(apvts);
            tab.setBounds(0, 0, 1280, 560);
            expectEquals(tab.getWidth(), 1280);
            expect(tab.getStrip() != nullptr, "FxTab should own a FxChainStrip");
            expectEquals(tab.getStrip()->getNumSlots(), 10);
        }
```

- [ ] **Step 2: Build, confirm fail**

```
./build-with-vs.bat
```

Expected: compile error — old `FxTab` references removed `ParamIDs::delayTime` (and friends).

- [ ] **Step 3: Replace the entire contents of `Source/UI/FXTab.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DesignTokens.h"
#include "components/FxChainStrip.h"
#include "components/PanelHeader.h"

class FxTab : public juce::Component
{
public:
    explicit FxTab(juce::AudioProcessorValueTreeState& apvts) : apvts(apvts)
    {
        strip = std::make_unique<SynthVibe::FxChainStrip>(apvts);
        addAndMakeVisible(*strip);
    }

    SynthVibe::FxChainStrip* getStrip() noexcept { return strip.get(); }

    void paint(juce::Graphics& g) override
    {
        using namespace SynthVibe::Tokens;
        auto b = getLocalBounds().toFloat().reduced(2.f);
        g.setColour(panel);
        g.fillRoundedRectangle(b, radiusLg);
        g.setColour(edge);
        g.drawRoundedRectangle(b.reduced(0.5f), radiusLg, 1.f);
    }

    void resized() override
    {
        using namespace SynthVibe::Tokens;
        auto area = getLocalBounds().reduced(spaceMd);
        if (strip != nullptr)
            strip->setBounds(area);
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::unique_ptr<SynthVibe::FxChainStrip> strip;
};
```

- [ ] **Step 4: Build and run tests**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynth Tests.exe"
```

Expected: plugin target builds; `FxTab constructs with chain strip attached` passes; all earlier tests still green.

- [ ] **Step 5: Commit**

```
git add Source/UI/FXTab.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): rewrite FxTab around FxChainStrip"
```

---

## Task 13: Visual smoke — deploy + manual host validation

**Files:** none (manual validation).

- [ ] **Step 1: Rebuild plugin in Release**

```
./build-with-vs.bat
```

Expected: no errors; `build/AISynth_artefacts/Release/VST3/AI Synth.vst3` is up to date.

- [ ] **Step 2: Confirm Reaper is closed (it locks the VST3 DLL)**

```
tasklist //FI "IMAGENAME eq reaper.exe"
```

Expected: `INFO: No tasks are running which match the specified criteria.` If Reaper is open, ask the user to close it — do NOT kill it.

- [ ] **Step 3: Deploy**

Run from PowerShell (not bash):

```
./deploy.ps1
```

Expected: new `AI Synth vN.vst3` copied into `C:\Program Files\Common Files\VST3\` with an incremented version number.

- [ ] **Step 4: Manual validation checklist**

Ask the user to open Reaper and confirm:

- [ ] The FX tab shows a 2-row × 5-column grid of slot cards (10 slots, numbered 1–10).
- [ ] Each card shows: slot number badge, type dropdown (default "None"), bypass toggle, "Mix" knob, and 4 generic knobs.
- [ ] Selecting "Drive" in slot 1 makes only the **Type** and **Amount** knob labels visible; **P3** / **P4** knobs hide.
- [ ] Selecting "EQ3" in slot 2 shows **Low / Mid / High / Mid Hz** as labels for p1..p4.
- [ ] Setting type=Delay + p1≈0.3 + p2≈0.4 + mix>0 yields an audible delay effect with no glitches.
- [ ] Toggling the bypass dot mutes the slot's effect (audio passes through dry).
- [ ] Setting type=Phaser/Flanger/Bitcrush/Filter/Comp confirms passthrough (silent on the user's side because the type indicator should show but no DSP runs).
- [ ] Save the host project, close, reopen — every slot's type, bypass, mix, p1..p4 persists.
- [ ] Switching to Sound / Mod / Arp tabs and back leaves slot state untouched.

- [ ] **Step 5: Take a screenshot of the rendered FX tab and compare against `docs/ClaudeDesign/AISynth V1 Hi-Fi.html` FX section**

If the layout structurally diverges from the mockup, treat it as a design brief for a follow-up iteration — do NOT bury as polish.

---

## What this plan explicitly does NOT ship

- **DSP for stub types (Phaser / Flanger / Bitcrush / Filter / Comp):** they appear in the dropdown and persist across save/load, but `FxSlot::process()` falls through to a no-op. Adding real DSP is a follow-up plan per type.
- **FXVisualization mini-display per slot:** the mockup shows a tiny per-effect preview (impulse response for delay, sine ripple for chorus, etc.). Out of scope for V1; the slot card is just knobs + header.
- **Drag-reorder:** slot order = slot index. The roadmap allows revisiting if the user wants visual reordering later.
- **Per-type accent color on slot cards:** all cards use the neutral `Tokens::fx` (teal). Color-coded slot borders by effect type are a polish-pass concern.
- **Migration from legacy presets:** Pick 1A is destructive — old presets with `fx.delay.time` etc. silently lose those values. No migration path is generated.
- **Right-click context menu** (clear slot, copy slot, etc.) — JUCE PopupMenu work for a polish pass.

---

## Self-review

**Spec coverage (vs. roadmap Phase 4 + components.md FX Chain rows):**
- `FXChainStrip` (components.md row 70) — Task 11
- `FXSlot` (components.md row 71) — Task 10 (`FxSlotCard`; class renamed to disambiguate from the engine `FxSlot`)
- `FXSlotHeader` (inline) — folded into `FxSlotCard::paint` (badge + type combo + bypass toggle in the header strip, drawn directly, no separate class)
- `FXVisualization` (components.md row 73) — explicitly deferred per "What this plan does NOT ship"
- `fx.1.{type,bypass,mix,p1..p4}` … `fx.10.*` (param-audit.md FX Chain table) — Tasks 1 + 2
- "≥ 3 types: drive, delay, reverb; the rest are stubs" (roadmap Phase 3 wording, applied here in Phase 4) — exceeded: Drive + Chorus + Delay + Reverb + Eq3 ship working DSP; Comp/Phaser/Flanger/Bitcrush/Filter are stubs

**Three user decisions covered:**
- 1.A (delete legacy IDs immediately) — Task 1 deletes constants, Task 1 Step 7 deletes layout entries, **Task 1 Step 1 adds a negative test** that any future re-introduction will catch.
- 2. Stretch (drive + chorus + delay + reverb + 1 new DSP) — Tasks 4–7 add Eq3 + the slot/runner glue (or Comp if you flip the open decision).
- 3.B (slot order = index) — `FxChainStrip::resized()` lays slot 1..10 in fixed grid order; no drag-and-drop code.

**Placeholder scan:** every step contains the literal code or the literal command. No "TBD" / "implement later" / "similar to Task N" anywhere. The single conditional ("If you flipped the decision to Comp") is bracketed at the plan top — it's a parameter, not a hole.

**Type consistency:**
- `FxRunner::kNumSlots` is `10` everywhere it appears (header, `FxChainStrip::kNumSlots`, runner test).
- `kFxTypeCount` is `11` everywhere (FxParams.h definition, ParameterLayout test assertion, FxSlotTypePicker, FxTypeMap arrays).
- `FxSlotParams` field order matches what `buildFxSnapshot` writes and what `toXxxParams` reads.
- `FxSlot::process()` switch has the same cases as the `setParams` switch — both use `FxType`. Stub types (Comp/Phaser/Flanger/Bitcrush/Filter) hit `default:` in both, returning a passthrough.
- The `FxSlotCard` test passes 8 args (apvts + 7 IDs + slotIndex); the constructor signature in Step 3 takes the same. Same for the `FxChainStrip` instantiation loop.

**Out-of-scope items properly declared:** the "What this plan does NOT ship" section flags stubs, viz, drag-reorder, per-type tinting, legacy migration, and context menu — preventing scope creep mid-execution.

---

## Execution Handoff

Plan complete and saved to `docs/superpowers/plans/2026-04-25-phase4-fx-slots.md`. Two execution options:

**1. Subagent-Driven (recommended)** — I dispatch a fresh subagent per task, review between tasks, fast iteration.

**2. Inline Execution** — Execute tasks in this session using executing-plans, batch execution with checkpoints.

**Which approach?**
