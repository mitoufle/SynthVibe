# UI Redesign Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Refactor the SynthVibe plugin UI from a flat 4-row layout to a tabbed layout at 1200×750 with a persistent top bar (preset nav + master vol) and bottom bar (voice count).

**Architecture:** Split the monolithic `PluginEditor` into focused sub-components: one per tab (`SoundTab`, `ModTab`, `FxTab`, `ArpTab`) plus `TopBar` and `BottomBar`. Each component owns its JUCE APVTS attachments. `PluginEditor` becomes a thin coordinator that sizes and shows/hides components as the active tab changes.

**Tech Stack:** JUCE 7 · C++17 · APVTS · juce::TextButton for tab buttons · juce::Timer for bottom bar polling · Visual Studio 2022 / build.bat

---

## File Map

| File | Action | Responsibility |
|---|---|---|
| `Source/UI/KnobWithLabel.h` | **Create** | Extract reusable knob+label component from PluginEditor.h |
| `Source/UI/SoundTab.h` | **Create** | OSC1 · OSC2 · Filter · AmpEnv · FltEnv (3-column layout) |
| `Source/UI/ModTab.h` | **Create** | LFO1 · LFO2 side-by-side |
| `Source/UI/FxTab.h` | **Create** | Delay · Chorus side-by-side |
| `Source/UI/ArpTab.h` | **Create** | Arp controls |
| `Source/UI/TopBar.h` | **Create** | Preset nav (◀ name ▶ · Save · Load) + master volume knob |
| `Source/UI/BottomBar.h` | **Create** | Active voice count + CPU label via juce::Timer |
| `Source/Engine/SynthEngine.h` | **Modify** | Add `getActiveVoiceCount()` public method |
| `Source/PluginProcessor.h` | **Modify** | Add public `PresetManager presetManager` + `getActiveVoiceCount()` |
| `Source/UI/LookAndFeel.h` | **Modify** | Add tab-button drawing via `drawButtonBackground` override |
| `Source/PluginEditor.h` | **Modify** | Replace 30+ individual members with 6 sub-components + 4 tab buttons |
| `Source/PluginEditor.cpp` | **Modify** | New `resized()` layout + tab switching logic; delete old layout code |

---

## Task 1: Extract KnobWithLabel into its own header

**Files:**
- Create: `Source/UI/KnobWithLabel.h`
- Modify: `Source/PluginEditor.h` (replace inline struct with `#include`)

- [ ] **Step 1: Create `Source/UI/KnobWithLabel.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "LookAndFeel.h"

struct KnobWithLabel : public juce::Component
{
    juce::Slider slider { juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Label  nameLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    KnobWithLabel(const juce::String& name,
                  juce::AudioProcessorValueTreeState& apvts,
                  const juce::String& paramID,
                  const juce::String& valueSuffix   = "",
                  int                 decimalPlaces = 2)
    {
        nameLabel.setText(name, juce::dontSendNotification);
        nameLabel.setJustificationType(juce::Justification::centred);
        nameLabel.setFont(juce::Font(11.f, juce::Font::bold));
        nameLabel.setColour(juce::Label::textColourId, juce::Colour(SynthLookAndFeel::colTextDim));
        addAndMakeVisible(nameLabel);
        slider.setTextValueSuffix(valueSuffix);
        slider.setNumDecimalPlacesToDisplay(decimalPlaces);
        slider.setTooltip("Double-click or Ctrl+click to enter a value");
        addAndMakeVisible(slider);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, paramID, slider);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        nameLabel.setBounds(b.removeFromTop(14));
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, b.getWidth(), 16);
        slider.setBounds(b);
    }
};
```

- [ ] **Step 2: Replace the inline struct in `Source/PluginEditor.h`**

Remove the `KnobWithLabel` struct from `PluginEditor.h` and add this include at the top (after `#include "UI/LookAndFeel.h"`):

```cpp
#include "UI/KnobWithLabel.h"
```

- [ ] **Step 3: Build to verify no regressions**

```bat
build.bat
```

Expected: build succeeds, no compiler errors.

- [ ] **Step 4: Commit**

```bash
git add Source/UI/KnobWithLabel.h Source/PluginEditor.h
git commit -m "refactor: extract KnobWithLabel into Source/UI/KnobWithLabel.h"
```

---

## Task 2: Add `getActiveVoiceCount()` to SynthEngine and AISynthProcessor

**Files:**
- Modify: `Source/Engine/SynthEngine.h`
- Modify: `Source/PluginProcessor.h`

- [ ] **Step 1: Add method to `Source/Engine/SynthEngine.h`**

Add this public method inside the `SynthEngine` class (after `processBlock`):

```cpp
int getActiveVoiceCount() const noexcept
{
    int count = 0;
    for (const auto& v : voices)
        if (v.isActive()) ++count;
    return count;
}
```

- [ ] **Step 2: Expose via `AISynthProcessor`**

In `Source/PluginProcessor.h`, add `PresetManager` include and members. The full updated private/public section:

