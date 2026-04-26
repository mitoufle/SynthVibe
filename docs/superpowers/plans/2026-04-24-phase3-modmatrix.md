# Phase 3 — Modulation Matrix (UI + params only) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the bare-bones `ModTab` with an 8-slot modulation matrix UI wired to APVTS, while preserving existing LFO1/LFO2 panels. Engine-side modulation application is out of scope for this plan.

**Architecture:** Six new UI atoms (`CurveSelect`, `ModSourcePicker`, `ModDestPicker`, `BipolarAmountBar`, `ModRow`, `ModMatrixTable`) compose into the refactored `ModTab`. Parameters declared statically for slots `mod.1..16.*` (UI shows 1..8, rest reserved for future-proofing). A new `kDestinations[]` table in `Source/Parameters/ModDestinations.{h,cpp}` provides the choice-index → paramId mapping that `ModDestPicker` reads at runtime.

**Tech Stack:** C++17, JUCE 7.0.9, `juce::AudioProcessorValueTreeState`, `juce::UnitTest` (existing test harness in `Tests/`).

**Build:** `./build-with-vs.bat` (loads vcvars64 + runs cmake/msbuild).
**Tests:** `./build/AISynthTests_artefacts/Release/AISynth Tests.exe`
**Deploy:** `./deploy.ps1` (PowerShell only, not bash).

**Branch:** stay on `phase2-sound-tab` for now (the next commit will be the first after the SoundTab landing). No new branch needed for this plan.

---

## File Structure

| File | Status | Responsibility |
|---|---|---|
| `Source/Parameters/ParameterIDs.h` | modify | add `mod1Src`, `mod1Dst`, `mod1Amount`, `mod1Curve` … through `mod16.*` constants |
| `Source/Parameters/ParameterLayout.cpp` | modify | register 16 × 4 = 64 modulation params via a loop |
| `Source/Parameters/ModDestinations.h` | create | declare `kDestinations[]` table (choice-index → paramId string + display label) |
| `Source/Parameters/ModDestinations.cpp` | create | define the 13-entry destination table |
| `Source/UI/components/CurveSelect.h` | create | 4-pill `lin / exp / log / s` picker bound to a choice param |
| `Source/UI/components/ModSourcePicker.h` | create | dropdown for the 10 modulation sources, color-coded per family |
| `Source/UI/components/ModDestPicker.h` | create | dropdown backed by `kDestinations[]`, shows destination label |
| `Source/UI/components/BipolarAmountBar.h` | create | horizontal -1..+1 drag bar, positive = accent, negative = filter hue |
| `Source/UI/components/ModRow.h` | create | one slot: src · dst · amount · curve, 4 sub-components side by side |
| `Source/UI/components/ModMatrixTable.h` | create | 8 `ModRow` instances stacked vertically with a pinned header strip |
| `Source/UI/ModTab.h` | modify | refactor to `LFO panels` (top ~35%) + `ModMatrixTable` (bottom ~65%) |
| `Tests/ParameterIdMigrationTests.cpp` | modify | add test block for `mod.N.*` IDs + defaults |
| `Tests/UIConstructionTests.cpp` | modify | add construction tests for each new component + refactored ModTab |
| `CMakeLists.txt` | modify | add `Source/Parameters/ModDestinations.cpp` to both `AISynth` and `AISynthTests` source lists |

---

## Task 1: Declare `mod.N.*` parameter IDs

**Files:**
- Modify: `Source/Parameters/ParameterIDs.h`
- Test: `Tests/ParameterIdMigrationTests.cpp`

- [ ] **Step 1: Write the failing test**

Append to the body of `runTest()` in `Tests/ParameterIdMigrationTests.cpp`, after the existing `"IDs use dot-separated scheme"` block (before `beginTest("Phase 2b new parameters…")`):

```cpp
        beginTest("Mod matrix IDs follow mod.N.suffix scheme");
        expectEquals(juce::String(ParamIDs::mod1Src),    juce::String("mod.1.src"));
        expectEquals(juce::String(ParamIDs::mod1Dst),    juce::String("mod.1.dst"));
        expectEquals(juce::String(ParamIDs::mod1Amount), juce::String("mod.1.amount"));
        expectEquals(juce::String(ParamIDs::mod1Curve),  juce::String("mod.1.curve"));
        expectEquals(juce::String(ParamIDs::mod8Src),    juce::String("mod.8.src"));
        expectEquals(juce::String(ParamIDs::mod16Curve), juce::String("mod.16.curve"));
```

- [ ] **Step 2: Run tests to confirm they fail**

```
./build-with-vs.bat
```

Expected: compile error — `mod1Src` not declared in `ParamIDs`.

- [ ] **Step 3: Add the constants to `ParameterIDs.h`**

Append before the closing `}` of `namespace ParamIDs` in `Source/Parameters/ParameterIDs.h`:

