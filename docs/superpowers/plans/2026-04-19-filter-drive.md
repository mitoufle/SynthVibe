# Filter Drive / Saturation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a post-FX saturation unit (Soft/Hard/Fold) inserted between Chorus and Reverb in FXChain, exposed on the FX tab.

**Architecture:** New `Drive` class follows the Delay/Chorus/Reverb pattern (prepare/setParams/process/reset). `FXChain` inserts it between Chorus and Reverb. Three new APVTS parameters. `FxTab` gains a fourth equal-width panel with a ComboBox (type) and two knobs (Drive, Mix).

**Tech Stack:** C++17, JUCE 7.0.9, APVTS, CMake / Visual Studio 2022. Build: `build.bat` from `AISynth/`. Output: `build\AISynth_artefacts\Release\VST3\AI Synth.vst3`.

---

## File Map

| Action | File | Responsibility |
|---|---|---|
| Modify | `Source/Parameters/ParameterIDs.h` | 3 new ID constants |
| Modify | `Source/Parameters/ParameterLayout.cpp` | Register 3 new APVTS params |
| Create | `Source/FX/Drive.h` | Class declaration with Type enum + Params struct |
| Create | `Source/FX/Drive.cpp` | Soft/Hard/Fold sample-level algorithms + parallel mix |
| Modify | `CMakeLists.txt` | Add `Drive.cpp` to source list |
| Modify | `Source/FX/FXChain.h` | Add `Drive` member, extend `setParams` signature |
| Modify | `Source/FX/FXChain.cpp` | Insert `drive` between chorus and reverb |
| Modify | `Source/PluginProcessor.h` | Add `buildDriveParams()` declaration |
| Modify | `Source/PluginProcessor.cpp` | Implement `buildDriveParams()`, update `processBlock` call |
| Modify | `Source/UI/FXTab.h` | Add DRIVE panel (ComboBox + 2 knobs), 4-column layout |

---

## Task 1: Add parameter ID constants

**Files:**
- Modify: `Source/Parameters/ParameterIDs.h`

- [ ] **Step 1: Add 3 new IDs after the Reverb block**

In `Source/Parameters/ParameterIDs.h`, add inside the `ParamIDs` namespace, after the `reverbMix` line and before the `// Unison` comment:

```cpp
    // Drive
    inline constexpr const char* driveType   = "drive_type";
    inline constexpr const char* driveAmount = "drive_amount";
    inline constexpr const char* driveMix    = "drive_mix";
```

- [ ] **Step 2: Commit**

```bash
git add Source/Parameters/ParameterIDs.h
git commit -m "feat: add Drive parameter ID constants"
```

---

## Task 2: Register APVTS parameters

**Files:**
- Modify: `Source/Parameters/ParameterLayout.cpp`

- [ ] **Step 1: Add Drive params after the Reverb block**

In `Source/Parameters/ParameterLayout.cpp`, add after the `reverbMix` `push_back` and before the `// Unison` comment:

```cpp
    // -----------------------------------------------------------------------
    // Drive
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParamIDs::driveType, "Drive Type",
        StringArray { "Soft", "Hard", "Fold" }, 0));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::driveAmount, "Drive Amount",
        NormalisableRange<float>(0.f, 24.f, 0.1f), 0.f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::driveMix, "Drive Mix",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 0.f));
```

- [ ] **Step 2: Build to verify no compile errors**

```bat
build.bat
```
Expected: build completes with no errors.

- [ ] **Step 3: Commit**

```bash
git add Source/Parameters/ParameterLayout.cpp
git commit -m "feat: register Drive APVTS parameters"
```

---

## Task 3: Create Drive class

**Files:**
- Create: `Source/FX/Drive.h`
- Create: `Source/FX/Drive.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write `Drive.h`**

Create `Source/FX/Drive.h`:

```cpp
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

class Drive
{
public:
    enum class Type { Soft = 0, Hard, Fold };