```cpp
// At the top of PluginProcessor.h, add:
#include "Presets/PresetManager.h"

// Inside the AISynthProcessor class, add these public members after `apvts`:
PresetManager presetManager { apvts };

int getActiveVoiceCount() const noexcept { return synth.getActiveVoiceCount(); }
```

- [ ] **Step 3: Build**

```bat
build.bat
```

Expected: build succeeds.

- [ ] **Step 4: Commit**

```bash
git add Source/Engine/SynthEngine.h Source/PluginProcessor.h
git commit -m "feat: expose getActiveVoiceCount() + PresetManager on processor"
```

---

## Task 3: Add tab-button styling to LookAndFeel

**Files:**
- Modify: `Source/UI/LookAndFeel.h`

- [ ] **Step 1: Override `drawButtonBackground` in `SynthLookAndFeel`**

Add this method inside the `SynthLookAndFeel` class (after the constructor):

```cpp
void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                          const juce::Colour&, bool, bool) override
{
    auto bounds = button.getLocalBounds().toFloat();
    if (button.getToggleState())
    {
        g.setColour(juce::Colour(colHighlight));
        g.fillRoundedRectangle(bounds.reduced(1.f), 4.f);
        g.setColour(juce::Colour(colHighlight).brighter(0.2f));
        g.drawRoundedRectangle(bounds.reduced(1.f), 4.f, 1.f);
    }
    else
    {
        g.setColour(juce::Colour(colAccent));
        g.fillRoundedRectangle(bounds.reduced(1.f), 4.f);
        g.setColour(juce::Colour(colKnobTrack));
        g.drawRoundedRectangle(bounds.reduced(1.f), 4.f, 1.f);
    }
}

void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                    bool, bool) override
{
    g.setFont(juce::Font(11.f, juce::Font::bold));
    g.setColour(button.getToggleState()
                    ? juce::Colours::white
                    : juce::Colour(colTextDim));
    g.drawText(button.getButtonText(), button.getLocalBounds(),
               juce::Justification::centred);
}
```

- [ ] **Step 2: Build**

```bat
build.bat
```

Expected: build succeeds.

- [ ] **Step 3: Commit**

```bash
git add Source/UI/LookAndFeel.h
git commit -m "feat: add tab-button drawing to SynthLookAndFeel"
```

---

## Task 4: Create SoundTab

**Files:**
- Create: `Source/UI/SoundTab.h`