```cpp
    // Modulation Matrix — 16 slots declared (UI exposes 1..8; 9..16 reserved for future expansion
    // so presets saved today will not need migration when slot count grows).
    inline constexpr const char* mod1Src  = "mod.1.src";
    inline constexpr const char* mod1Dst  = "mod.1.dst";
    inline constexpr const char* mod1Amount = "mod.1.amount";
    inline constexpr const char* mod1Curve  = "mod.1.curve";
    inline constexpr const char* mod2Src  = "mod.2.src";
    inline constexpr const char* mod2Dst  = "mod.2.dst";
    inline constexpr const char* mod2Amount = "mod.2.amount";
    inline constexpr const char* mod2Curve  = "mod.2.curve";
    inline constexpr const char* mod3Src  = "mod.3.src";
    inline constexpr const char* mod3Dst  = "mod.3.dst";
    inline constexpr const char* mod3Amount = "mod.3.amount";
    inline constexpr const char* mod3Curve  = "mod.3.curve";
    inline constexpr const char* mod4Src  = "mod.4.src";
    inline constexpr const char* mod4Dst  = "mod.4.dst";
    inline constexpr const char* mod4Amount = "mod.4.amount";
    inline constexpr const char* mod4Curve  = "mod.4.curve";
    inline constexpr const char* mod5Src  = "mod.5.src";
    inline constexpr const char* mod5Dst  = "mod.5.dst";
    inline constexpr const char* mod5Amount = "mod.5.amount";
    inline constexpr const char* mod5Curve  = "mod.5.curve";
    inline constexpr const char* mod6Src  = "mod.6.src";
    inline constexpr const char* mod6Dst  = "mod.6.dst";
    inline constexpr const char* mod6Amount = "mod.6.amount";
    inline constexpr const char* mod6Curve  = "mod.6.curve";
    inline constexpr const char* mod7Src  = "mod.7.src";
    inline constexpr const char* mod7Dst  = "mod.7.dst";
    inline constexpr const char* mod7Amount = "mod.7.amount";
    inline constexpr const char* mod7Curve  = "mod.7.curve";
    inline constexpr const char* mod8Src  = "mod.8.src";
    inline constexpr const char* mod8Dst  = "mod.8.dst";
    inline constexpr const char* mod8Amount = "mod.8.amount";
    inline constexpr const char* mod8Curve  = "mod.8.curve";
    inline constexpr const char* mod9Src  = "mod.9.src";
    inline constexpr const char* mod9Dst  = "mod.9.dst";
    inline constexpr const char* mod9Amount = "mod.9.amount";
    inline constexpr const char* mod9Curve  = "mod.9.curve";
    inline constexpr const char* mod10Src  = "mod.10.src";
    inline constexpr const char* mod10Dst  = "mod.10.dst";
    inline constexpr const char* mod10Amount = "mod.10.amount";
    inline constexpr const char* mod10Curve  = "mod.10.curve";
    inline constexpr const char* mod11Src  = "mod.11.src";
    inline constexpr const char* mod11Dst  = "mod.11.dst";
    inline constexpr const char* mod11Amount = "mod.11.amount";
    inline constexpr const char* mod11Curve  = "mod.11.curve";
    inline constexpr const char* mod12Src  = "mod.12.src";
    inline constexpr const char* mod12Dst  = "mod.12.dst";
    inline constexpr const char* mod12Amount = "mod.12.amount";
    inline constexpr const char* mod12Curve  = "mod.12.curve";
    inline constexpr const char* mod13Src  = "mod.13.src";
    inline constexpr const char* mod13Dst  = "mod.13.dst";
    inline constexpr const char* mod13Amount = "mod.13.amount";
    inline constexpr const char* mod13Curve  = "mod.13.curve";
    inline constexpr const char* mod14Src  = "mod.14.src";
    inline constexpr const char* mod14Dst  = "mod.14.dst";
    inline constexpr const char* mod14Amount = "mod.14.amount";
    inline constexpr const char* mod14Curve  = "mod.14.curve";
    inline constexpr const char* mod15Src  = "mod.15.src";
    inline constexpr const char* mod15Dst  = "mod.15.dst";
    inline constexpr const char* mod15Amount = "mod.15.amount";
    inline constexpr const char* mod15Curve  = "mod.15.curve";
    inline constexpr const char* mod16Src  = "mod.16.src";
    inline constexpr const char* mod16Dst  = "mod.16.dst";
    inline constexpr const char* mod16Amount = "mod.16.amount";
    inline constexpr const char* mod16Curve  = "mod.16.curve";
```

- [ ] **Step 4: Run tests to confirm they pass**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynth Tests.exe"
```

Expected: `All tests completed successfully`. The `"Mod matrix IDs follow mod.N.suffix scheme"` block should pass. ParameterLayout tests may not yet cover `mod.*` defaults — that comes in Task 3.

- [ ] **Step 5: Commit**

```
git add Source/Parameters/ParameterIDs.h Tests/ParameterIdMigrationTests.cpp
git commit -m "feat(params): declare mod.1..16.{src,dst,amount,curve} IDs"
```

---

## Task 2: Create the `kDestinations[]` destination-table

**Files:**
- Create: `Source/Parameters/ModDestinations.h`
- Create: `Source/Parameters/ModDestinations.cpp`
- Modify: `CMakeLists.txt` (add to both `AISynth` and `AISynthTests` source lists)
- Test: `Tests/ParameterIdMigrationTests.cpp`

- [ ] **Step 1: Write the failing test**

Add a new `#include` at the top of `Tests/ParameterIdMigrationTests.cpp`:

```cpp
#include "Parameters/ModDestinations.h"
```

Append a new test block to `runTest()` after the previous block from Task 1:

```cpp
        beginTest("kDestinations[] maps curated 13 entries (none + 12 targets)");
        expectEquals((int) SynthVibe::kDestinations.size(), 13);
        expect(juce::String(SynthVibe::kDestinations[0].paramId).isEmpty(),
               "slot 0 is the 'none' sentinel (empty paramId)");
        expectEquals(juce::String(SynthVibe::kDestinations[0].label), juce::String("None"));
        expectEquals(juce::String(SynthVibe::kDestinations[1].paramId), juce::String(ParamIDs::filterCutoff));
        expectEquals(juce::String(SynthVibe::kDestinations[1].label),   juce::String("Cutoff"));
        expectEquals(juce::String(SynthVibe::kDestinations[12].paramId), juce::String(ParamIDs::ampRelease));
        expectEquals(juce::String(SynthVibe::kDestinations[12].label),   juce::String("Amp Release"));
```

- [ ] **Step 2: Run tests to confirm they fail**

```
./build-with-vs.bat
```

Expected: compile error — `ModDestinations.h` missing.

- [ ] **Step 3: Create `Source/Parameters/ModDestinations.h`**

```cpp
#pragma once
#include <array>

namespace SynthVibe
{
    struct ModDestination
    {
        const char* paramId;  // empty string = "none" sentinel (slot 0)
        const char* label;    // human-readable name shown in the UI
    };

    // Curated 13-entry destination table (index 0 = None, 1..12 = targets).
    // Index is persisted in the APVTS choice param; adding new destinations
    // MUST only append to the end so existing presets keep resolving correctly.
    extern const std::array<ModDestination, 13> kDestinations;
}
```

- [ ] **Step 4: Create `Source/Parameters/ModDestinations.cpp`**

```cpp
#include "ModDestinations.h"
#include "ParameterIDs.h"

namespace SynthVibe
{
    const std::array<ModDestination, 13> kDestinations = {{
        { "",                      "None"        },
        { ParamIDs::filterCutoff,  "Cutoff"      },
        { ParamIDs::filterResonance,"Resonance"  },
        { ParamIDs::filterDrive,   "Drive"       },
        { ParamIDs::osc1Detune,    "Osc1 Pitch"  },
        { ParamIDs::osc2Detune,    "Osc2 Pitch"  },
        { ParamIDs::osc1Level,     "Osc1 Level"  },
        { ParamIDs::osc2Level,     "Osc2 Level"  },
        { ParamIDs::osc1Pwm,       "Osc1 PWM"    },
        { ParamIDs::osc2Pwm,       "Osc2 PWM"    },
        { ParamIDs::masterVolume,  "Master Vol"  },
        { ParamIDs::ampAttack,     "Amp Attack"  },
        { ParamIDs::ampRelease,    "Amp Release" },
    }};
}
```