    struct Params
    {
        Type  type    = Type::Soft;
        float driveDb = 0.f;   // 0–24 dB pre-gain
        float mix     = 0.f;   // 0 = dry, 1 = wet
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

- [ ] **Step 2: Write `Drive.cpp`**

Create `Source/FX/Drive.cpp`:

```cpp
#include "Drive.h"
#include <cmath>

void Drive::prepare(double /*sampleRate*/, int /*maxBlockSize*/) {}
void Drive::setParams(const Params& p) { params = p; }
void Drive::reset() {}

void Drive::process(juce::AudioBuffer<float>& buffer)
{
    if (params.mix < 0.001f)
        return;

    const int   numChannels = buffer.getNumChannels();
    const int   numSamples  = buffer.getNumSamples();
    const float gain        = std::pow(10.f, params.driveDb / 20.f);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            const float dry = data[i];
            const float wet = processSample(dry, params.type, gain);
            data[i] = dry * (1.f - params.mix) + wet * params.mix;
        }
    }
}

float Drive::processSample(float x, Type type, float gain) noexcept
{
    switch (type)
    {
        case Type::Soft:
            return std::tanh(gain * x);

        case Type::Hard:
            return juce::jlimit(-1.f, 1.f, gain * x);

        case Type::Fold:
        {
            float y = gain * x;
            while (y > 1.f)  y = 2.f - y;
            while (y < -1.f) y = -2.f - y;
            return y;
        }
    }
    return x;
}
```

- [ ] **Step 3: Register `Drive.cpp` in `CMakeLists.txt`**

In `CMakeLists.txt`, add `Source/FX/Drive.cpp` right after the `Source/FX/Chorus.cpp` line:

```cmake
    Source/FX/Chorus.cpp
    Source/FX/Drive.cpp
    Source/FX/Reverb.cpp
```

- [ ] **Step 4: Build to verify**

```bat
build.bat
```
Expected: build completes with no errors.

- [ ] **Step 5: Commit**

```bash
git add Source/FX/Drive.h Source/FX/Drive.cpp CMakeLists.txt
git commit -m "feat: add Drive class (Soft/Hard/Fold saturation)"
```

---

## Task 4: Extend FXChain

**Files:**
- Modify: `Source/FX/FXChain.h`
- Modify: `Source/FX/FXChain.cpp`

- [ ] **Step 1: Update `FXChain.h`**

Replace the entire content of `Source/FX/FXChain.h`:

```cpp
#pragma once
#include "Delay.h"
#include "Chorus.h"
#include "Drive.h"
#include "Reverb.h"

class FXChain
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void setParams(const Delay::Params& dp,
                   const Chorus::Params& cp,
                   const Drive::Params& drp,
                   const Reverb::Params& rp);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    Delay  delay;
    Chorus chorus;
    Drive  drive;
    Reverb reverb;
};
```

- [ ] **Step 2: Update `FXChain.cpp`**

Replace the entire content of `Source/FX/FXChain.cpp`:

```cpp
#include "FXChain.h"

void FXChain::prepare(double sr, int maxBlockSize)
{
    delay.prepare(sr, maxBlockSize);
    chorus.prepare(sr, maxBlockSize);
    drive.prepare(sr, maxBlockSize);
    reverb.prepare(sr, maxBlockSize);
}

void FXChain::setParams(const Delay::Params& dp,
                        const Chorus::Params& cp,
                        const Drive::Params& drp,
                        const Reverb::Params& rp)
{
    delay.setParams(dp);
    chorus.setParams(cp);
    drive.setParams(drp);
    reverb.setParams(rp);
}

void FXChain::process(juce::AudioBuffer<float>& buffer)
{
    delay.process(buffer);
    chorus.process(buffer);
    drive.process(buffer);
    reverb.process(buffer);
}

void FXChain::reset()
{
    delay.reset();
    chorus.reset();
    drive.reset();
    reverb.reset();
}
```

- [ ] **Step 3: Stage but do NOT build yet**

The `fxChain.setParams(...)` call in `PluginProcessor.cpp` still passes 3 arguments; the build will fail until Task 5 updates that call site. Stage and proceed directly to Task 5.

```bash
git add Source/FX/FXChain.h Source/FX/FXChain.cpp
```

---

## Task 5: Wire Drive into PluginProcessor

**Files:**
- Modify: `Source/PluginProcessor.h`
- Modify: `Source/PluginProcessor.cpp`

- [ ] **Step 1: Add `buildDriveParams()` declaration to `PluginProcessor.h`**

In `Source/PluginProcessor.h`, in the `private:` section, add after `buildChorusParams()`:

```cpp
    VoiceParams       buildVoiceParams()  const;
    Delay::Params     buildDelayParams()  const;
    Chorus::Params    buildChorusParams() const;
    Drive::Params     buildDriveParams()  const;
    Reverb::Params    buildReverbParams() const;
    ArpEngine::Params buildArpParams()    const;
```

- [ ] **Step 2: Add `buildDriveParams()` implementation to `PluginProcessor.cpp`**

In `Source/PluginProcessor.cpp`, add the new function after `buildChorusParams()`:

```cpp
Drive::Params AISynthProcessor::buildDriveParams() const
{
    Drive::Params p;
    p.type    = static_cast<Drive::Type>(static_cast<int>(*apvts.getRawParameterValue(ParamIDs::driveType)));
    p.driveDb = *apvts.getRawParameterValue(ParamIDs::driveAmount);
    p.mix     = *apvts.getRawParameterValue(ParamIDs::driveMix);
    return p;
}
```

- [ ] **Step 3: Update `fxChain.setParams` call in `processBlock`**

In `PluginProcessor.cpp`, in `processBlock()`, replace:

```cpp
    fxChain.setParams(buildDelayParams(), buildChorusParams(), buildReverbParams());
```

With:

```cpp
    fxChain.setParams(buildDelayParams(), buildChorusParams(), buildDriveParams(), buildReverbParams());
```

- [ ] **Step 4: Build to verify (FXChain + PluginProcessor together)**

```bat
build.bat
```
Expected: build completes with no errors. VST3 at `build\AISynth_artefacts\Release\VST3\AI Synth.vst3`.

- [ ] **Step 5: Commit FXChain + PluginProcessor together**

```bash
git add Source/FX/FXChain.h Source/FX/FXChain.cpp Source/PluginProcessor.h Source/PluginProcessor.cpp
git commit -m "feat: extend FXChain with Drive, wire buildDriveParams into processBlock"
```

---

## Task 6: Add Drive UI panel in FXTab

**Files:**
- Modify: `Source/UI/FXTab.h`

- [ ] **Step 1: Replace the entire content of `Source/UI/FXTab.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "KnobWithLabel.h"
#include "LookAndFeel.h"
#include "../Parameters/ParameterIDs.h"

class FxTab : public juce::Component
{
public:
    explicit FxTab(juce::AudioProcessorValueTreeState& apvts) : apvts(apvts)
    {
        addAndMakeVisible(knobDelayTime);
        addAndMakeVisible(knobDelayFeedback);
        addAndMakeVisible(knobDelayMix);
        addAndMakeVisible(knobChorusRate);
        addAndMakeVisible(knobChorusDepth);
        addAndMakeVisible(knobChorusMix);

        driveTypeBox.addItemList({ "Soft", "Hard", "Fold" }, 1);
        driveTypeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, ParamIDs::driveType, driveTypeBox);
        addAndMakeVisible(driveTypeBox);
        addAndMakeVisible(knobDriveAmount);
        addAndMakeVisible(knobDriveMix);

        addAndMakeVisible(knobReverbRoom);
        addAndMakeVisible(knobReverbDamp);
        addAndMakeVisible(knobReverbMix);
    }

    void paint(juce::Graphics& g) override
    {
        drawPanel(g, delayBounds,  "DELAY",  SynthLookAndFeel::colFxAccent);
        drawPanel(g, chorusBounds, "CHORUS", SynthLookAndFeel::colFxAccent);
        drawPanel(g, driveBounds,  "DRIVE",  SynthLookAndFeel::colFxAccent);
        drawPanel(g, reverbBounds, "REVERB", SynthLookAndFeel::colFxAccent);
    }

    void resized() override
    {
        const int pad    = 8;
        const int titleH = 20;
        const int comboH = 28;
        auto area = getLocalBounds().reduced(pad);
        const int colW = area.getWidth() / 4;

        delayBounds  = area.removeFromLeft(colW).reduced(pad, 0);
        chorusBounds = area.removeFromLeft(colW).reduced(pad, 0);
        driveBounds  = area.removeFromLeft(colW).reduced(pad, 0);
        reverbBounds = area.reduced(pad, 0);

        auto layoutFx = [&](juce::Rectangle<int> bounds,
                            KnobWithLabel& k1, KnobWithLabel& k2, KnobWithLabel& k3)
        {
            auto b = bounds.withTrimmedTop(titleH);
            const int w = b.getWidth() / 3;
            k1.setBounds(b.removeFromLeft(w));
            k2.setBounds(b.removeFromLeft(w));
            k3.setBounds(b);
        };

        layoutFx(delayBounds,  knobDelayTime,   knobDelayFeedback, knobDelayMix);
        layoutFx(chorusBounds, knobChorusRate,  knobChorusDepth,   knobChorusMix);
        layoutFx(reverbBounds, knobReverbRoom,  knobReverbDamp,    knobReverbMix);

        // Drive panel: ComboBox (type) | Drive knob | Mix knob
        {
            auto b = driveBounds.withTrimmedTop(titleH);
            const int w = b.getWidth() / 3;
            auto typeCol = b.removeFromLeft(w);
            driveTypeBox.setBounds(typeCol.withSizeKeepingCentre(typeCol.getWidth() - 8, comboH));
            knobDriveAmount.setBounds(b.removeFromLeft(w));
            knobDriveMix.setBounds(b);
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::Rectangle<int> delayBounds, chorusBounds, driveBounds, reverbBounds;

    KnobWithLabel knobDelayTime     { "Time",     apvts, ParamIDs::delayTime,     " ms", 0 };
    KnobWithLabel knobDelayFeedback { "Feedback", apvts, ParamIDs::delayFeedback, "",    2 };
    KnobWithLabel knobDelayMix      { "Mix",      apvts, ParamIDs::delayMix,      "",    2 };
    KnobWithLabel knobChorusRate    { "Rate",     apvts, ParamIDs::chorusRate,    " Hz", 2 };
    KnobWithLabel knobChorusDepth   { "Depth",    apvts, ParamIDs::chorusDepth,   "",    3 };
    KnobWithLabel knobChorusMix     { "Mix",      apvts, ParamIDs::chorusMix,     "",    2 };

    juce::ComboBox driveTypeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> driveTypeAttach;
    KnobWithLabel knobDriveAmount   { "Drive",    apvts, ParamIDs::driveAmount,   " dB", 1 };
    KnobWithLabel knobDriveMix      { "Mix",      apvts, ParamIDs::driveMix,      "",    2 };

    KnobWithLabel knobReverbRoom    { "Room",     apvts, ParamIDs::reverbRoom,    "",    2 };
    KnobWithLabel knobReverbDamp    { "Damp",     apvts, ParamIDs::reverbDamp,    "",    2 };
    KnobWithLabel knobReverbMix     { "Mix",      apvts, ParamIDs::reverbMix,     "",    2 };

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

- [ ] **Step 2: Build to verify**

```bat
build.bat
```
Expected: build completes with no errors.

- [ ] **Step 3: Commit**

```bash
git add Source/UI/FXTab.h
git commit -m "feat: add DRIVE panel (Soft/Hard/Fold) to FXTab, 4-column layout"
```

---

## Task 7: Smoke test

- [ ] **Step 1: Deploy the plugin**

```bat
deploy.bat
```
Expected: `AI Synth.vst3` copied to `C:\Program Files\Common Files\VST3\`.

- [ ] **Step 2: Test in plugin host**

Load the plugin in a DAW or JUCE AudioPluginHost. Verify:

1. **Default (mix = 0)**: Sound unchanged — no Drive overhead.
2. **Soft, Drive 0 dB, Mix 1.0**: Sound identical to dry (tanh(x) ≈ x at low gain).
3. **Soft, Drive 12 dB, Mix 1.0**: Warm compression-like saturation, no harsh edges.
4. **Soft, Drive 24 dB, Mix 1.0**: Strong saturation, signal stays within ±1, no clipping artifacts.
5. **Hard, Drive 12 dB, Mix 1.0**: Harder, more rectangular waveshape — more aggressive than Soft.
6. **Fold, Drive 12 dB, Mix 1.0**: Metallic, FM-like character, harmonics above fundamental audible.
7. **Mix = 0.5**: Audible blend of dry and saturated signal (parallel saturation).
8. **No regression**: Delay, Chorus, Reverb still work. FX tab shows 4 panels: DELAY / CHORUS / DRIVE / REVERB.