- [ ] **Step 1: Create `Source/UI/SoundTab.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "KnobWithLabel.h"
#include "SynthSection.h"
#include "LookAndFeel.h"

class SoundTab : public juce::Component
{
public:
    explicit SoundTab(juce::AudioProcessorValueTreeState& apvts)
        : apvts(apvts)
    {
        osc1WaveBox.addItemList({ "Sine", "Saw", "Square", "Triangle" }, 1);
        osc1WaveAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, "osc1_waveform", osc1WaveBox);

        osc2WaveBox.addItemList({ "Sine", "Saw", "Square", "Triangle" }, 1);
        osc2WaveAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, "osc2_waveform", osc2WaveBox);

        filterTypeBox.addItemList({ "Low Pass", "High Pass", "Band Pass" }, 1);
        filterTypeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, "filter_type", filterTypeBox);

        for (auto* c : { &osc1WaveBox, &osc2WaveBox, &filterTypeBox })
            addAndMakeVisible(c);

        for (auto* k : allKnobs())
            addAndMakeVisible(k);
    }

    void paint(juce::Graphics& g) override
    {
        // OSC 1 panel
        drawPanel(g, osc1Bounds, "OSC 1", SynthLookAndFeel::colOscAccent);
        drawPanel(g, osc2Bounds, "OSC 2", SynthLookAndFeel::colOscAccent);
        drawPanel(g, filterBounds, "FILTER", SynthLookAndFeel::colFilterAccent);
        drawPanel(g, ampEnvBounds, "AMP ENV", SynthLookAndFeel::colEnvAccent);
        drawPanel(g, fltEnvBounds, "FLT ENV", SynthLookAndFeel::colEnvAccent);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(6);
        const int pad = 6;
        const int comboH = 26;
        const int titleH = 20;

        // 3 columns
        const int col3W = area.getWidth() / 3;
        auto col1 = area.removeFromLeft(col3W).reduced(pad, 0);
        auto col2 = area.removeFromLeft(col3W).reduced(pad, 0);
        auto col3 = area.reduced(pad, 0);

        // OSC 1
        osc1Bounds = col1;
        auto c1 = col1.withTrimmedTop(titleH);
        osc1WaveBox.setBounds(c1.removeFromTop(comboH));
        layoutKnobs(c1, { &knobOsc1Oct, &knobOsc1Semi, &knobOsc1Detune, &knobOsc1Level });

        // OSC 2
        osc2Bounds = col2;
        auto c2 = col2.withTrimmedTop(titleH);
        osc2WaveBox.setBounds(c2.removeFromTop(comboH));
        layoutKnobs(c2, { &knobOsc2Oct, &knobOsc2Semi, &knobOsc2Detune, &knobOsc2Level });

        // Col 3: Filter | AmpEnv | FltEnv stacked
        const int filterH = col3.getHeight() * 35 / 100;
        const int envH    = (col3.getHeight() - filterH) / 2;

        filterBounds = col3.removeFromTop(filterH);
        auto fc = filterBounds.withTrimmedTop(titleH);
        filterTypeBox.setBounds(fc.removeFromTop(comboH));
        layoutKnobs(fc, { &knobCutoff, &knobResonance, &knobFilterEnv });

        ampEnvBounds = col3.removeFromTop(envH);
        layoutKnobs(ampEnvBounds.withTrimmedTop(titleH), { &knobAmpA, &knobAmpD, &knobAmpS, &knobAmpR });

        fltEnvBounds = col3;
        layoutKnobs(fltEnvBounds.withTrimmedTop(titleH), { &knobFltA, &knobFltD, &knobFltS, &knobFltR });
    }

private:
    juce::AudioProcessorValueTreeState& apvts;

    juce::Rectangle<int> osc1Bounds, osc2Bounds, filterBounds, ampEnvBounds, fltEnvBounds;

    juce::ComboBox osc1WaveBox, osc2WaveBox, filterTypeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        osc1WaveAttach, osc2WaveAttach, filterTypeAttach;

    KnobWithLabel knobOsc1Oct    { "Oct",    apvts, "osc1_octave",   "",    0 };
    KnobWithLabel knobOsc1Semi   { "Semi",   apvts, "osc1_semitone", "",    0 };
    KnobWithLabel knobOsc1Detune { "Detune", apvts, "osc1_detune",   " ct", 1 };
    KnobWithLabel knobOsc1Level  { "Level",  apvts, "osc1_level",    "",    2 };

    KnobWithLabel knobOsc2Oct    { "Oct",    apvts, "osc2_octave",   "",    0 };
    KnobWithLabel knobOsc2Semi   { "Semi",   apvts, "osc2_semitone", "",    0 };
    KnobWithLabel knobOsc2Detune { "Detune", apvts, "osc2_detune",   " ct", 1 };
    KnobWithLabel knobOsc2Level  { "Level",  apvts, "osc2_level",    "",    2 };

    KnobWithLabel knobCutoff    { "Cutoff",  apvts, "filter_cutoff",    " Hz", 0 };
    KnobWithLabel knobResonance { "Res",     apvts, "filter_resonance", "",    2 };
    KnobWithLabel knobFilterEnv { "Env Amt", apvts, "filter_env_amt",   "",    2 };

    KnobWithLabel knobAmpA { "A", apvts, "amp_attack",  " s", 3 };
    KnobWithLabel knobAmpD { "D", apvts, "amp_decay",   " s", 3 };
    KnobWithLabel knobAmpS { "S", apvts, "amp_sustain", "",   2 };
    KnobWithLabel knobAmpR { "R", apvts, "amp_release", " s", 3 };

    KnobWithLabel knobFltA { "A", apvts, "flt_attack",  " s", 3 };
    KnobWithLabel knobFltD { "D", apvts, "flt_decay",   " s", 3 };
    KnobWithLabel knobFltS { "S", apvts, "flt_sustain", "",   2 };
    KnobWithLabel knobFltR { "R", apvts, "flt_release", " s", 3 };

    std::initializer_list<juce::Component*> allKnobs()
    {
        return { &knobOsc1Oct, &knobOsc1Semi, &knobOsc1Detune, &knobOsc1Level,
                 &knobOsc2Oct, &knobOsc2Semi, &knobOsc2Detune, &knobOsc2Level,
                 &knobCutoff, &knobResonance, &knobFilterEnv,
                 &knobAmpA, &knobAmpD, &knobAmpS, &knobAmpR,
                 &knobFltA, &knobFltD, &knobFltS, &knobFltR };
    }

    static void drawPanel(juce::Graphics& g, juce::Rectangle<int> bounds,
                          const juce::String& title, juce::uint32 accentColour)
    {
        auto f = bounds.toFloat().reduced(2.f);
        g.setColour(juce::Colour(SynthLookAndFeel::colPanel));
        g.fillRoundedRectangle(f, 6.f);
        g.setColour(juce::Colour(accentColour));
        g.drawRoundedRectangle(f, 6.f, 1.f);
        g.setFont(juce::Font(11.f, juce::Font::bold));
        g.drawText(title.toUpperCase(), bounds.getX() + 10, bounds.getY() + 4,
                   bounds.getWidth() - 20, 16, juce::Justification::centredLeft);
    }

    static void layoutKnobs(juce::Rectangle<int> bounds,
                            std::initializer_list<juce::Component*> knobs)
    {
        if (knobs.size() == 0) return;
        const int w = bounds.getWidth() / static_cast<int>(knobs.size());
        auto b = bounds;
        for (auto* k : knobs)
            k->setBounds(b.removeFromLeft(w));
    }
};
```

- [ ] **Step 2: Build**

```bat
build.bat
```