- [ ] **Step 5: Register the new `.cpp` in CMake**

Modify `CMakeLists.txt`. In the `target_sources(AISynth PRIVATE …)` block, add after the existing `Source/Parameters/ParameterLayout.cpp` line:

```cmake
    Source/Parameters/ModDestinations.cpp
```

Then in the `target_sources(AISynthTests PRIVATE …)` block, add the same line after the existing `Source/Parameters/ParameterLayout.cpp`:

```cmake
    Source/Parameters/ModDestinations.cpp
```

- [ ] **Step 6: Run tests to confirm they pass**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynth Tests.exe"
```

Expected: the `"kDestinations[] maps curated 13 entries"` test passes. No prior tests regress.

- [ ] **Step 7: Commit**

```
git add Source/Parameters/ModDestinations.h Source/Parameters/ModDestinations.cpp CMakeLists.txt Tests/ParameterIdMigrationTests.cpp
git commit -m "feat(params): add kDestinations[] mod destination table"
```

---

## Task 3: Register `mod.N.*` parameters in ParameterLayout

**Files:**
- Modify: `Source/Parameters/ParameterLayout.cpp`
- Test: `Tests/ParameterIdMigrationTests.cpp`

- [ ] **Step 1: Write the failing test**

Append to `runTest()` in `Tests/ParameterIdMigrationTests.cpp`, inside the same braced scope that constructs the APVTS (the `"Phase 2b new parameters are registered with correct defaults"` block already instantiates one; reuse it, or open a new block with its own APVTS). Add a new `beginTest` block:

```cpp
        beginTest("Mod slots register with correct defaults");
        {
            juce::AudioProcessorGraph dummyGraph;
            juce::AudioProcessorValueTreeState apvts(
                dummyGraph, nullptr, "AISynthState", ParameterLayout::create());

            auto readFloat = [&](const char* id) {
                auto* p = apvts.getRawParameterValue(id);
                expect(p != nullptr, juce::String("missing param: ") + id);
                return p ? p->load() : 0.f;
            };

            // Source default = 0 ("none")
            expectEquals(readFloat(ParamIDs::mod1Src), 0.f);
            // Destination default = 0 ("none")
            expectEquals(readFloat(ParamIDs::mod1Dst), 0.f);
            // Amount default = 0 (centre of -1..+1)
            expectWithinAbsoluteError(readFloat(ParamIDs::mod1Amount), 0.f, 0.001f);
            // Curve default = 0 ("lin")
            expectEquals(readFloat(ParamIDs::mod1Curve), 0.f);

            // Reserved slots 9..16 also present
            expect(apvts.getRawParameterValue(ParamIDs::mod16Amount) != nullptr,
                   "reserved slot 16 amount must be registered");
        }
```

- [ ] **Step 2: Run tests to confirm they fail**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynth Tests.exe"
```

Expected: test fails — `readFloat` logs `"missing param: mod.1.src"`, slot 16 not registered.

- [ ] **Step 3: Add the mod-slot registration loop to `ParameterLayout.cpp`**

In `Source/Parameters/ParameterLayout.cpp`, add `#include "ModDestinations.h"` at the top (after the two existing includes):

```cpp
#include "ModDestinations.h"
```

Append the registration block before `return { params.begin(), params.end() };`:

```cpp
    // -----------------------------------------------------------------------
    // Modulation Matrix — 16 slots (UI shows 1..8; 9..16 reserved)
    // -----------------------------------------------------------------------
    {
        const StringArray srcLabels {
            "None", "LFO 1", "LFO 2", "Env Amp", "Env Filt",
            "Velocity", "Modwheel", "Aftertouch", "Keytrack", "Random"
        };

        StringArray dstLabels;
        for (const auto& d : SynthVibe::kDestinations)
            dstLabels.add(d.label);

        const StringArray curveLabels { "lin", "exp", "log", "s" };

        struct SlotIds { const char* src; const char* dst; const char* amount; const char* curve; };
        const SlotIds slots[16] = {
            { ParamIDs::mod1Src,  ParamIDs::mod1Dst,  ParamIDs::mod1Amount,  ParamIDs::mod1Curve  },
            { ParamIDs::mod2Src,  ParamIDs::mod2Dst,  ParamIDs::mod2Amount,  ParamIDs::mod2Curve  },
            { ParamIDs::mod3Src,  ParamIDs::mod3Dst,  ParamIDs::mod3Amount,  ParamIDs::mod3Curve  },
            { ParamIDs::mod4Src,  ParamIDs::mod4Dst,  ParamIDs::mod4Amount,  ParamIDs::mod4Curve  },
            { ParamIDs::mod5Src,  ParamIDs::mod5Dst,  ParamIDs::mod5Amount,  ParamIDs::mod5Curve  },
            { ParamIDs::mod6Src,  ParamIDs::mod6Dst,  ParamIDs::mod6Amount,  ParamIDs::mod6Curve  },
            { ParamIDs::mod7Src,  ParamIDs::mod7Dst,  ParamIDs::mod7Amount,  ParamIDs::mod7Curve  },
            { ParamIDs::mod8Src,  ParamIDs::mod8Dst,  ParamIDs::mod8Amount,  ParamIDs::mod8Curve  },
            { ParamIDs::mod9Src,  ParamIDs::mod9Dst,  ParamIDs::mod9Amount,  ParamIDs::mod9Curve  },
            { ParamIDs::mod10Src, ParamIDs::mod10Dst, ParamIDs::mod10Amount, ParamIDs::mod10Curve },
            { ParamIDs::mod11Src, ParamIDs::mod11Dst, ParamIDs::mod11Amount, ParamIDs::mod11Curve },
            { ParamIDs::mod12Src, ParamIDs::mod12Dst, ParamIDs::mod12Amount, ParamIDs::mod12Curve },
            { ParamIDs::mod13Src, ParamIDs::mod13Dst, ParamIDs::mod13Amount, ParamIDs::mod13Curve },
            { ParamIDs::mod14Src, ParamIDs::mod14Dst, ParamIDs::mod14Amount, ParamIDs::mod14Curve },
            { ParamIDs::mod15Src, ParamIDs::mod15Dst, ParamIDs::mod15Amount, ParamIDs::mod15Curve },
            { ParamIDs::mod16Src, ParamIDs::mod16Dst, ParamIDs::mod16Amount, ParamIDs::mod16Curve },
        };

        for (int i = 0; i < 16; ++i)
        {
            const auto& s = slots[i];
            const auto label = "Mod " + String(i + 1) + " ";
            params.push_back(std::make_unique<AudioParameterChoice>(
                s.src, label + "Src", srcLabels, 0));
            params.push_back(std::make_unique<AudioParameterChoice>(
                s.dst, label + "Dst", dstLabels, 0));
            params.push_back(std::make_unique<AudioParameterFloat>(
                s.amount, label + "Amount",
                NormalisableRange<float>(-1.f, 1.f, 0.001f), 0.f));
            params.push_back(std::make_unique<AudioParameterChoice>(
                s.curve, label + "Curve", curveLabels, 0));
        }
    }
```

