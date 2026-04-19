# Unison Engine Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a unison engine that stacks up to 7 detuned oscillator copies per voice, producing the dense supersaw sound characteristic of Serum/Massive/Sylenth1.

**Architecture:** A new `UnisonOscillator` class wraps up to 7 `Oscillator` instances and exposes the same interface as `Oscillator`. `Voice` replaces its two bare `Oscillator` members with two `UnisonOscillator` instances — no changes to the Voice/SynthEngine interface. Two global APVTS parameters (`unison_voices`, `unison_detune`) control voice count and spread.

**Tech Stack:** C++17, JUCE 7.0.9, APVTS, CMake / Visual Studio 2022.

---

## File Map

| Action | File | Responsibility |
|---|---|---|
| Create | `Source/Engine/UnisonOscillator.h` | Class declaration |
| Create | `Source/Engine/UnisonOscillator.cpp` | Implementation |
| Modify | `Source/Parameters/ParameterIDs.h` | Add 2 new ID constants |
| Modify | `Source/Parameters/ParameterLayout.cpp` | Register 2 new APVTS params |
| Modify | `Source/Engine/Voice.h` | Extend VoiceParams; swap Oscillator → UnisonOscillator |
| Modify | `Source/Engine/Voice.cpp` | Call setUnison() in setParams() |
| Modify | `Source/PluginProcessor.cpp` | Read new params in buildVoiceParams() |
| Modify | `Source/UI/SoundTab.h` | Add 2 knobs (Voices, Spread) |
| Modify | `CMakeLists.txt` | Add UnisonOscillator.cpp to source list |

---

## Task 1: Add parameter ID constants

**Files:**
- Modify: `Source/Parameters/ParameterIDs.h`

- [ ] **Step 1: Add the two new IDs after the Chorus block**

In `Source/Parameters/ParameterIDs.h`, add inside the `ParamIDs` namespace, after the `chorusMix` line and before the `arpEnabled` line:

```cpp
    // Unison
    inline constexpr const char* unisonVoices = "unison_voices";
    inline constexpr const char* unisonDetune = "unison_detune";
```

- [ ] **Step 2: Commit**

```bash
git add Source/Parameters/ParameterIDs.h
git commit -m "feat: add unison parameter ID constants"
```

---

## Task 2: Register APVTS parameters

**Files:**
- Modify: `Source/Parameters/ParameterLayout.cpp`

- [ ] **Step 1: Add unison params after the Chorus block**

In `Source/Parameters/ParameterLayout.cpp`, add after the `chorusMix` push_back and before the Arp block:

```cpp
    // -----------------------------------------------------------------------
    // Unison
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::unisonVoices, "Unison Voices", 1, 7, 1));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::unisonDetune, "Unison Spread",
        NormalisableRange<float>(0.f, 100.f, 0.1f), 0.f));
```

- [ ] **Step 2: Commit**

```bash
git add Source/Parameters/ParameterLayout.cpp
git commit -m "feat: register unison APVTS parameters"
```

---

## Task 3: Extend VoiceParams

**Files:**
- Modify: `Source/Engine/Voice.h`

- [ ] **Step 1: Add two fields to VoiceParams**

In `Source/Engine/Voice.h`, add the two new fields at the end of the `VoiceParams` struct, after `fltEnv`:

```cpp
    int   unisonVoices = 1;
    float unisonSpread = 0.f;
```

The complete struct bottom should read:

```cpp
    Envelope::Params ampEnv;
    Envelope::Params fltEnv;

    int   unisonVoices = 1;
    float unisonSpread = 0.f;
};
```

- [ ] **Step 2: Commit**

```bash
git add Source/Engine/Voice.h
git commit -m "feat: add unisonVoices/unisonSpread to VoiceParams"
```

---

## Task 4: Create UnisonOscillator

**Files:**
- Create: `Source/Engine/UnisonOscillator.h`
- Create: `Source/Engine/UnisonOscillator.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write UnisonOscillator.h**

Create `Source/Engine/UnisonOscillator.h`:

```cpp
#pragma once
#include "Oscillator.h"
#include <algorithm>

class UnisonOscillator
{
public:
    static constexpr int MaxUnison = 7;