Expected: build succeeds.

- [ ] **Step 3: Commit**

```bash
git add Source/UI/SoundTab.h
git commit -m "feat: add SoundTab component (OSC1/OSC2/Filter/Envs 3-col layout)"
```

---

## Task 5: Create ModTab

**Files:**
- Create: `Source/UI/ModTab.h`

- [ ] **Step 1: Create `Source/UI/ModTab.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "KnobWithLabel.h"
#include "LookAndFeel.h"

class ModTab : public juce::Component
{
public:
    explicit ModTab(juce::AudioProcessorValueTreeState& apvts) : apvts(apvts)
    {
        lfo1ShapeBox.addItemList({ "Sine", "Saw", "Square", "Triangle" }, 1);
        lfo1ShapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, "lfo1_shape", lfo1ShapeBox);
        lfo1DestBox.addItemList({ "Pitch", "Filter", "Amp", "Detune" }, 1);
        lfo1DestAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, "lfo1_dest", lfo1DestBox);

        lfo2ShapeBox.addItemList({ "Sine", "Saw", "Square", "Triangle" }, 1);
        lfo2ShapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, "lfo2_shape", lfo2ShapeBox);
        lfo2DestBox.addItemList({ "Pitch", "Filter", "Amp", "Detune" }, 1);
        lfo2DestAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, "lfo2_dest", lfo2DestBox);

        for (auto* c : { &lfo1ShapeBox, &lfo1DestBox, &lfo2ShapeBox, &lfo2DestBox })
            addAndMakeVisible(c);
        for (auto* k : { (juce::Component*)&knobLfo1Rate, &knobLfo1Depth,
                         &knobLfo2Rate, &knobLfo2Depth })
            addAndMakeVisible(k);
    }

    void paint(juce::Graphics& g) override
    {
        drawPanel(g, lfo1Bounds, "LFO 1", SynthLookAndFeel::colLfoAccent);
        drawPanel(g, lfo2Bounds, "LFO 2", SynthLookAndFeel::colLfoAccent);
    }

    void resized() override
    {
        const int pad    = 8;
        const int comboH = 26;
        const int titleH = 20;

        auto area = getLocalBounds().reduced(pad);
        lfo1Bounds = area.removeFromLeft(area.getWidth() / 2).reduced(pad, 0);
        lfo2Bounds = area.reduced(pad, 0);

        auto layoutLfo = [&](juce::Rectangle<int> bounds,
                             juce::ComboBox& shapeBox, juce::ComboBox& destBox,
                             KnobWithLabel& rate, KnobWithLabel& depth)
        {
            auto b = bounds.withTrimmedTop(titleH);
            shapeBox.setBounds(b.removeFromTop(comboH));
            destBox.setBounds (b.removeFromTop(comboH));
            const int knobW = b.getWidth() / 2;
            rate .setBounds(b.removeFromLeft(knobW));
            depth.setBounds(b);
        };

        layoutLfo(lfo1Bounds, lfo1ShapeBox, lfo1DestBox, knobLfo1Rate, knobLfo1Depth);
        layoutLfo(lfo2Bounds, lfo2ShapeBox, lfo2DestBox, knobLfo2Rate, knobLfo2Depth);
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::Rectangle<int> lfo1Bounds, lfo2Bounds;

    juce::ComboBox lfo1ShapeBox, lfo1DestBox, lfo2ShapeBox, lfo2DestBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        lfo1ShapeAttach, lfo1DestAttach, lfo2ShapeAttach, lfo2DestAttach;

    KnobWithLabel knobLfo1Rate  { "Rate",  apvts, "lfo1_rate",  " Hz", 2 };
    KnobWithLabel knobLfo1Depth { "Depth", apvts, "lfo1_depth", "",    2 };
    KnobWithLabel knobLfo2Rate  { "Rate",  apvts, "lfo2_rate",  " Hz", 2 };
    KnobWithLabel knobLfo2Depth { "Depth", apvts, "lfo2_depth", "",    2 };

    static void drawPanel(juce::Graphics& g, juce::Rectangle<int> bounds,
                          const juce::String& title, juce::uint32 accentColour)
    {
        auto f = bounds.toFloat().reduced(2.f);
        g.setColour(juce::Colour(SynthLookAndFeel::colPanel));
        g.fillRoundedRectangle(f, 6.f);
        g.setColour(juce::Colour(accentColour));
        g.drawRoundedRectangle(f, 6.f, 1.f);
        g.setFont(juce::Font(11.f, juce::Font::bold));
        g.drawText(title.toUpperCase(), bounds.getX() + 10, bounds.getY() + 4,
                   bounds.getWidth() - 20, 16, juce::Justification::centredLeft);
    }
};
```

- [ ] **Step 2: Build**

```bat
build.bat
```

- [ ] **Step 3: Commit**

```bash
git add Source/UI/ModTab.h
git commit -m "feat: add ModTab component (LFO1 + LFO2)"
```

---

## Task 6: Create FxTab

**Files:**
- Create: `Source/UI/FxTab.h`