- [ ] **Step 4: Run tests to confirm they pass**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynth Tests.exe"
```

Expected: `"Mod slots register with correct defaults"` passes. All earlier tests still green.

- [ ] **Step 5: Commit**

```
git add Source/Parameters/ParameterLayout.cpp Tests/ParameterIdMigrationTests.cpp
git commit -m "feat(params): register 16 modulation slot parameters"
```

---

## Task 4: `CurveSelect` component

**Files:**
- Create: `Source/UI/components/CurveSelect.h`
- Test: `Tests/UIConstructionTests.cpp`

- [ ] **Step 1: Write the failing test**

Add to the `#include` block at the top of `Tests/UIConstructionTests.cpp`:

```cpp
#include "UI/components/CurveSelect.h"
```

Append a new test block inside `runTest()` after the `EnvelopeEditor` one (before `SoundTab`):

```cpp
        beginTest("CurveSelect binds to a choice param");
        {
            SynthVibe::CurveSelect cs(apvts, ParamIDs::mod1Curve);
            cs.setBounds(0, 0, 120, 22);
            // default curve index = 0 ("lin")
            expectEquals(cs.getCurrentIndex(), 0);
        }
```

- [ ] **Step 2: Run tests to confirm they fail**

```
./build-with-vs.bat
```

Expected: compile error — `CurveSelect.h` missing.

- [ ] **Step 3: Create `Source/UI/components/CurveSelect.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class CurveSelect : public juce::Component
    {
    public:
        CurveSelect(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID)
        {
            const char* labels[] = { "lin", "exp", "log", "s" };
            for (int i = 0; i < 4; ++i)
            {
                auto& btn = pills[i];
                btn.setButtonText(labels[i]);
                btn.setClickingTogglesState(true);
                btn.setRadioGroupId(41);
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
                    currentIndex = juce::jlimit(0, 3, (int) std::round(v));
                    for (int i = 0; i < 4; ++i)
                        pills[i].setToggleState(i == currentIndex, juce::dontSendNotification);
                });
            paramAttach->sendInitialUpdate();
        }

        int getCurrentIndex() const noexcept { return currentIndex; }

        void resized() override
        {
            auto b = getLocalBounds();
            const int w = b.getWidth() / 4;
            for (int i = 0; i < 4; ++i)
                pills[i].setBounds(b.removeFromLeft(w).reduced(1));
        }

    private:
        juce::TextButton pills[4];
        int currentIndex = 0;
        std::unique_ptr<juce::ParameterAttachment> paramAttach;
    };
}
```

- [ ] **Step 4: Run tests to confirm they pass**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynth Tests.exe"
```

Expected: `"CurveSelect binds to a choice param"` passes.

- [ ] **Step 5: Commit**

```
git add Source/UI/components/CurveSelect.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): CurveSelect pill-row for mod matrix curve param"
```

---

## Task 5: `ModSourcePicker` component

**Files:**
- Create: `Source/UI/components/ModSourcePicker.h`
- Test: `Tests/UIConstructionTests.cpp`

- [ ] **Step 1: Write the failing test**

Add to includes:

```cpp
#include "UI/components/ModSourcePicker.h"
```

Append a new block in `runTest()`:

```cpp
        beginTest("ModSourcePicker binds and defaults to 'None'");
        {
            SynthVibe::ModSourcePicker src(apvts, ParamIDs::mod1Src);
            src.setBounds(0, 0, 110, 24);
            // Default source index = 0 ("None"); ComboBox IDs are 1-based → 1.
            expectEquals(src.getCombo().getSelectedId(), 1);
        }
```

- [ ] **Step 2: Run tests to confirm they fail**

```
./build-with-vs.bat
```

Expected: compile error — `ModSourcePicker.h` missing.

- [ ] **Step 3: Create `Source/UI/components/ModSourcePicker.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"

namespace SynthVibe
{
    class ModSourcePicker : public juce::Component
    {
    public:
        ModSourcePicker(juce::AudioProcessorValueTreeState& apvts,
                        const juce::String& paramID)
        {
            const char* labels[] = {
                "None", "LFO 1", "LFO 2", "Env Amp", "Env Filt",
                "Velocity", "Modwheel", "Aftertouch", "Keytrack", "Random"
            };
            for (int i = 0; i < 10; ++i)
                combo.addItem(labels[i], i + 1);

            attach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                apvts, paramID, combo);