    void setSampleRate(double sr);
    void setFrequency(double hz);
    void setWaveform(Waveform wf);
    void setDetuneCents(float baseCents);
    void setUnison(int voices, float spreadCents);
    void reset();
    float getNextSample();

private:
    Oscillator oscs[MaxUnison];
    int   unisonVoices = 1;
    float spreadCents  = 0.f;
    float baseCents    = 0.f;

    void recomputeDetune();
};
```

- [ ] **Step 2: Write UnisonOscillator.cpp**

Create `Source/Engine/UnisonOscillator.cpp`:

```cpp
#include "UnisonOscillator.h"
#include <cmath>

void UnisonOscillator::setSampleRate(double sr)
{
    for (auto& o : oscs)
        o.setSampleRate(sr);
}

void UnisonOscillator::setFrequency(double hz)
{
    for (int i = 0; i < unisonVoices; ++i)
        oscs[i].setFrequency(hz);
}

void UnisonOscillator::setWaveform(Waveform wf)
{
    for (auto& o : oscs)
        o.setWaveform(wf);
}

void UnisonOscillator::setDetuneCents(float base)
{
    baseCents = base;
    recomputeDetune();
}

void UnisonOscillator::setUnison(int voices, float spread)
{
    unisonVoices = std::clamp(voices, 1, MaxUnison);
    spreadCents  = spread;
    recomputeDetune();
}

void UnisonOscillator::reset()
{
    for (auto& o : oscs)
        o.reset();
}

float UnisonOscillator::getNextSample()
{
    float sum = 0.f;
    for (int i = 0; i < unisonVoices; ++i)
        sum += oscs[i].getNextSample();
    return sum / std::sqrt(static_cast<float>(unisonVoices));
}

void UnisonOscillator::recomputeDetune()
{
    for (int i = 0; i < unisonVoices; ++i)
    {
        const float offset = (unisonVoices > 1)
            ? spreadCents * (2.0f * i / static_cast<float>(unisonVoices - 1) - 1.0f)
            : 0.f;
        oscs[i].setDetuneCents(baseCents + offset);
    }
}
```

- [ ] **Step 3: Register UnisonOscillator.cpp in CMakeLists.txt**

In `CMakeLists.txt`, add `Source/Engine/UnisonOscillator.cpp` right after the `Source/Engine/Oscillator.cpp` line:

```cmake
    Source/Engine/Oscillator.cpp
    Source/Engine/UnisonOscillator.cpp
    Source/Engine/Envelope.cpp
```

- [ ] **Step 4: Commit**

```bash
git add Source/Engine/UnisonOscillator.h Source/Engine/UnisonOscillator.cpp CMakeLists.txt
git commit -m "feat: add UnisonOscillator class"
```

---

## Task 5: Swap Oscillator → UnisonOscillator in Voice

**Files:**
- Modify: `Source/Engine/Voice.h` (private members)
- Modify: `Source/Engine/Voice.cpp` (setParams)

- [ ] **Step 1: Update Voice.h — replace the two Oscillator members**

In `Source/Engine/Voice.h`, replace the include and the two member declarations. Change:

```cpp
#include "Oscillator.h"
```
To:
```cpp
#include "UnisonOscillator.h"
```

Then in the `private:` section, change:
```cpp
    Oscillator osc1;
    Oscillator osc2;
```
To:
```cpp
    UnisonOscillator osc1;
    UnisonOscillator osc2;
```

The `lfo1Osc` and `lfo2Osc` stay as bare `Oscillator` — LFOs don't need unison.

- [ ] **Step 2: Update Voice.cpp — call setUnison in setParams()**

In `Source/Engine/Voice.cpp`, in the `setParams()` function, add two `setUnison` calls after the existing `setDetuneCents` lines:

```cpp
    osc1.setWaveform(p.osc1.waveform);
    osc1.setDetuneCents(p.osc1.detune);
    osc2.setWaveform(p.osc2.waveform);
    osc2.setDetuneCents(p.osc2.detune);
    osc1.setUnison(p.unisonVoices, p.unisonSpread);   // add this
    osc2.setUnison(p.unisonVoices, p.unisonSpread);   // add this