- [ ] **Step 1: Create `Source/UI/FxTab.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "KnobWithLabel.h"
#include "LookAndFeel.h"

class FxTab : public juce::Component
{
public:
    explicit FxTab(juce::AudioProcessorValueTreeState& apvts) : apvts(apvts)
    {
        for (auto* k : { (juce::Component*)&knobDelayTime, &knobDelayFeedback, &knobDelayMix,
                         &knobChorusRate, &knobChorusDepth, &knobChorusMix })
            addAndMakeVisible(k);
    }

    void paint(juce::Graphics& g) override
    {
        drawPanel(g, delayBounds,  "DELAY",  SynthLookAndFeel::colFxAccent);
        drawPanel(g, chorusBounds, "CHORUS", SynthLookAndFeel::colFxAccent);
    }

    void resized() override
    {
        const int pad    = 8;
        const int titleH = 20;
        auto area = getLocalBounds().reduced(pad);
        delayBounds  = area.removeFromLeft(area.getWidth() / 2).reduced(pad, 0);
        chorusBounds = area.reduced(pad, 0);

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
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::Rectangle<int> delayBounds, chorusBounds;

    KnobWithLabel knobDelayTime     { "Time",     apvts, "delay_time",     " ms", 0 };
    KnobWithLabel knobDelayFeedback { "Feedback", apvts, "delay_feedback", "",    2 };
    KnobWithLabel knobDelayMix      { "Mix",      apvts, "delay_mix",      "",    2 };
    KnobWithLabel knobChorusRate    { "Rate",     apvts, "chorus_rate",    " Hz", 2 };
    KnobWithLabel knobChorusDepth   { "Depth",    apvts, "chorus_depth",   "",    3 };
    KnobWithLabel knobChorusMix     { "Mix",      apvts, "chorus_mix",     "",    2 };

    static void drawPanel(juce::Graphics& g, juce::Rectangle<int> bounds,
                          const juce::String& title, juce::uint32 accentColour)
    {
        auto f = bounds.toFloat().reduced(2.f);
        g.setColour(juce::Colour(SynthLookAndFeel::colPanel));
        g.fillRoundedRectangle(f, 6.f);
        g.setColour(juce::Colour(accentColour));
        g.drawRoundedRectangle(f, 6.f, 1.f);
        g.setFont(juce::Font(11.f, juce::Font::bold));
        g.drawText(title.toUpperCase(), bounds.getX() + 10, bounds.getY() + 4,
                   bounds.getWidth() - 20, 16, juce::Justification::centredLeft);
    }
};
```

- [ ] **Step 2: Build**

```bat
build.bat
```

- [ ] **Step 3: Commit**

```bash
git add Source/UI/FxTab.h
git commit -m "feat: add FxTab component (Delay + Chorus)"
```

---

## Task 7: Create ArpTab

**Files:**
- Create: `Source/UI/ArpTab.h`

- [ ] **Step 1: Create `Source/UI/ArpTab.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "KnobWithLabel.h"
#include "LookAndFeel.h"

class ArpTab : public juce::Component
{
public:
    explicit ArpTab(juce::AudioProcessorValueTreeState& apvts) : apvts(apvts)
    {
        arpOnButton.setButtonText("ARP ON");
        arpOnButton.setClickingTogglesState(true);
        arpOnAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, "arp_enabled", arpOnButton);

        arpModeBox.addItemList({ "Up", "Down", "UpDown", "Random" }, 1);
        arpModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, "arp_mode", arpModeBox);

        arpRateBox.addItemList({ "1/16", "1/8", "1/4", "1/2" }, 1);
        arpRateAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            apvts, "arp_rate", arpRateBox);

        addAndMakeVisible(arpOnButton);
        addAndMakeVisible(arpModeBox);
        addAndMakeVisible(arpRateBox);
        addAndMakeVisible(knobArpOct);
    }

    void paint(juce::Graphics& g) override
    {
        drawPanel(g, panelBounds, "ARP", SynthLookAndFeel::colArpAccent);
    }

    void resized() override
    {
        const int pad    = 8;
        const int comboH = 28;
        const int btnH   = 32;

        // Centre a panel taking up 60% of width
        auto area = getLocalBounds().reduced(pad);
        const int panelW = juce::jmin(area.getWidth(), 500);
        panelBounds = area.withSizeKeepingCentre(panelW, area.getHeight());

        auto inner = panelBounds.reduced(12, 24); // leave room for title
        arpOnButton.setBounds(inner.removeFromTop(btnH));
        inner.removeFromTop(6);
        arpModeBox.setBounds(inner.removeFromTop(comboH));
        inner.removeFromTop(6);
        arpRateBox.setBounds(inner.removeFromTop(comboH));
        inner.removeFromTop(6);
        knobArpOct.setBounds(inner.removeFromTop(80).withSizeKeepingCentre(80, 80));
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::Rectangle<int> panelBounds;

    juce::ToggleButton arpOnButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> arpOnAttach;

    juce::ComboBox arpModeBox, arpRateBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        arpModeAttach, arpRateAttach;

    KnobWithLabel knobArpOct { "Octaves", apvts, "arp_octave_range", "", 0 };

    static void drawPanel(juce::Graphics& g, juce::Rectangle<int> bounds,
                          const juce::String& title, juce::uint32 accentColour)
    {
        auto f = bounds.toFloat().reduced(2.f);
        g.setColour(juce::Colour(SynthLookAndFeel::colPanel));
        g.fillRoundedRectangle(f, 6.f);
        g.setColour(juce::Colour(accentColour));
        g.drawRoundedRectangle(f, 6.f, 1.f);
        g.setFont(juce::Font(11.f, juce::Font::bold));
        g.drawText(title.toUpperCase(), bounds.getX() + 10, bounds.getY() + 4,
                   bounds.getWidth() - 20, 16, juce::Justification::centredLeft);
    }
};
```