            combo.setColour(juce::ComboBox::backgroundColourId, Tokens::panel2);
            combo.setColour(juce::ComboBox::outlineColourId,    Tokens::edge);
            combo.setColour(juce::ComboBox::textColourId,       Tokens::ink);
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

- [ ] **Step 4: Run tests to confirm they pass**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynth Tests.exe"
```

Expected: `"ModSourcePicker binds and defaults to 'None'"` passes.

- [ ] **Step 5: Commit**

```
git add Source/UI/components/ModSourcePicker.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): ModSourcePicker dropdown for mod matrix source"
```

---

## Task 6: `ModDestPicker` component

**Files:**
- Create: `Source/UI/components/ModDestPicker.h`
- Test: `Tests/UIConstructionTests.cpp`

- [ ] **Step 1: Write the failing test**

Add to includes:

```cpp
#include "UI/components/ModDestPicker.h"
```

Append a new test block:

```cpp
        beginTest("ModDestPicker reads kDestinations[] and defaults to 'None'");
        {
            SynthVibe::ModDestPicker dst(apvts, ParamIDs::mod1Dst);
            dst.setBounds(0, 0, 140, 24);
            // Default destination index = 0 ("None"); ComboBox IDs are 1-based → 1.
            expectEquals(dst.getCombo().getSelectedId(), 1);
            // 13 entries expected (matches kDestinations size)
            expectEquals(dst.getCombo().getNumItems(), 13);
        }
```

- [ ] **Step 2: Run tests to confirm they fail**

```
./build-with-vs.bat
```

Expected: compile error — `ModDestPicker.h` missing.

- [ ] **Step 3: Create `Source/UI/components/ModDestPicker.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"
#include "../../Parameters/ModDestinations.h"

namespace SynthVibe
{
    class ModDestPicker : public juce::Component
    {
    public:
        ModDestPicker(juce::AudioProcessorValueTreeState& apvts,
                      const juce::String& paramID)
        {
            for (int i = 0; i < (int) kDestinations.size(); ++i)
                combo.addItem(kDestinations[(size_t) i].label, i + 1);

            attach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                apvts, paramID, combo);

            combo.setColour(juce::ComboBox::backgroundColourId, Tokens::panel2);
            combo.setColour(juce::ComboBox::outlineColourId,    Tokens::edge);
            combo.setColour(juce::ComboBox::textColourId,       Tokens::ink);
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

- [ ] **Step 4: Run tests to confirm they pass**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynth Tests.exe"
```

Expected: both expectations in the new test pass.

- [ ] **Step 5: Commit**

```
git add Source/UI/components/ModDestPicker.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): ModDestPicker dropdown backed by kDestinations"
```

---

## Task 7: `BipolarAmountBar` component

**Files:**
- Create: `Source/UI/components/BipolarAmountBar.h`
- Test: `Tests/UIConstructionTests.cpp`

- [ ] **Step 1: Write the failing test**

Add to includes:

```cpp
#include "UI/components/BipolarAmountBar.h"
```

Append a new test block:

```cpp
        beginTest("BipolarAmountBar reads amount float param");
        {
            SynthVibe::BipolarAmountBar bar(apvts, ParamIDs::mod1Amount);
            bar.setBounds(0, 0, 180, 18);
            // Default mod.1.amount = 0
            expectWithinAbsoluteError(bar.getValue(), 0.f, 0.001f);
        }
```

- [ ] **Step 2: Run tests to confirm they fail**

```
./build-with-vs.bat
```

Expected: compile error — `BipolarAmountBar.h` missing.

- [ ] **Step 3: Create `Source/UI/components/BipolarAmountBar.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"

namespace SynthVibe
{
    // Horizontal -1..+1 drag bar. Click-drag anywhere inside sets the value
    // proportional to x / width (0 is dead-centre). Double-click resets to 0.
    // Positive fills with Tokens::modPositive (violet); negative fills with
    // Tokens::modNegative (pink).
    class BipolarAmountBar : public juce::Component
    {
    public:
        BipolarAmountBar(juce::AudioProcessorValueTreeState& apvts,
                         const juce::String& paramID)
            : parameter(apvts.getParameter(paramID))
        {
            jassert(parameter != nullptr);
            attach = std::make_unique<juce::ParameterAttachment>(
                *parameter,
                [this](float v) {
                    value = v;
                    repaint();
                });
            attach->sendInitialUpdate();
        }

        float getValue() const noexcept { return value; }

        void paint(juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat();
            g.setColour(Tokens::panel2);
            g.fillRoundedRectangle(b, Tokens::radiusSm);
            g.setColour(Tokens::edge);
            g.drawRoundedRectangle(b.reduced(0.5f), Tokens::radiusSm, 1.f);

            // Center tick
            const float cx = b.getCentreX();
            g.setColour(Tokens::ink4);
            g.drawLine(cx, b.getY() + 3.f, cx, b.getBottom() - 3.f, 1.f);

            // Fill from centre toward current value
            const float halfW = b.getWidth() * 0.5f;
            const float filled = juce::jlimit(-1.f, 1.f, value) * halfW;
            const auto colour = value >= 0.f ? Tokens::modPositive : Tokens::modNegative;
            g.setColour(colour);
            juce::Rectangle<float> fill;
            if (value >= 0.f)
                fill = { cx, b.getY() + 2.f, filled, b.getHeight() - 4.f };
            else
                fill = { cx + filled, b.getY() + 2.f, -filled, b.getHeight() - 4.f };
            g.fillRoundedRectangle(fill, Tokens::radiusSm * 0.5f);
        }

        void mouseDown(const juce::MouseEvent& e) override { updateFromMouse(e); }
        void mouseDrag(const juce::MouseEvent& e) override { updateFromMouse(e); }

        void mouseDoubleClick(const juce::MouseEvent&) override
        {
            if (parameter != nullptr)
                parameter->setValueNotifyingHost(parameter->convertTo0to1(0.f));
        }

    private:
        void updateFromMouse(const juce::MouseEvent& e)
        {
            if (parameter == nullptr) return;
            const float w = (float) getWidth();
            if (w <= 0.f) return;
            const float norm = juce::jlimit(0.f, 1.f, e.position.x / w);
            const float v    = norm * 2.f - 1.f;  // [0,1] → [-1,+1]
            parameter->setValueNotifyingHost(parameter->convertTo0to1(v));
        }