```

All other calls to `osc1` and `osc2` in Voice.cpp (`setSampleRate`, `reset`, `setFrequency`, `getNextSample`, `setDetuneCents`) have the same signature on UnisonOscillator — no other changes needed.

- [ ] **Step 3: Build to verify**

Run from the `AISynth/` directory:
```bat
build.bat
```
Expected: build completes with no errors. The VST3 is at `build\AISynth_artefacts\Release\VST3\AI Synth.vst3`.

If you see a linker error about `UnisonOscillator`, check that CMakeLists.txt was saved with the new source line (Task 4, Step 3).

- [ ] **Step 4: Commit**

```bash
git add Source/Engine/Voice.h Source/Engine/Voice.cpp
git commit -m "feat: replace Oscillator with UnisonOscillator in Voice"
```

---

## Task 6: Wire new params into PluginProcessor

**Files:**
- Modify: `Source/PluginProcessor.cpp`

- [ ] **Step 1: Read unison params in buildVoiceParams()**

In `Source/PluginProcessor.cpp`, in the `buildVoiceParams()` function, add at the end before the `return p;` statement:

```cpp
    p.unisonVoices = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::unisonVoices));
    p.unisonSpread = *apvts.getRawParameterValue(ParamIDs::unisonDetune);
```

- [ ] **Step 2: Build to verify**

```bat
build.bat
```
Expected: build succeeds with no errors.

- [ ] **Step 3: Commit**

```bash
git add Source/PluginProcessor.cpp
git commit -m "feat: read unison params in buildVoiceParams"
```

---

## Task 7: Add UI knobs in SoundTab

**Files:**
- Modify: `Source/UI/SoundTab.h`

- [ ] **Step 1: Add two KnobWithLabel members**

In `Source/UI/SoundTab.h`, add after the `knobOsc1Level` declaration:

```cpp
    KnobWithLabel knobUnisonVoices { "Voices", apvts, ParamIDs::unisonVoices, "",    0 };
    KnobWithLabel knobUnisonSpread { "Spread", apvts, ParamIDs::unisonDetune,  " ct", 1 };
```

- [ ] **Step 2: Register them with addAndMakeVisible in the constructor**

In the constructor body, add after `addAndMakeVisible(knobOsc1Level)`:

```cpp
        addAndMakeVisible(knobUnisonVoices);
        addAndMakeVisible(knobUnisonSpread);
```

- [ ] **Step 3: Update resized() to lay out the unison knobs**

In `resized()`, find the OSC1 layout block:

```cpp
        // OSC 1
        osc1Bounds = col1;
        auto c1 = col1.withTrimmedTop(titleH);
        osc1WaveBox.setBounds(c1.removeFromTop(comboH));
        layoutKnobs(c1, { &knobOsc1Oct, &knobOsc1Semi, &knobOsc1Detune, &knobOsc1Level });
```

Replace the last `layoutKnobs` call with a two-row layout:

```cpp
        // OSC 1
        osc1Bounds = col1;
        auto c1 = col1.withTrimmedTop(titleH);
        osc1WaveBox.setBounds(c1.removeFromTop(comboH));
        auto oscRow = c1.removeFromTop(c1.getHeight() / 2);
        layoutKnobs(oscRow, { &knobOsc1Oct, &knobOsc1Semi, &knobOsc1Detune, &knobOsc1Level });
        layoutKnobs(c1, { &knobUnisonVoices, &knobUnisonSpread });
```

- [ ] **Step 4: Build to verify**

```bat
build.bat
```
Expected: build succeeds with no errors.

- [ ] **Step 5: Commit**

```bash
git add Source/UI/SoundTab.h
git commit -m "feat: add Voices and Spread knobs to SoundTab"
```

---

## Task 8: Smoke test

- [ ] **Step 1: Deploy the plugin**

Run `/deploy-vst` or:
```bat
deploy.bat
```
Expected: `AI Synth.vst3` copied to `C:\Program Files\Common Files\VST3\`.

- [ ] **Step 2: Test in plugin host**

Load the plugin in a DAW or in JUCE's AudioPluginHost. Verify:

1. **No regression at default settings** — UNISON=1, SPREAD=0 ct: sound is identical to before.
2. **Unison activates** — Set UNISON=7, SPREAD=30 ct: the sound becomes noticeably thicker/wider, like a supersaw.
3. **Volume is consistent** — Increasing voice count should not cause a volume spike. The sound should stay roughly the same loudness.
4. **Osc2 also benefits** — Raise Osc2 Level to 1.0, set different waveform: Osc2 also gains unison thickness.
5. **LFO detune still works** — With unison active, assign LFO1 to Detune: the modulation should animate the entire unison spread.