- [ ] **Step 2: Build**

```bat
build.bat
```

- [ ] **Step 3: Commit**

```bash
git add Source/UI/ArpTab.h
git commit -m "feat: add ArpTab component"
```

---

## Task 8: Create TopBar

**Files:**
- Create: `Source/UI/TopBar.h`

- [ ] **Step 1: Create `Source/UI/TopBar.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "KnobWithLabel.h"
#include "LookAndFeel.h"
#include "../Presets/PresetManager.h"

class TopBar : public juce::Component
{
public:
    TopBar(juce::AudioProcessorValueTreeState& apvts, PresetManager& pm)
        : presetManager(pm)
    {
        logoLabel.setText("AI Synth", juce::dontSendNotification);
        logoLabel.setFont(juce::Font(14.f, juce::Font::bold));
        logoLabel.setColour(juce::Label::textColourId, juce::Colour(SynthLookAndFeel::colHighlight));
        addAndMakeVisible(logoLabel);

        presetNameLabel.setEditable(false);
        presetNameLabel.setColour(juce::Label::textColourId, juce::Colour(SynthLookAndFeel::colText));
        presetNameLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(presetNameLabel);

        prevButton.setButtonText("<");
        nextButton.setButtonText(">");
        saveButton.setButtonText("Save");
        loadButton.setButtonText("Load");

        prevButton.onClick = [this] { navigatePreset(-1); };
        nextButton.onClick = [this] { navigatePreset(+1); };
        saveButton.onClick = [this] { saveCurrentPreset(); };
        loadButton.onClick = [this] { loadCurrentPreset(); };

        for (auto* b : { &prevButton, &nextButton, &saveButton, &loadButton })
            addAndMakeVisible(b);

        addAndMakeVisible(knobMaster);

        refreshPresetLabel();
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colour(SynthLookAndFeel::colAccent));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(6, 4);

        logoLabel.setBounds(area.removeFromLeft(90));
        knobMaster.setBounds(area.removeFromRight(56));

        auto nav = area;
        prevButton.setBounds(nav.removeFromLeft(28));
        nextButton.setBounds(nav.removeFromRight(28));
        saveButton.setBounds(nav.removeFromRight(52));
        loadButton.setBounds(nav.removeFromRight(52));
        nav.removeFromRight(4);
        presetNameLabel.setBounds(nav);
    }

private:
    PresetManager& presetManager;
    int            currentPresetIndex = -1;

    juce::Label       logoLabel;
    juce::Label       presetNameLabel;
    juce::TextButton  prevButton, nextButton, saveButton, loadButton;
    KnobWithLabel     knobMaster { "Vol", *getApvts(), "master_volume", "", 2 };

    // Workaround: store apvts ptr set via constructor helper below
    juce::AudioProcessorValueTreeState* apvtsPtr = nullptr;
    juce::AudioProcessorValueTreeState* getApvts() { return apvtsPtr; }

    void refreshPresetLabel()
    {
        auto names = presetManager.getPresetNames();
        if (names.isEmpty())
        {
            presetNameLabel.setText("Init", juce::dontSendNotification);
            currentPresetIndex = -1;
        }
        else
        {
            currentPresetIndex = juce::jlimit(0, names.size() - 1, currentPresetIndex);
            presetNameLabel.setText(names[currentPresetIndex], juce::dontSendNotification);
        }
    }

    void navigatePreset(int delta)
    {
        auto names = presetManager.getPresetNames();
        if (names.isEmpty()) return;
        currentPresetIndex = (currentPresetIndex + delta + names.size()) % names.size();
        presetManager.loadPreset(names[currentPresetIndex]);
        refreshPresetLabel();
    }

    void saveCurrentPreset()
    {
        juce::AlertWindow::showInputBoxAsync(
            "Save Preset", "Enter preset name:", "New Preset", nullptr,
            juce::ModalCallbackFunction::create([this](int result, juce::String name)
            {
                if (result == 1 && name.isNotEmpty())
                {
                    presetManager.savePreset(name);
                    refreshPresetLabel();
                }
            }));
    }

    void loadCurrentPreset()
    {
        auto names = presetManager.getPresetNames();
        if (names.isEmpty()) return;
        presetManager.loadPreset(names[currentPresetIndex]);
    }
};
```