        juce::RangedAudioParameter* parameter = nullptr;
        std::unique_ptr<juce::ParameterAttachment> attach;
        float value = 0.f;
    };
}
```

- [ ] **Step 4: Run tests to confirm they pass**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynth Tests.exe"
```

Expected: `"BipolarAmountBar reads amount float param"` passes.

- [ ] **Step 5: Commit**

```
git add Source/UI/components/BipolarAmountBar.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): BipolarAmountBar drag widget for mod amount"
```

---

## Task 8: `ModRow` component

**Files:**
- Create: `Source/UI/components/ModRow.h`
- Test: `Tests/UIConstructionTests.cpp`

- [ ] **Step 1: Write the failing test**

Add to includes:

```cpp
#include "UI/components/ModRow.h"
```

Append a new test block:

```cpp
        beginTest("ModRow composes src/dst/amount/curve for slot 1");
        {
            SynthVibe::ModRow row(apvts,
                ParamIDs::mod1Src, ParamIDs::mod1Dst,
                ParamIDs::mod1Amount, ParamIDs::mod1Curve);
            row.setBounds(0, 0, 640, 28);
            expect(row.getWidth() == 640);
            expect(row.getHeight() == 28);
        }
```

- [ ] **Step 2: Run tests to confirm they fail**

```
./build-with-vs.bat
```

Expected: compile error — `ModRow.h` missing.

- [ ] **Step 3: Create `Source/UI/components/ModRow.h`**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "ModSourcePicker.h"
#include "ModDestPicker.h"
#include "BipolarAmountBar.h"
#include "CurveSelect.h"
#include "../DesignTokens.h"

namespace SynthVibe
{
    class ModRow : public juce::Component
    {
    public:
        ModRow(juce::AudioProcessorValueTreeState& apvts,
               const juce::String& srcId,
               const juce::String& dstId,
               const juce::String& amountId,
               const juce::String& curveId)
            : src(apvts, srcId),
              dst(apvts, dstId),
              amount(apvts, amountId),
              curve(apvts, curveId)
        {
            addAndMakeVisible(src);
            addAndMakeVisible(dst);
            addAndMakeVisible(amount);
            addAndMakeVisible(curve);
        }

        void paint(juce::Graphics& g) override
        {
            g.setColour(Tokens::edge.withAlpha(0.35f));
            g.drawHorizontalLine(getHeight() - 1, 0.f, (float) getWidth());
        }

        void resized() override
        {
            // Column layout: src 20% · dst 28% · amount 38% · curve 14%
            auto b = getLocalBounds().reduced(Tokens::spaceXs, Tokens::spaceXs / 2);
            const int wSrc    = (int) (b.getWidth() * 0.20f);
            const int wDst    = (int) (b.getWidth() * 0.28f);
            const int wCurve  = (int) (b.getWidth() * 0.14f);
            src   .setBounds(b.removeFromLeft(wSrc   ).reduced(2, 0));
            dst   .setBounds(b.removeFromLeft(wDst   ).reduced(2, 0));
            curve .setBounds(b.removeFromRight(wCurve).reduced(2, 0));
            amount.setBounds(b.reduced(Tokens::spaceSm, 4));
        }