> **Note:** `KnobWithLabel` takes `apvts` by reference in its constructor but `TopBar` needs to receive `apvts` first before constructing the member. Fix: pass `apvts` to the constructor and store a pointer, constructing `knobMaster` via a helper or by making it a `std::unique_ptr` initialised in the constructor body. Replace the `knobMaster` member declaration and constructor body as follows:

```cpp
// Replace member declaration:
std::unique_ptr<KnobWithLabel> knobMaster;

// In constructor, after other setup:
knobMaster = std::make_unique<KnobWithLabel>("Vol", apvts, "master_volume", "", 2);
addAndMakeVisible(*knobMaster);

// In resized(), replace knobMaster.setBounds with:
if (knobMaster) knobMaster->setBounds(area.removeFromRight(56));
```

- [ ] **Step 2: Build**

```bat
build.bat
```

Expected: build succeeds. Fix any pointer/reference ordering issues if the compiler complains about `knobMaster` initialisation order.

- [ ] **Step 3: Commit**

```bash
git add Source/UI/TopBar.h
git commit -m "feat: add TopBar component with preset nav and master volume"
```

---

## Task 9: Create BottomBar

**Files:**
- Create: `Source/UI/BottomBar.h`

- [ ] **Step 1: Create `Source/UI/BottomBar.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "LookAndFeel.h"
#include "../PluginProcessor.h"

class BottomBar : public juce::Component,
                  private juce::Timer
{
public:
    explicit BottomBar(AISynthProcessor& p) : processor(p)
    {
        voiceLabel.setColour(juce::Label::textColourId, juce::Colour(SynthLookAndFeel::colTextDim));
        voiceLabel.setFont(juce::Font(10.f));
        addAndMakeVisible(voiceLabel);

        cpuLabel.setColour(juce::Label::textColourId, juce::Colour(SynthLookAndFeel::colTextDim));
        cpuLabel.setFont(juce::Font(10.f));
        cpuLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(cpuLabel);

        startTimerHz(10); // poll 10x per second
        updateLabels();
    }

    ~BottomBar() override { stopTimer(); }

    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colour(SynthLookAndFeel::colAccent));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(8, 2);
        voiceLabel.setBounds(area.removeFromLeft(area.getWidth() / 2));
        cpuLabel.setBounds(area);
    }

private:
    AISynthProcessor& processor;
    juce::Label voiceLabel, cpuLabel;

    void timerCallback() override { updateLabels(); }

    void updateLabels()
    {
        const int active = processor.getActiveVoiceCount();
        juce::String bar;
        for (int i = 0; i < 8; ++i)
            bar += (i < active ? juce::String(L"\u2588") : juce::String(L"\u2591"));
        voiceLabel.setText("Voices: " + bar + " " + juce::String(active) + "/8",
                           juce::dontSendNotification);
        cpuLabel.setText("CPU: --", juce::dontSendNotification);
    }
};
```

- [ ] **Step 2: Build**

```bat
build.bat
```

- [ ] **Step 3: Commit**

```bash
git add Source/UI/BottomBar.h
git commit -m "feat: add BottomBar with voice count polling"
```

---

## Task 10: Refactor PluginEditor — wire everything together

**Files:**
- Modify: `Source/PluginEditor.h`
- Modify: `Source/PluginEditor.cpp`

- [ ] **Step 1: Replace `Source/PluginEditor.h` with the new version**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/LookAndFeel.h"
#include "UI/KnobWithLabel.h"
#include "UI/TopBar.h"
#include "UI/BottomBar.h"
#include "UI/SoundTab.h"
#include "UI/ModTab.h"
#include "UI/FxTab.h"
#include "UI/ArpTab.h"

class AISynthEditor : public juce::AudioProcessorEditor
{
public:
    explicit AISynthEditor(AISynthProcessor&);
    ~AISynthEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    AISynthProcessor& processor;
    SynthLookAndFeel  laf;
    juce::TooltipWindow tooltipWindow { this, 500 };

    // Persistent bars
    TopBar    topBar;
    BottomBar bottomBar;

    // Tab buttons
    juce::TextButton tabButtons[4];
    int currentTab = 0;

    // Tab content
    SoundTab soundTab;
    ModTab   modTab;
    FxTab    fxTab;
    ArpTab   arpTab;

    void showTab(int index);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AISynthEditor)
};
```

- [ ] **Step 2: Replace `Source/PluginEditor.cpp` with the new version**

```cpp
#include "PluginEditor.h"

AISynthEditor::AISynthEditor(AISynthProcessor& p)
    : AudioProcessorEditor(&p), processor(p),
      topBar(p.apvts, p.presetManager),
      bottomBar(p),
      soundTab(p.apvts),
      modTab(p.apvts),
      fxTab(p.apvts),
      arpTab(p.apvts)
{
    setLookAndFeel(&laf);
    setSize(1200, 750);

    const juce::StringArray tabNames { "SOUND", "MOD", "FX", "ARP" };
    for (int i = 0; i < 4; ++i)
    {
        tabButtons[i].setButtonText(tabNames[i]);
        tabButtons[i].setClickingTogglesState(false);
        tabButtons[i].onClick = [this, i] { showTab(i); };
        addAndMakeVisible(tabButtons[i]);
    }

    addAndMakeVisible(topBar);
    addAndMakeVisible(bottomBar);
    addAndMakeVisible(soundTab);
    addAndMakeVisible(modTab);
    addAndMakeVisible(fxTab);
    addAndMakeVisible(arpTab);

    showTab(0);
}

AISynthEditor::~AISynthEditor()
{
    setLookAndFeel(nullptr);
}

void AISynthEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(SynthLookAndFeel::colBackground));
}

void AISynthEditor::resized()
{
    const int topBarH    = 38;
    const int tabBarH    = 30;
    const int bottomBarH = 26;
    const int pad        = 6;

    auto area = getLocalBounds().reduced(pad);

    topBar.setBounds(area.removeFromTop(topBarH));
    area.removeFromTop(pad);

    // Tab buttons row
    auto tabRow = area.removeFromTop(tabBarH);
    const int tabW = tabRow.getWidth() / 4;
    for (int i = 0; i < 4; ++i)
        tabButtons[i].setBounds(tabRow.removeFromLeft(tabW).reduced(2, 0));

    area.removeFromTop(pad);
    bottomBar.setBounds(area.removeFromBottom(bottomBarH));
    area.removeFromBottom(pad);

    // All tab content occupies the same rectangle — only one is visible at a time
    for (auto* tab : { (juce::Component*)&soundTab, &modTab, &fxTab, &arpTab })
        tab->setBounds(area);
}

void AISynthEditor::showTab(int index)
{
    currentTab = index;
    soundTab.setVisible(index == 0);
    modTab  .setVisible(index == 1);
    fxTab   .setVisible(index == 2);
    arpTab  .setVisible(index == 3);

    for (int i = 0; i < 4; ++i)
        tabButtons[i].setToggleState(i == index, juce::dontSendNotification);

    repaint();
}
```

- [ ] **Step 3: Build**

```bat
build.bat
```

Expected: build succeeds. If `BottomBar.h` has a circular include via `PluginProcessor.h`, move the `#include "../PluginProcessor.h"` to the `.cpp` and forward-declare `AISynthProcessor` in the header instead:

```cpp
// BottomBar.h — replace #include with forward declaration:
class AISynthProcessor;

// Then keep AISynthProcessor& processor as member.
// The full include goes only in a BottomBar.cpp if needed,
// but since BottomBar.h uses processor methods directly, keep the include
// and ensure PluginEditor.h includes PluginProcessor.h first.
```

- [ ] **Step 4: Load plugin in AudioPluginHost or DAW**

- Open the built VST3 at `build\AISynth_artefacts\Release\VST3\AI Synth.vst3`
- Verify: plugin opens at 1200×750
- Verify: SOUND / MOD / FX / ARP tab buttons visible, clicking switches content
- Verify: all knobs on SOUND tab respond (move a knob, play MIDI, hear difference)
- Verify: top bar preset Save/Load buttons work
- Verify: bottom bar shows voice count updating when notes are played

- [ ] **Step 5: Commit**

```bash
git add Source/PluginEditor.h Source/PluginEditor.cpp
git commit -m "feat: refactor PluginEditor to tabbed layout 1200x750"
```

---

## Self-Review

**Spec coverage check:**

| Spec requirement | Task |
|---|---|
| Window 1200×750 | Task 10 — `setSize(1200, 750)` |
| Tabbed layout (SOUND · MOD · FX · ARP) | Task 10 — `showTab()` + 4 TextButtons |
| Top bar: logo + preset nav + save/load + master vol | Task 8 — `TopBar.h` |
| Bottom bar: voice count | Task 9 — `BottomBar.h` + timer |
| Sound tab 3-col: OSC1 · OSC2 · Filter+AmpEnv+FltEnv | Task 4 — `SoundTab.h` |
| Mod tab: LFO1 + LFO2 | Task 5 — `ModTab.h` |
| FX tab: Delay + Chorus | Task 6 — `FxTab.h` |
| Arp tab | Task 7 — `ArpTab.h` |
| SynthSection-style panels (rounded rect + accent border) | `drawPanel()` in each tab |
| Knobs ~48px, labels 11px | `KnobWithLabel.h` font set to 11px; knob size controlled by `setBounds` in `resized()` |
| Palette unchanged | All colour references use `SynthLookAndFeel::col*` constants |
| `getActiveVoiceCount()` | Task 2 — SynthEngine + AISynthProcessor |
| `PresetManager` on processor | Task 2 — `AISynthProcessor::presetManager` |
| Tab button styling | Task 3 — LookAndFeel `drawButtonBackground` override |
| KnobWithLabel extracted | Task 1 |

All spec requirements covered. No TBDs or placeholder steps.