    private:
        ModSourcePicker src;
        ModDestPicker   dst;
        BipolarAmountBar amount;
        CurveSelect     curve;
    };
}
```

- [ ] **Step 4: Run tests to confirm they pass**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynth Tests.exe"
```

Expected: `"ModRow composes src/dst/amount/curve for slot 1"` passes.

- [ ] **Step 5: Commit**

```
git add Source/UI/components/ModRow.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): ModRow composes src/dst/amount/curve widgets"
```

---

## Task 9: `ModMatrixTable` component

**Files:**
- Create: `Source/UI/components/ModMatrixTable.h`
- Test: `Tests/UIConstructionTests.cpp`

- [ ] **Step 1: Write the failing test**

Add to includes:

```cpp
#include "UI/components/ModMatrixTable.h"
```

Append a new test block:

```cpp
        beginTest("ModMatrixTable renders 8 rows");
        {
            SynthVibe::ModMatrixTable table(apvts);
            table.setBounds(0, 0, 720, 360);
            expectEquals(table.getNumRows(), 8);
        }
```

- [ ] **Step 2: Run tests to confirm they fail**

```
./build-with-vs.bat
```

Expected: compile error — `ModMatrixTable.h` missing.

- [ ] **Step 3: Create `Source/UI/components/ModMatrixTable.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "ModRow.h"
#include "../DesignTokens.h"
#include "../Fonts.h"
#include "../../Parameters/ParameterIDs.h"

namespace SynthVibe
{
    class ModMatrixTable : public juce::Component
    {
    public:
        static constexpr int kVisibleRows = 8;

        explicit ModMatrixTable(juce::AudioProcessorValueTreeState& apvts)
        {
            struct SlotIds { const char* src; const char* dst; const char* amount; const char* curve; };
            const SlotIds slots[kVisibleRows] = {
                { ParamIDs::mod1Src, ParamIDs::mod1Dst, ParamIDs::mod1Amount, ParamIDs::mod1Curve },
                { ParamIDs::mod2Src, ParamIDs::mod2Dst, ParamIDs::mod2Amount, ParamIDs::mod2Curve },
                { ParamIDs::mod3Src, ParamIDs::mod3Dst, ParamIDs::mod3Amount, ParamIDs::mod3Curve },
                { ParamIDs::mod4Src, ParamIDs::mod4Dst, ParamIDs::mod4Amount, ParamIDs::mod4Curve },
                { ParamIDs::mod5Src, ParamIDs::mod5Dst, ParamIDs::mod5Amount, ParamIDs::mod5Curve },
                { ParamIDs::mod6Src, ParamIDs::mod6Dst, ParamIDs::mod6Amount, ParamIDs::mod6Curve },
                { ParamIDs::mod7Src, ParamIDs::mod7Dst, ParamIDs::mod7Amount, ParamIDs::mod7Curve },
                { ParamIDs::mod8Src, ParamIDs::mod8Dst, ParamIDs::mod8Amount, ParamIDs::mod8Curve },
            };
            for (int i = 0; i < kVisibleRows; ++i)
            {
                auto& slot = slots[i];
                rows[i] = std::make_unique<ModRow>(apvts,
                    slot.src, slot.dst, slot.amount, slot.curve);
                addAndMakeVisible(*rows[i]);
            }
        }

        int getNumRows() const noexcept { return kVisibleRows; }

        void paint(juce::Graphics& g) override
        {
            auto b = getLocalBounds();
            g.setColour(Tokens::panel2);
            g.fillRoundedRectangle(b.toFloat(), Tokens::radiusLg);
            g.setColour(Tokens::edge);
            g.drawRoundedRectangle(b.toFloat().reduced(0.5f), Tokens::radiusLg, 1.f);

            // Header strip
            auto header = b.removeFromTop(kHeaderH);
            g.setColour(Tokens::ink3);
            g.setFont(juce::Font(Tokens::Font::label, juce::Font::bold));
            drawColumn(g, header, 0.00f, 0.20f, "SRC");
            drawColumn(g, header, 0.20f, 0.48f, "DST");
            drawColumn(g, header, 0.48f, 0.86f, "AMOUNT");
            drawColumn(g, header, 0.86f, 1.00f, "CURVE");

            g.setColour(Tokens::edge);
            g.drawHorizontalLine(kHeaderH, 0.f, (float) getWidth());
        }

        void resized() override
        {
            auto b = getLocalBounds();
            b.removeFromTop(kHeaderH);
            const int rowH = b.getHeight() / kVisibleRows;
            for (int i = 0; i < kVisibleRows; ++i)
                rows[i]->setBounds(b.removeFromTop(rowH));
        }

    private:
        static constexpr int kHeaderH = 20;

        static void drawColumn(juce::Graphics& g, juce::Rectangle<int> header,
                               float xFrac0, float xFrac1, const juce::String& label)
        {
            const int x0 = (int) (header.getWidth() * xFrac0);
            const int x1 = (int) (header.getWidth() * xFrac1);
            auto cell = juce::Rectangle<int>(x0, header.getY(), x1 - x0, header.getHeight())
                            .reduced(Tokens::spaceSm, 0);
            g.drawText(label, cell, juce::Justification::centredLeft);
        }

        std::unique_ptr<ModRow> rows[kVisibleRows];
    };
}
```

- [ ] **Step 4: Run tests to confirm they pass**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynth Tests.exe"
```

Expected: `"ModMatrixTable renders 8 rows"` passes.

- [ ] **Step 5: Commit**

```
git add Source/UI/components/ModMatrixTable.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): ModMatrixTable stacks 8 mod rows with header"
```

---

## Task 10: Wire `ModMatrixTable` into `ModTab`

**Files:**
- Modify: `Source/UI/ModTab.h`
- Test: `Tests/UIConstructionTests.cpp`

- [ ] **Step 1: Write the failing test**

Append a new test block inside `runTest()` (after the existing `SoundTab` block):

```cpp
        beginTest("ModTab constructs with mod matrix attached");
        {
            ModTab tab(apvts);
            tab.setBounds(0, 0, 1280, 560);
            expectEquals(tab.getWidth(), 1280);
            expect(tab.getMatrix() != nullptr, "ModTab should own a ModMatrixTable");
            expectEquals(tab.getMatrix()->getNumRows(), 8);
        }
```

- [ ] **Step 2: Run tests to confirm they fail**

```
./build-with-vs.bat
```

Expected: compile error — `ModTab::getMatrix` not declared.

- [ ] **Step 3: Refactor `Source/UI/ModTab.h`**

Replace the full contents of `Source/UI/ModTab.h` with:

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "KnobWithLabel.h"
#include "LookAndFeel.h"
#include "DesignTokens.h"
#include "components/ModMatrixTable.h"
#include "../Parameters/ParameterIDs.h"

class ModTab : public juce::Component
{
public:
    explicit ModTab(juce::AudioProcessorValueTreeState& apvts) : apvts(apvts)
    {
        lfo1ShapeBox.addItemList({ "Sine", "Saw", "Square", "Triangle" }, 1);
        lfo1ShapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, ParamIDs::lfo1Shape, lfo1ShapeBox);
        lfo1DestBox.addItemList({ "Pitch", "Filter", "Amp", "Detune" }, 1);
        lfo1DestAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, ParamIDs::lfo1Dest, lfo1DestBox);

        lfo2ShapeBox.addItemList({ "Sine", "Saw", "Square", "Triangle" }, 1);
        lfo2ShapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, ParamIDs::lfo2Shape, lfo2ShapeBox);
        lfo2DestBox.addItemList({ "Pitch", "Filter", "Amp", "Detune" }, 1);
        lfo2DestAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, ParamIDs::lfo2Dest, lfo2DestBox);

        for (auto* c : { &lfo1ShapeBox, &lfo1DestBox, &lfo2ShapeBox, &lfo2DestBox })
            addAndMakeVisible(c);
        addAndMakeVisible(knobLfo1Rate);
        addAndMakeVisible(knobLfo1Depth);
        addAndMakeVisible(knobLfo2Rate);
        addAndMakeVisible(knobLfo2Depth);

        matrix = std::make_unique<SynthVibe::ModMatrixTable>(apvts);
        addAndMakeVisible(*matrix);
    }

    SynthVibe::ModMatrixTable* getMatrix() noexcept { return matrix.get(); }

    void paint(juce::Graphics& g) override
    {
        drawPanel(g, lfo1Bounds, "LFO 1", SynthLookAndFeel::colLfoAccent);
        drawPanel(g, lfo2Bounds, "LFO 2", SynthLookAndFeel::colLfoAccent);
    }

    void resized() override
    {
        using namespace SynthVibe::Tokens;
        auto area = getLocalBounds().reduced(spaceMd);

        // Top 35%: two LFO panels side-by-side. Bottom 65%: mod matrix.
        auto top = area.removeFromTop(juce::roundToInt(area.getHeight() * 0.35f));
        area.removeFromTop(spaceMd);   // gap between the two regions

        lfo1Bounds = top.removeFromLeft(top.getWidth() / 2).reduced(spaceSm, 0);
        lfo2Bounds = top.reduced(spaceSm, 0);

        const int comboH = 26;
        const int titleH = 20;
        auto layoutLfo = [&](juce::Rectangle<int> bounds,
                             juce::ComboBox& shapeBox, juce::ComboBox& destBox,
                             KnobWithLabel& rate, KnobWithLabel& depth)
        {
            auto b = bounds.withTrimmedTop(titleH);
            shapeBox.setBounds(b.removeFromTop(comboH));
            destBox .setBounds(b.removeFromTop(comboH));
            const int knobW = b.getWidth() / 2;
            rate .setBounds(b.removeFromLeft(knobW));
            depth.setBounds(b);
        };

        layoutLfo(lfo1Bounds, lfo1ShapeBox, lfo1DestBox, knobLfo1Rate, knobLfo1Depth);
        layoutLfo(lfo2Bounds, lfo2ShapeBox, lfo2DestBox, knobLfo2Rate, knobLfo2Depth);

        if (matrix != nullptr)
            matrix->setBounds(area);
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::Rectangle<int> lfo1Bounds, lfo2Bounds;

    juce::ComboBox lfo1ShapeBox, lfo1DestBox, lfo2ShapeBox, lfo2DestBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        lfo1ShapeAttach, lfo1DestAttach, lfo2ShapeAttach, lfo2DestAttach;

    KnobWithLabel knobLfo1Rate  { "Rate",  apvts, ParamIDs::lfo1Rate,  " Hz", 2 };
    KnobWithLabel knobLfo1Depth { "Depth", apvts, ParamIDs::lfo1Depth, "",    2 };
    KnobWithLabel knobLfo2Rate  { "Rate",  apvts, ParamIDs::lfo2Rate,  " Hz", 2 };
    KnobWithLabel knobLfo2Depth { "Depth", apvts, ParamIDs::lfo2Depth, "",    2 };

    std::unique_ptr<SynthVibe::ModMatrixTable> matrix;

    static void drawPanel(juce::Graphics& g, juce::Rectangle<int> bounds,
                          const juce::String& title, juce::uint32 accentColour)
    {
        auto f = bounds.toFloat().reduced(2.f);
        g.setColour(juce::Colour(SynthLookAndFeel::colPanel));
        g.fillRoundedRectangle(f, 6.f);
        g.setColour(juce::Colour(accentColour));
        g.drawRoundedRectangle(f, 6.f, 1.f);
        g.setColour(juce::Colour(SynthLookAndFeel::colHighlight));
        g.setFont(juce::Font(11.f, juce::Font::bold));
        g.drawText(title, bounds.getX() + 10, bounds.getY() + 4,
                   bounds.getWidth() - 20, 16, juce::Justification::centredLeft);
    }
};
```

- [ ] **Step 4: Run tests to confirm they pass**

```
./build-with-vs.bat && "./build/AISynthTests_artefacts/Release/AISynth Tests.exe"
```

Expected: `"ModTab constructs with mod matrix attached"` passes. All earlier tests still green.

- [ ] **Step 5: Commit**

```
git add Source/UI/ModTab.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): wire ModMatrixTable into ModTab below LFO panels"
```

---

## Task 11: Visual smoke — deploy + host load

**Files:** none (manual validation).

- [ ] **Step 1: Rebuild the plugin in Release**

```
./build-with-vs.bat
```

Expected: no errors; `build/AISynth_artefacts/Release/VST3/AI Synth.vst3` is up to date.

- [ ] **Step 2: Check Reaper is closed (it locks the VST3 DLL)**

```
tasklist //FI "IMAGENAME eq reaper.exe"
```

Expected: `INFO: No tasks are running which match the specified criteria.` If Reaper is running, ask the user to close it — don't kill it.

- [ ] **Step 3: Deploy**

Run from PowerShell (not bash):

```
./deploy.ps1
```

Expected: new `AI Synth vN.vst3` copied into `C:\Program Files\Common Files\VST3\` with an incremented version number.

- [ ] **Step 4: Manual validation checklist**

Ask the user to open Reaper / their host and confirm:

- [ ] The Mod tab renders with 2 LFO panels in the top ~35 % and an 8-row matrix in the bottom ~65 %.
- [ ] Each row shows: SRC dropdown, DST dropdown, bipolar amount bar (centred), CURVE pill-row.
- [ ] Clicking a source dropdown shows the 10 entries (None, LFO 1, LFO 2, Env Amp, Env Filt, Velocity, Modwheel, Aftertouch, Keytrack, Random).
- [ ] Clicking a destination dropdown shows the 13 entries (None + 12 curated targets).
- [ ] Dragging the amount bar left of centre fills with pink (`modNegative`); right of centre fills with violet (`modPositive`). Double-click resets to 0.
- [ ] Selecting `exp` in a row's curve pill highlights it with the accent colour.
- [ ] Saving and reloading the host project persists all slot values (proof that APVTS registration is intact).
- [ ] Switching to Sound / FX / Arp tabs and back leaves matrix values unchanged.

- [ ] **Step 5: Take a screenshot of the rendered Mod tab and confirm with the user**

Compare against `docs/ClaudeDesign/AISynth V1 Hi-Fi.html` Modulation section. If structural divergence exists, treat it as a design brief for a follow-up iteration — do NOT bury as polish.

---

## What this plan explicitly does NOT ship

- **ModEngine.cpp**: no runtime evaluation of the matrix. Slots are persisted but nothing modulates the audio yet.
- **`mod.9..16.*` UI**: declared at param level only; no UI rows rendered for them.
- **Source colour-coding**: `ModSourcePicker` currently uses one neutral combo style. Per-family accent colour (lfo=violet, env=green, vel=gold) is deferred to a follow-up polish pass.
- **Hierarchical destination menu**: `ModDestPicker` uses a flat 13-entry combo rather than the hierarchical `Osc 1 ▸ cutoff` popup from the mock. Upgrade this when the destination count grows toward 30.
- **Drag-reorder of rows**: not in scope; slot index is fixed.
- **Scroll / fade states**: with exactly 8 visible rows fitting the viewport, no scroll needed. Revisit when expanding to 16.

---

## Self-review

**Spec coverage:** Each Phase-3 ModMatrix atom listed in `docs/ClaudeDesign/docs/components.md` rows 59–64 is implemented in a task (5 atoms + the table + the row composer = Tasks 4–9). `kDestinations[]` from the handover is Task 2. `mod.N.*` params with reserved slots 9..16 per the roadmap is Task 1 + Task 3. The three user decisions from brainstorming — keep LFO panels (Task 10 preserves them), declare `mod.1..16.*` (Tasks 1 + 3), curated 12 destinations (Task 2) — are each covered.

**Placeholder scan:** no TBD/TODO/etc. Every step has runnable code or an exact command.

**Type consistency:** `ModRow` constructor signature `(apvts, srcId, dstId, amountId, curveId)` is used consistently in Task 8 (definition) and Task 9 (instantiation via `ModMatrixTable`). `ModMatrixTable::getNumRows()` returns `int` everywhere. `getMatrix()` returns `SynthVibe::ModMatrixTable*` — matches the field type. `kDestinations` is an `std::array<ModDestination, 13>` everywhere. Size 13 (= 1 sentinel + 12 targets) is consistent between Task 2 (definition) and Task 6 (test assertion).

**Out-of-scope items properly declared:** the "What this plan explicitly does NOT ship" section flags engine work, colour-coding, hierarchy, drag-reorder, and scroll as deferred — preventing scope creep.

---

## Execution Handoff

Plan complete and saved to `docs/superpowers/plans/2026-04-24-phase3-modmatrix.md`. Two execution options:

**1. Subagent-Driven (recommended)** — I dispatch a fresh subagent per task, review between tasks, fast iteration.

**2. Inline Execution** — Execute tasks in this session using executing-plans, batch execution with checkpoints.

**Which approach?**
