# Phase 2 — Sound Tab Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace `SoundTab` with the mockup's layout built from 7 new atomic UI components, refine the ADSR engine to exponential decay/release curves, and scaffold the responsive breakpoint API.

**Architecture:** Three layers. (1) **Engine refinement** — `Envelope.cpp` switches decay and release to exponential time-constant math with coefficients pre-computed in `setParams`; attack stays linear. (2) **Visualization helpers** — a new `Source/Engine/FilterCoefficients.h` computes RBJ biquad coefficients analytically for the UI only (the audio engine continues to use JUCE's `StateVariableTPTFilter`). (3) **Component layer** — seven new atoms in `Source/UI/components/` (`OscilloscopeView`, `WaveTypeSelect`, `FilterResponseView`, `FilterTypeSelect`, `EnvelopeEditor`, `PanelHeader`, `KnobTooltip`). `SoundTab` is rewritten as a pure composition of these atoms, with a new `layoutFor(BP)` method and a `Breakpoint.h` scaffold shared by future tabs.

Phase 2 touches no APVTS parameter IDs (done in Phase 1 Task 0) and adds no new audio features. Engine additions (osc phase/PWM, filter drive, keytrack, osc2 unison, LP12/Notch types) are deferred to Phase 2b.

**Tech Stack:** C++17, JUCE 7.0.9, CMake. Tests use `juce::UnitTest` via the `AISynthTests` console app.

**Source of truth:** `docs/superpowers/specs/2026-04-22-phase2-sound-tab-design.md`, mockup `docs/ClaudeDesign/AISynth V1 Hi-Fi.html`.

---

## Important correction vs spec (reconciled here)

The spec's §2.2 says "extract the RBJ coefficients from `Source/Engine/Filter.cpp`". **This isn't possible** — `Filter.cpp` uses `juce::dsp::StateVariableTPTFilter`, which computes its coefficients internally (no RBJ formulas in our code).

Resolution applied throughout the plan: `FilterCoefficients.h` is a **new, visualization-only helper** that computes RBJ biquad coefficients analytically. `Filter.cpp` is **not** modified. The visual RBJ curve for LP/HP/BP at a given cutoff/Q matches the TPT filter's perceived shape closely enough for a display (frequency-domain response), and keeping the engine untouched avoids any risk of audio regression in Phase 2.

---

## File Structure

**New files:**
- `Source/UI/Breakpoint.h` — `enum class BP` and `breakpointForWidth(int)` helper
- `Source/UI/components/PanelHeader.h` — section dot + uppercase title band
- `Source/UI/components/KnobTooltip.h` — drag readout overlay
- `Source/UI/components/WaveTypeSelect.h` — pill dropdown, `ComboBoxAttachment`
- `Source/UI/components/FilterTypeSelect.h` — 3 exclusive pills, `ParameterAttachment`
- `Source/UI/components/OscilloscopeView.h` — synthetic waveform rendering, ambient drift
- `Source/UI/components/FilterResponseView.h` — biquad magnitude response plot
- `Source/UI/components/EnvelopeEditor.h` — 4-node ADSR with exp curves, draggable
- `Source/Engine/FilterCoefficients.h` — header-only RBJ biquad coefficient calculator (viz only)
- `Tests/ExponentialEnvelopeTests.cpp` — verifies exp decay/release math
- `Tests/FilterCoefficientsTests.cpp` — verifies RBJ math against known reference values

**Modified files:**
- `Source/Engine/Envelope.h` — `setParams` pre-computes decay/release coefficients
- `Source/Engine/Envelope.cpp` — decay and release switch to exponential math
- `Source/UI/SoundTab.h` — rewrite, composes new atoms, exposes `layoutFor(BP)`
- `Source/PluginEditor.h` — include `Breakpoint.h`
- `Source/PluginEditor.cpp` — `resized()` computes BP, calls `soundTab.layoutFor(bp)`
- `Tests/UIConstructionTests.cpp` — extend with new components
- `CMakeLists.txt` — add 2 new test files

**Left untouched:**
- `Source/Engine/Filter.cpp` — stays on TPT filter
- `Source/UI/KnobWithLabel.h` — still used by ModTab/FXTab/ArpTab until Phase 3/4
- `Source/UI/LookAndFeel.h` legacy colour aliases — same reason
- `Source/UI/ModTab.h`, `FXTab.h`, `ArpTab.h`, `BottomBar.h`, `SynthSection.h`

---

## Task ordering rationale

Task 0 (envelope math) lands first, alone, so any regression is bisectable without UI noise. Task 1 (FilterCoefficients) is next because two later components (`FilterResponseView` tests) depend on it. Task 2 (`Breakpoint.h`) lands before `SoundTab` because the rewrite signature references `BP`. Tasks 3–9 are the seven atoms; each is a self-contained commit. Task 10 rewires `SoundTab` to use them. Task 11 wires `PluginEditor::resized()` to call the new layout method. Task 12 is a manual smoke test + deploy.

---

## Task 0: Envelope exponential decay & release

**Goal:** Replace linear decay and release with exponential time-constant math. Attack stays linear.

**Files:**
- Modify: `Source/Engine/Envelope.h`
- Modify: `Source/Engine/Envelope.cpp`
- Create: `Tests/ExponentialEnvelopeTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing test**

Create `Tests/ExponentialEnvelopeTests.cpp`:

```cpp
#include <juce_core/juce_core.h>
#include "Engine/Envelope.h"
#include <cmath>

struct ExponentialEnvelopeTests : public juce::UnitTest
{
    ExponentialEnvelopeTests() : juce::UnitTest("ExponentialEnvelope", "AISynth") {}

    void runTest() override
    {
        const double sr = 48000.0;

        beginTest("decay converges exponentially toward sustain");
        {
            Envelope env;
            env.setSampleRate(sr);
            Envelope::Params p;
            p.attack  = 0.001f;
            p.decay   = 0.050f;   // tau = 50 ms
            p.sustain = 0.5f;
            p.release = 0.1f;
            env.setParams(p);
            env.noteOn();

            // Skip the 48-sample attack then sample 4 tau into decay.
            const int skip = static_cast<int>(0.001 * sr) + 2;
            for (int i = 0; i < skip; ++i) env.getNextSample();

            const int stepsFourTau = static_cast<int>(4.0 * p.decay * sr);
            for (int i = 0; i < stepsFourTau; ++i) env.getNextSample();

            const float level = env.getNextSample();
            // After 4 tau, (1 - sustain) * exp(-4) ≈ 0.5 * 0.0183 ≈ 0.00916.
            const float expected = p.sustain + (1.f - p.sustain) * std::exp(-4.f);
            expect(std::abs(level - expected) < 0.01f,
                   juce::String("decay level ") + juce::String(level)
                 + " differs from expected " + juce::String(expected));
        }

        beginTest("release decays exponentially from noteOff level");
        {
            Envelope env;
            env.setSampleRate(sr);
            Envelope::Params p;
            p.attack  = 0.001f;
            p.decay   = 0.001f;
            p.sustain = 1.0f;
            p.release = 0.100f;   // tau = 100 ms
            env.setParams(p);
            env.noteOn();

            // Reach sustain (=1) within a few ms.
            for (int i = 0; i < 1000; ++i) env.getNextSample();

            env.noteOff();
            const int stepsOneTau = static_cast<int>(p.release * sr);
            for (int i = 0; i < stepsOneTau; ++i) env.getNextSample();

            const float level = env.getNextSample();
            // After 1 tau, level should be ~ 1.0 * exp(-1) ≈ 0.368.
            expect(std::abs(level - std::exp(-1.f)) < 0.02f,
                   juce::String("release at 1 tau was ") + juce::String(level));
        }

        beginTest("noteOn during release retriggers smoothly (no snap)");
        {
            Envelope env;
            env.setSampleRate(sr);
            Envelope::Params p;
            p.attack  = 0.005f;
            p.decay   = 0.050f;
            p.sustain = 0.5f;
            p.release = 0.200f;
            env.setParams(p);
            env.noteOn();
            for (int i = 0; i < 5000; ++i) env.getNextSample();
            env.noteOff();
            for (int i = 0; i < 1000; ++i) env.getNextSample();
            const float beforeRetrigger = env.getNextSample();
            env.noteOn();
            const float afterRetrigger = env.getNextSample();
            // The new attack should continue from the release level upward, not snap.
            expect(afterRetrigger >= beforeRetrigger - 0.01f,
                   "retrigger snapped downward");
        }
    }
};

static ExponentialEnvelopeTests sExponentialEnvelopeTests;
```

- [ ] **Step 2: Add test to CMake**

In `CMakeLists.txt`, inside `target_sources(AISynthTests PRIVATE ...)`, add `Tests/ExponentialEnvelopeTests.cpp` after `Tests/UIConstructionTests.cpp`.

- [ ] **Step 3: Run tests, confirm the new test fails**

Run: `build-with-vs.bat && build\AISynthTests_artefacts\Release\AI Synth Tests.exe`
Expected: `ExponentialEnvelope / decay converges exponentially toward sustain` fails because decay is still linear.

- [ ] **Step 4: Update `Envelope.h` to pre-compute exp coefficients**

Replace the body of `Envelope.h` with:

```cpp
#pragma once
#include <algorithm>
#include <cmath>

class Envelope
{
public:
    struct Params
    {
        float attack  = 0.01f;
        float decay   = 0.1f;
        float sustain = 0.7f;
        float release = 0.2f;
    };

    void setSampleRate(double sr)
    {
        sampleRate = sr;
        recomputeCoeffs();
    }

    void setParams(const Params& p)
    {
        params = p;
        recomputeCoeffs();
    }

    void noteOn();
    void noteOff();
    void reset();

    float getNextSample();
    bool  isActive() const { return stage != Stage::Idle; }

private:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    void recomputeCoeffs()
    {
        const float sr = static_cast<float>(sampleRate);
        decayCoeff   = std::exp(-1.f / (std::max(0.0001f, params.decay)   * sr));
        releaseCoeff = std::exp(-1.f / (std::max(0.0001f, params.release) * sr));
    }

    double sampleRate    = 44100.0;
    Params params;
    Stage  stage         = Stage::Idle;
    float  currentLevel  = 0.f;
    float  releaseLevel  = 0.f;
    float  decayCoeff    = 0.f;
    float  releaseCoeff  = 0.f;
};
```

- [ ] **Step 5: Update `Envelope.cpp` stages**

Replace `getNextSample()` body with the exponential math:

```cpp
float Envelope::getNextSample()
{
    const float sr = static_cast<float>(sampleRate);

    switch (stage)
    {
        case Stage::Idle:
            return 0.f;

        case Stage::Attack:
            currentLevel += 1.f / (std::max(0.0001f, params.attack) * sr);
            if (currentLevel >= 1.f) {
                currentLevel = 1.f;
                stage = Stage::Decay;
            }
            break;

        case Stage::Decay:
            currentLevel = params.sustain + (currentLevel - params.sustain) * decayCoeff;
            if (std::abs(currentLevel - params.sustain) < 1e-5f) {
                currentLevel = params.sustain;
                stage = Stage::Sustain;
            }
            break;

        case Stage::Sustain:
            currentLevel = params.sustain;
            break;

        case Stage::Release:
            currentLevel *= releaseCoeff;
            if (currentLevel < 1e-5f) {
                currentLevel = 0.f;
                stage = Stage::Idle;
            }
            break;
    }

    return currentLevel;
}
```

Leave `noteOn`, `noteOff`, `reset` unchanged.

- [ ] **Step 6: Run tests, confirm they pass**

Run: `build-with-vs.bat && build\AISynthTests_artefacts\Release\AI Synth Tests.exe`
Expected: all `ExponentialEnvelope` tests pass. Existing `VoiceTests`/`SynthEngineTests` still pass.

- [ ] **Step 7: Commit**

```bash
git add Source/Engine/Envelope.h Source/Engine/Envelope.cpp Tests/ExponentialEnvelopeTests.cpp CMakeLists.txt
git commit -m "feat(engine): exponential decay and release in Envelope (Phase 2 Task 0)"
```

---

## Task 1: FilterCoefficients.h (RBJ biquad for visualization)

**Goal:** Header-only helper that computes RBJ biquad coefficients for LP/HP/BP filters, used by `FilterResponseView`. Not wired into the audio engine.

**Files:**
- Create: `Source/Engine/FilterCoefficients.h`
- Create: `Tests/FilterCoefficientsTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing test**

Create `Tests/FilterCoefficientsTests.cpp`:

```cpp
#include <juce_core/juce_core.h>
#include "Engine/FilterCoefficients.h"
#include <cmath>

struct FilterCoefficientsTests : public juce::UnitTest
{
    FilterCoefficientsTests() : juce::UnitTest("FilterCoefficients", "AISynth") {}

    void runTest() override
    {
        using SynthVibe::FilterCoefficients;
        using Type = SynthVibe::FilterCoefficients::Type;

        beginTest("lowpass magnitude at cutoff is ~ 1/sqrt(2) for Q=0.707");
        {
            auto c = FilterCoefficients::compute(Type::LowPass, 1000.f, 0.707f, 48000.0);
            const float mag = c.magnitudeAt(1000.f, 48000.0);
            expect(std::abs(mag - 0.707f) < 0.05f,
                   juce::String("LP @ fc magnitude was ") + juce::String(mag));
        }

        beginTest("highpass magnitude near DC is ~ 0");
        {
            auto c = FilterCoefficients::compute(Type::HighPass, 1000.f, 0.707f, 48000.0);
            expect(c.magnitudeAt(20.f, 48000.0) < 0.1f, "HP near DC should attenuate");
        }

        beginTest("bandpass peak near cutoff with high Q");
        {
            auto c = FilterCoefficients::compute(Type::BandPass, 1000.f, 8.f, 48000.0);
            const float magAtFc  = c.magnitudeAt(1000.f, 48000.0);
            const float magLow   = c.magnitudeAt(100.f,  48000.0);
            const float magHigh  = c.magnitudeAt(10000.f, 48000.0);
            expect(magAtFc > magLow && magAtFc > magHigh, "BP should peak near fc");
        }
    }
};

static FilterCoefficientsTests sFilterCoefficientsTests;
```

- [ ] **Step 2: Add test to CMake**

In `CMakeLists.txt`, under `target_sources(AISynthTests PRIVATE ...)`, add `Tests/FilterCoefficientsTests.cpp`.

- [ ] **Step 3: Run, confirm compile fails**

Run: `build-with-vs.bat`
Expected: failure — `FilterCoefficients.h` doesn't exist.

- [ ] **Step 4: Create `Source/Engine/FilterCoefficients.h`**

```cpp
#pragma once
#include <cmath>
#include <complex>

namespace SynthVibe
{
    // Visualization-only RBJ biquad coefficients. The audio engine uses
    // juce::dsp::StateVariableTPTFilter in Source/Engine/Filter.cpp — this
    // helper exists so FilterResponseView can draw a magnitude curve without
    // tapping the live filter. The RBJ and TPT frequency responses match
    // closely for LP/HP/BP at moderate Q, which is sufficient for a display.
    struct FilterCoefficients
    {
        enum class Type { LowPass, HighPass, BandPass };

        float b0 = 1.f, b1 = 0.f, b2 = 0.f;
        float a1 = 0.f, a2 = 0.f;

        static FilterCoefficients compute(Type type, float cutoffHz, float q, double sampleRate)
        {
            FilterCoefficients c;
            const float fc = std::max(20.f, std::min(cutoffHz, (float) sampleRate * 0.49f));
            const float w0 = 2.f * 3.14159265358979323846f * fc / (float) sampleRate;
            const float cosW0 = std::cos(w0);
            const float sinW0 = std::sin(w0);
            const float alpha = sinW0 / (2.f * std::max(0.1f, q));
            const float a0 = 1.f + alpha;

            switch (type)
            {
                case Type::LowPass:
                    c.b0 = (1.f - cosW0) * 0.5f / a0;
                    c.b1 = (1.f - cosW0)        / a0;
                    c.b2 = (1.f - cosW0) * 0.5f / a0;
                    break;
                case Type::HighPass:
                    c.b0 =  (1.f + cosW0) * 0.5f / a0;
                    c.b1 = -(1.f + cosW0)        / a0;
                    c.b2 =  (1.f + cosW0) * 0.5f / a0;
                    break;
                case Type::BandPass: // constant 0 dB peak gain form
                    c.b0 =  alpha           / a0;
                    c.b1 =  0.f;
                    c.b2 = -alpha           / a0;
                    break;
            }
            c.a1 = -2.f * cosW0 / a0;
            c.a2 = (1.f - alpha) / a0;
            return c;
        }

        // |H(e^{jω})| at freq Hz.
        float magnitudeAt(float freqHz, double sampleRate) const
        {
            const float w = 2.f * 3.14159265358979323846f * freqHz / (float) sampleRate;
            using C = std::complex<float>;
            const C z1 = std::polar(1.f, -w);
            const C z2 = z1 * z1;
            const C num = b0 + b1 * z1 + b2 * z2;
            const C den = 1.f + a1 * z1 + a2 * z2;
            return std::abs(num / den);
        }
    };
}
```

- [ ] **Step 5: Run tests, confirm pass**

Run: `build-with-vs.bat && build\AISynthTests_artefacts\Release\AI Synth Tests.exe`
Expected: all `FilterCoefficients` tests pass.

- [ ] **Step 6: Commit**

```bash
git add Source/Engine/FilterCoefficients.h Tests/FilterCoefficientsTests.cpp CMakeLists.txt
git commit -m "feat(engine): add RBJ biquad coefficient helper for UI visualization (Phase 2 Task 1)"
```

---

## Task 2: Breakpoint.h scaffold

**Goal:** Header-only responsive breakpoint enum and helper, shared across the UI.

**Files:**
- Create: `Source/UI/Breakpoint.h`

- [ ] **Step 1: Create the header**

```cpp
#pragma once

namespace SynthVibe
{
    enum class BP { Compact, Default, Wide };

    inline BP breakpointForWidth(int w)
    {
        if (w < 1150) return BP::Compact;
        if (w < 1550) return BP::Default;
        return BP::Wide;
    }
}
```

- [ ] **Step 2: Build to confirm it compiles as a standalone header**

Run: `build-with-vs.bat`
Expected: build succeeds unchanged.

- [ ] **Step 3: Commit**

```bash
git add Source/UI/Breakpoint.h
git commit -m "feat(ui): add BP enum scaffold for responsive layout (Phase 2 Task 2)"
```

---

## Task 3: PanelHeader component

**Goal:** Small header band with a coloured section dot and uppercase mono title.

**Files:**
- Create: `Source/UI/components/PanelHeader.h`
- Modify: `Tests/UIConstructionTests.cpp`

- [ ] **Step 1: Create `Source/UI/components/PanelHeader.h`**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class PanelHeader : public juce::Component
    {
    public:
        PanelHeader(const juce::String& title, juce::Colour dot)
            : titleText(title.toUpperCase()), dotColour(dot) {}

        void paint(juce::Graphics& g) override
        {
            using namespace Tokens;
            auto b = getLocalBounds().toFloat();

            const float dotR = 3.f;
            const float dotX = b.getX() + spaceSm + dotR;
            const float dotY = b.getCentreY();
            g.setColour(dotColour);
            g.fillEllipse(dotX - dotR, dotY - dotR, dotR * 2.f, dotR * 2.f);

            g.setColour(ink2);
            g.setFont(Fonts::mono(Font::panelTitle));
            g.drawText(titleText,
                       (int) (dotX + dotR + spaceSm),
                       (int) b.getY(),
                       (int) (b.getWidth() - (dotX + dotR + spaceSm)),
                       (int) b.getHeight(),
                       juce::Justification::centredLeft);

            g.setColour(edge);
            g.drawHorizontalLine((int) b.getBottom() - 1,
                                 b.getX(), b.getRight());
        }

    private:
        juce::String  titleText;
        juce::Colour  dotColour;
    };
}
```

- [ ] **Step 2: Extend `Tests/UIConstructionTests.cpp`**

Add inside `runTest()` after the existing `beginTest`:

```cpp
beginTest("PanelHeader constructs");
{
    SynthVibe::PanelHeader h("OSC 1", SynthVibe::Tokens::osc);
    h.setBounds(0, 0, 200, 20);
    expect(h.isVisible() == false); // not yet added to a parent
}
```

Include at the top: `#include "UI/components/PanelHeader.h"`.

- [ ] **Step 3: Build & run tests**

Run: `build-with-vs.bat && build\AISynthTests_artefacts\Release\AI Synth Tests.exe`
Expected: all tests pass.

- [ ] **Step 4: Commit**

```bash
git add Source/UI/components/PanelHeader.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): add PanelHeader component (Phase 2 Task 3)"
```

---

## Task 4: KnobTooltip component

**Goal:** Floating readout that appears next to a knob during drag. Owned by SoundTab for now; each atom that needs it holds a pointer.

**Files:**
- Create: `Source/UI/components/KnobTooltip.h`

- [ ] **Step 1: Create the header**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class KnobTooltip : public juce::Component
    {
    public:
        KnobTooltip()
        {
            setInterceptsMouseClicks(false, false);
            setVisible(false);
        }

        void showAt(juce::Point<int> anchor, const juce::String& text)
        {
            readout = text;
            const int w = juce::jmax(48,
                Fonts::mono(Tokens::Font::value).getStringWidth(text) + Tokens::spaceMd);
            setBounds(anchor.x - w / 2,
                      anchor.y - 22,
                      w, 18);
            setVisible(true);
            toFront(false);
            repaint();
        }

        void hide() { setVisible(false); }

        void paint(juce::Graphics& g) override
        {
            using namespace Tokens;
            auto b = getLocalBounds().toFloat();
            g.setColour(panel2.withAlpha(0.95f));
            g.fillRoundedRectangle(b, radiusSm);
            g.setColour(edge);
            g.drawRoundedRectangle(b, radiusSm, 1.f);
            g.setColour(ink);
            g.setFont(Fonts::mono(Font::value));
            g.drawText(readout, getLocalBounds(), juce::Justification::centred);
        }

    private:
        juce::String readout;
    };
}
```

- [ ] **Step 2: Build**

Run: `build-with-vs.bat`
Expected: build succeeds.

- [ ] **Step 3: Commit**

```bash
git add Source/UI/components/KnobTooltip.h
git commit -m "feat(ui): add KnobTooltip component (Phase 2 Task 4)"
```

---

## Task 5: WaveTypeSelect component

**Goal:** Pill-styled dropdown for waveform choice, bound to an APVTS choice parameter.

**Files:**
- Create: `Source/UI/components/WaveTypeSelect.h`
- Modify: `Tests/UIConstructionTests.cpp`

- [ ] **Step 1: Create the header**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class WaveTypeSelect : public juce::Component
    {
    public:
        WaveTypeSelect(juce::AudioProcessorValueTreeState& apvts,
                       const juce::String& paramID)
        {
            combo.addItemList({ "Sine", "Saw", "Square", "Triangle" }, 1);
            combo.setColour(juce::ComboBox::backgroundColourId, Tokens::panel2);
            combo.setColour(juce::ComboBox::textColourId, Tokens::ink);
            combo.setColour(juce::ComboBox::outlineColourId, Tokens::edge);
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

- [ ] **Step 2: Extend `Tests/UIConstructionTests.cpp`**

Add inside `runTest()`:

```cpp
beginTest("WaveTypeSelect binds to osc1.wave");
{
    SynthVibe::WaveTypeSelect ws(apvts, ParamIDs::osc1Waveform);
    ws.setBounds(0, 0, 120, 26);
    // Default waveform index should be Sine (1).
    expectEquals(ws.getCombo().getSelectedId(), 1);
}
```

Include `#include "UI/components/WaveTypeSelect.h"` at top.

- [ ] **Step 3: Build & run tests**

Run: `build-with-vs.bat && build\AISynthTests_artefacts\Release\AI Synth Tests.exe`
Expected: all tests pass.

- [ ] **Step 4: Commit**

```bash
git add Source/UI/components/WaveTypeSelect.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): add WaveTypeSelect component (Phase 2 Task 5)"
```

---

## Task 6: FilterTypeSelect component

**Goal:** Row of three mutually-exclusive pills (LP / HP / BP), bound to the `filt.type` choice parameter.

**Files:**
- Create: `Source/UI/components/FilterTypeSelect.h`
- Modify: `Tests/UIConstructionTests.cpp`

- [ ] **Step 1: Create the header**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class FilterTypeSelect : public juce::Component
    {
    public:
        FilterTypeSelect(juce::AudioProcessorValueTreeState& apvts,
                         const juce::String& paramID)
        {
            const char* labels[] = { "LP", "HP", "BP" };
            for (int i = 0; i < 3; ++i)
            {
                auto& btn = pills[i];
                btn.setButtonText(labels[i]);
                btn.setClickingTogglesState(true);
                btn.setRadioGroupId(1);
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
                    const int idx = juce::jlimit(0, 2, (int) std::round(v));
                    for (int i = 0; i < 3; ++i)
                        pills[i].setToggleState(i == idx, juce::dontSendNotification);
                });
            paramAttach->sendInitialUpdate();
        }

        void resized() override
        {
            auto b = getLocalBounds();
            const int w = b.getWidth() / 3;
            for (int i = 0; i < 3; ++i)
                pills[i].setBounds(b.removeFromLeft(w).reduced(2));
        }

    private:
        juce::TextButton pills[3];
        std::unique_ptr<juce::ParameterAttachment> paramAttach;
    };
}
```

- [ ] **Step 2: Extend `Tests/UIConstructionTests.cpp`**

Add inside `runTest()`:

```cpp
beginTest("FilterTypeSelect constructs and binds to filt.type");
{
    SynthVibe::FilterTypeSelect fts(apvts, ParamIDs::filterType);
    fts.setBounds(0, 0, 150, 28);
}
```

Include `#include "UI/components/FilterTypeSelect.h"`.

- [ ] **Step 3: Build & run tests**

Run: `build-with-vs.bat && build\AISynthTests_artefacts\Release\AI Synth Tests.exe`
Expected: all tests pass.

- [ ] **Step 4: Commit**

```bash
git add Source/UI/components/FilterTypeSelect.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): add FilterTypeSelect component (Phase 2 Task 6)"
```

---

## Task 7: OscilloscopeView component

**Goal:** Synthetic waveform rendering driven by the `oscN.wave` choice param, with a slow ambient drift animation.

**Files:**
- Create: `Source/UI/components/OscilloscopeView.h`
- Modify: `Tests/UIConstructionTests.cpp`

- [ ] **Step 1: Create the header**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"

namespace SynthVibe
{
    class OscilloscopeView : public juce::Component, private juce::Timer
    {
    public:
        OscilloscopeView(juce::AudioProcessorValueTreeState& apvts,
                         const juce::String& waveformParamID,
                         juce::Colour lineColour = Tokens::osc)
            : param(apvts.getParameter(waveformParamID)),
              colour(lineColour)
        {
            waveAttach = std::make_unique<juce::ParameterAttachment>(
                *param, [this](float v) {
                    waveIndex = juce::jlimit(0, 3, (int) std::round(v));
                    repaint();
                });
            waveAttach->sendInitialUpdate();
            startTimerHz(30);
        }

        void paint(juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat().reduced(2.f);
            const float midY = b.getCentreY();
            const float amp  = b.getHeight() * 0.35f;

            juce::Path path;
            const int N = 128;
            for (int i = 0; i <= N; ++i)
            {
                const float phase = (float) i / N;
                const float t = std::fmod(phase + drift, 1.f);
                const float y = midY - amp * sample(t);
                const float x = b.getX() + phase * b.getWidth();
                if (i == 0) path.startNewSubPath(x, y);
                else        path.lineTo(x, y);
            }

            g.setColour(colour.withAlpha(0.25f));
            g.strokePath(path, juce::PathStrokeType(3.f));
            g.setColour(colour);
            g.strokePath(path, juce::PathStrokeType(1.5f));
        }

    private:
        void timerCallback() override
        {
            drift += 1.f / (float) (Tokens::Motion::ambientScopeMs / 33);
            if (drift > 1.f) drift -= 1.f;
            repaint();
        }

        float sample(float t) const
        {
            switch (waveIndex)
            {
                case 0: return std::sin(2.f * 3.14159265358979323846f * t);     // Sine
                case 1: return 2.f * t - 1.f;                                    // Saw
                case 2: return t < 0.5f ? 1.f : -1.f;                            // Square
                case 3: return 4.f * std::abs(t - 0.5f) - 1.f;                   // Triangle
            }
            return 0.f;
        }

        juce::RangedAudioParameter* param;
        juce::Colour  colour;
        int           waveIndex = 0;
        float         drift     = 0.f;
        std::unique_ptr<juce::ParameterAttachment> waveAttach;
    };
}
```

- [ ] **Step 2: Extend `Tests/UIConstructionTests.cpp`**

Add:

```cpp
beginTest("OscilloscopeView binds without crash");
{
    SynthVibe::OscilloscopeView scope(apvts, ParamIDs::osc1Waveform);
    scope.setBounds(0, 0, 200, 60);
}
```

Include `#include "UI/components/OscilloscopeView.h"`.

- [ ] **Step 3: Build & run tests**

Run: `build-with-vs.bat && build\AISynthTests_artefacts\Release\AI Synth Tests.exe`
Expected: all tests pass.

- [ ] **Step 4: Commit**

```bash
git add Source/UI/components/OscilloscopeView.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): add OscilloscopeView component (Phase 2 Task 7)"
```

---

## Task 8: FilterResponseView component

**Goal:** Magnitude response plot of an RBJ biquad driven by `filt.{type,cutoff,res}`.

**Files:**
- Create: `Source/UI/components/FilterResponseView.h`
- Modify: `Tests/UIConstructionTests.cpp`

- [ ] **Step 1: Create the header**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"
#include "../../Engine/FilterCoefficients.h"

namespace SynthVibe
{
    class FilterResponseView : public juce::Component, private juce::Timer
    {
    public:
        FilterResponseView(juce::AudioProcessorValueTreeState& apvts,
                           const juce::String& typeParam,
                           const juce::String& cutoffParam,
                           const juce::String& resParam,
                           juce::Colour lineColour = Tokens::filter)
            : apvtsRef(apvts),
              typeID(typeParam), cutoffID(cutoffParam), resID(resParam),
              colour(lineColour)
        {
            startTimerHz(30);
        }

        void paint(juce::Graphics& g) override
        {
            using Type = FilterCoefficients::Type;
            const int typeIdx = (int) std::round(apvtsRef.getRawParameterValue(typeID)->load());
            const float cutoff = apvtsRef.getRawParameterValue(cutoffID)->load();
            const float resN   = apvtsRef.getRawParameterValue(resID)->load();
            const float q      = juce::jmap(juce::jlimit(0.f, 1.f, resN), 0.5f, 12.f);

            const Type t = typeIdx == 0 ? Type::LowPass
                         : typeIdx == 1 ? Type::HighPass
                                        : Type::BandPass;
            const double sr = 48000.0;
            const auto coeffs = FilterCoefficients::compute(t, cutoff, q, sr);

            auto b = getLocalBounds().toFloat().reduced(4.f);
            const int N = 120;
            const float fLow  = 20.f;
            const float fHigh = 20000.f;

            juce::Path path;
            for (int i = 0; i <= N; ++i)
            {
                const float u = (float) i / N;
                const float freq = fLow * std::pow(fHigh / fLow, u);
                const float mag  = coeffs.magnitudeAt(freq, sr);
                const float db   = juce::Decibels::gainToDecibels(mag + 1e-6f);
                const float y    = juce::jmap(juce::jlimit(-36.f, 12.f, db),
                                              -36.f, 12.f, b.getBottom(), b.getY());
                const float x    = b.getX() + u * b.getWidth();
                if (i == 0) path.startNewSubPath(x, y);
                else        path.lineTo(x, y);
            }

            g.setColour(colour.withAlpha(0.25f));
            g.strokePath(path, juce::PathStrokeType(3.f));
            g.setColour(colour);
            g.strokePath(path, juce::PathStrokeType(1.5f));
        }

    private:
        void timerCallback() override { repaint(); }

        juce::AudioProcessorValueTreeState& apvtsRef;
        juce::String typeID, cutoffID, resID;
        juce::Colour colour;
    };
}
```

- [ ] **Step 2: Extend `Tests/UIConstructionTests.cpp`**

Add:

```cpp
beginTest("FilterResponseView binds without crash");
{
    SynthVibe::FilterResponseView frv(apvts,
        ParamIDs::filterType, ParamIDs::filterCutoff, ParamIDs::filterResonance);
    frv.setBounds(0, 0, 200, 80);
}
```

Include `#include "UI/components/FilterResponseView.h"`.

- [ ] **Step 3: Build & run tests**

Run: `build-with-vs.bat && build\AISynthTests_artefacts\Release\AI Synth Tests.exe`
Expected: all tests pass.

- [ ] **Step 4: Commit**

```bash
git add Source/UI/components/FilterResponseView.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): add FilterResponseView component (Phase 2 Task 8)"
```

---

## Task 9: EnvelopeEditor component

**Goal:** 4-node ADSR editor. Nodes are draggable; they write via `ParameterAttachment`. Rendering uses exponential curves matching the engine.

**Files:**
- Create: `Source/UI/components/EnvelopeEditor.h`
- Modify: `Tests/UIConstructionTests.cpp`

- [ ] **Step 1: Create the header**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class EnvelopeEditor : public juce::Component
    {
    public:
        EnvelopeEditor(juce::AudioProcessorValueTreeState& apvts,
                       const juce::String& attackID,
                       const juce::String& decayID,
                       const juce::String& sustainID,
                       const juce::String& releaseID,
                       juce::Colour lineColour = Tokens::env)
            : apvtsRef(apvts),
              aID(attackID), dID(decayID), sID(sustainID), rID(releaseID),
              colour(lineColour)
        {
            auto makeAttach = [&](const juce::String& id) {
                return std::make_unique<juce::ParameterAttachment>(
                    *apvts.getParameter(id),
                    [this](float) { repaint(); });
            };
            aAttach = makeAttach(attackID);
            dAttach = makeAttach(decayID);
            sAttach = makeAttach(sustainID);
            rAttach = makeAttach(releaseID);
            aAttach->sendInitialUpdate();
            dAttach->sendInitialUpdate();
            sAttach->sendInitialUpdate();
            rAttach->sendInitialUpdate();
        }

        void paint(juce::Graphics& g) override
        {
            const float a = apvtsRef.getRawParameterValue(aID)->load();
            const float d = apvtsRef.getRawParameterValue(dID)->load();
            const float s = apvtsRef.getRawParameterValue(sID)->load();
            const float r = apvtsRef.getRawParameterValue(rID)->load();

            auto b = getLocalBounds().toFloat().reduced(6.f);
            b.removeFromBottom(14.f); // space for readout line

            const float totalTime = a + d + 0.5f + r; // visual sustain window = 0.5s
            const auto xFor = [&](float t) {
                return b.getX() + (t / totalTime) * b.getWidth();
            };
            const auto yFor = [&](float level) {
                return b.getBottom() - level * b.getHeight();
            };

            juce::Path path;
            path.startNewSubPath(xFor(0.f), yFor(0.f));
            path.lineTo(xFor(a), yFor(1.f));

            // Decay exp curve
            const int decaySteps = 40;
            for (int i = 1; i <= decaySteps; ++i)
            {
                const float t  = (float) i / decaySteps * d;
                const float y  = s + (1.f - s) * std::exp(-t / std::max(0.0001f, d));
                path.lineTo(xFor(a + t), yFor(y));
            }
            path.lineTo(xFor(a + d + 0.5f), yFor(s));

            const int releaseSteps = 40;
            for (int i = 1; i <= releaseSteps; ++i)
            {
                const float t = (float) i / releaseSteps * r;
                const float y = s * std::exp(-t / std::max(0.0001f, r));
                path.lineTo(xFor(a + d + 0.5f + t), yFor(y));
            }

            g.setColour(colour.withAlpha(0.25f));
            g.strokePath(path, juce::PathStrokeType(3.f));
            g.setColour(colour);
            g.strokePath(path, juce::PathStrokeType(1.5f));

            // Nodes
            const juce::Point<float> nodes[] = {
                { xFor(a),              yFor(1.f) },
                { xFor(a + d),          yFor(s)   },
                { xFor(a + d + 0.5f),   yFor(s)   },
                { xFor(a + d + 0.5f+r), yFor(0.f) }
            };
            for (auto& n : nodes)
            {
                g.setColour(Tokens::panel);
                g.fillEllipse(n.x - 4.f, n.y - 4.f, 8.f, 8.f);
                g.setColour(colour);
                g.drawEllipse(n.x - 4.f, n.y - 4.f, 8.f, 8.f, 1.5f);
            }

            // Readout line
            juce::String readout;
            readout << "a " << juce::String((int) (a * 1000)) << "ms"
                    << " · d " << juce::String((int) (d * 1000)) << "ms"
                    << " · s " << juce::String(s, 2)
                    << " · r " << juce::String((int) (r * 1000)) << "ms";
            g.setColour(Tokens::ink3);
            g.setFont(Fonts::mono(Tokens::Font::label));
            g.drawText(readout,
                       getLocalBounds().removeFromBottom(14).reduced(6, 0),
                       juce::Justification::centredLeft);
        }

        void mouseDown(const juce::MouseEvent& e) override
        {
            dragNode = findNearestNode(e.position);
            if (dragNode >= 0)
            {
                aAttach->beginGesture();
                dAttach->beginGesture();
                sAttach->beginGesture();
                rAttach->beginGesture();
            }
        }

        void mouseDrag(const juce::MouseEvent& e) override
        {
            if (dragNode < 0) return;
            auto b = getLocalBounds().toFloat().reduced(6.f);
            b.removeFromBottom(14.f);

            const float a = apvtsRef.getRawParameterValue(aID)->load();
            const float d = apvtsRef.getRawParameterValue(dID)->load();
            const float r = apvtsRef.getRawParameterValue(rID)->load();
            const float totalTime = a + d + 0.5f + r;

            const float normX = juce::jlimit(0.f, 1.f, (e.position.x - b.getX()) / b.getWidth());
            const float normY = juce::jlimit(0.f, 1.f, 1.f - (e.position.y - b.getY()) / b.getHeight());
            const float timeAtX = normX * totalTime;

            switch (dragNode)
            {
                case 0: // attack
                    aAttach->setValueAsPartOfGesture(juce::jlimit(0.001f, 4.f, timeAtX));
                    break;
                case 1: // decay corner
                    dAttach->setValueAsPartOfGesture(juce::jlimit(0.001f, 4.f, timeAtX - a));
                    sAttach->setValueAsPartOfGesture(normY);
                    break;
                case 2: // sustain-end y-only
                    sAttach->setValueAsPartOfGesture(normY);
                    break;
                case 3: // release end
                    rAttach->setValueAsPartOfGesture(juce::jlimit(0.001f, 8.f, timeAtX - a - d - 0.5f));
                    break;
            }
        }

        void mouseUp(const juce::MouseEvent&) override
        {
            if (dragNode < 0) return;
            aAttach->endGesture();
            dAttach->endGesture();
            sAttach->endGesture();
            rAttach->endGesture();
            dragNode = -1;
        }

    private:
        int findNearestNode(juce::Point<float> p) const
        {
            const float a = apvtsRef.getRawParameterValue(aID)->load();
            const float d = apvtsRef.getRawParameterValue(dID)->load();
            const float s = apvtsRef.getRawParameterValue(sID)->load();
            const float r = apvtsRef.getRawParameterValue(rID)->load();
            auto b = getLocalBounds().toFloat().reduced(6.f);
            b.removeFromBottom(14.f);
            const float totalTime = a + d + 0.5f + r;

            const juce::Point<float> nodes[] = {
                { b.getX() + (a / totalTime) * b.getWidth(),            b.getBottom() - 1.f * b.getHeight() },
                { b.getX() + ((a + d) / totalTime) * b.getWidth(),      b.getBottom() - s   * b.getHeight() },
                { b.getX() + ((a + d + 0.5f) / totalTime) * b.getWidth(), b.getBottom() - s * b.getHeight() },
                { b.getX() + ((a + d + 0.5f + r) / totalTime) * b.getWidth(), b.getBottom() - 0.f * b.getHeight() }
            };
            int best = -1;
            float bestDist = 14.f; // hit radius in px
            for (int i = 0; i < 4; ++i)
            {
                const float dist = p.getDistanceFrom(nodes[i]);
                if (dist < bestDist) { best = i; bestDist = dist; }
            }
            return best;
        }

        juce::AudioProcessorValueTreeState& apvtsRef;
        juce::String aID, dID, sID, rID;
        juce::Colour colour;
        std::unique_ptr<juce::ParameterAttachment> aAttach, dAttach, sAttach, rAttach;
        int dragNode = -1;
    };
}
```

- [ ] **Step 2: Extend `Tests/UIConstructionTests.cpp`**

Add:

```cpp
beginTest("EnvelopeEditor binds without crash");
{
    SynthVibe::EnvelopeEditor env(apvts,
        ParamIDs::ampAttack, ParamIDs::ampDecay,
        ParamIDs::ampSustain, ParamIDs::ampRelease);
    env.setBounds(0, 0, 240, 120);
}
```

Include `#include "UI/components/EnvelopeEditor.h"`.

- [ ] **Step 3: Build & run tests**

Run: `build-with-vs.bat && build\AISynthTests_artefacts\Release\AI Synth Tests.exe`
Expected: all tests pass.

- [ ] **Step 4: Commit**

```bash
git add Source/UI/components/EnvelopeEditor.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): add EnvelopeEditor component (Phase 2 Task 9)"
```

---

## Task 10: SoundTab rewrite

**Goal:** Replace the current `SoundTab.h` with a composition of the new atoms. Keep 3 columns, keep the Filter/AmpEnv/FltEnv stack in column 3, add the oscilloscopes and filter response plot, swap combos for the new pill components, swap the ADSR knobs for EnvelopeEditor instances. Omit the knobs that don't have engine parameters yet (phase, PWM, drive, keytrack, osc2 unison) — those come in Phase 2b.

**Files:**
- Modify: `Source/UI/SoundTab.h`

- [ ] **Step 1: Rewrite `Source/UI/SoundTab.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Breakpoint.h"
#include "DesignTokens.h"
#include "../Parameters/ParameterIDs.h"
#include "components/ArcKnob.h"
#include "components/EnvelopeEditor.h"
#include "components/FilterResponseView.h"
#include "components/FilterTypeSelect.h"
#include "components/OscilloscopeView.h"
#include "components/PanelHeader.h"
#include "components/WaveTypeSelect.h"

class SoundTab : public juce::Component
{
public:
    explicit SoundTab(juce::AudioProcessorValueTreeState& apvts)
        : apvts(apvts),
          osc1Header("OSC 1", SynthVibe::Tokens::osc),
          osc2Header("OSC 2", SynthVibe::Tokens::osc),
          filterHeader("FILTER", SynthVibe::Tokens::filter),
          ampEnvHeader("AMP ENV", SynthVibe::Tokens::env),
          fltEnvHeader("FLT ENV", SynthVibe::Tokens::env),
          osc1Wave(apvts, ParamIDs::osc1Waveform),
          osc2Wave(apvts, ParamIDs::osc2Waveform),
          filterType(apvts, ParamIDs::filterType),
          osc1Scope(apvts, ParamIDs::osc1Waveform),
          osc2Scope(apvts, ParamIDs::osc2Waveform),
          filterResponse(apvts,
                         ParamIDs::filterType,
                         ParamIDs::filterCutoff,
                         ParamIDs::filterResonance),
          ampEnv(apvts, ParamIDs::ampAttack, ParamIDs::ampDecay,
                 ParamIDs::ampSustain, ParamIDs::ampRelease),
          fltEnv(apvts, ParamIDs::fltAttack, ParamIDs::fltDecay,
                 ParamIDs::fltSustain, ParamIDs::fltRelease),
          knobOsc1Oct("Oct", apvts, ParamIDs::osc1Octave, SynthVibe::Tokens::osc, "", 0),
          knobOsc1Semi("Semi", apvts, ParamIDs::osc1Semitone, SynthVibe::Tokens::osc, "", 0),
          knobOsc1Detune("Detune", apvts, ParamIDs::osc1Detune, SynthVibe::Tokens::osc, " ct", 1),
          knobOsc1Level("Level", apvts, ParamIDs::osc1Level, SynthVibe::Tokens::osc, "", 2),
          knobUniVoices("Voices", apvts, ParamIDs::unisonVoices, SynthVibe::Tokens::osc, "", 0),
          knobUniDetune("Detune", apvts, ParamIDs::unisonDetune, SynthVibe::Tokens::osc, " ct", 1),
          knobUniSpread("Stereo", apvts, ParamIDs::unisonStereoSpread, SynthVibe::Tokens::osc, "", 2),
          knobOsc2Oct("Oct", apvts, ParamIDs::osc2Octave, SynthVibe::Tokens::osc, "", 0),
          knobOsc2Semi("Semi", apvts, ParamIDs::osc2Semitone, SynthVibe::Tokens::osc, "", 0),
          knobOsc2Detune("Detune", apvts, ParamIDs::osc2Detune, SynthVibe::Tokens::osc, " ct", 1),
          knobOsc2Level("Level", apvts, ParamIDs::osc2Level, SynthVibe::Tokens::osc, "", 2),
          knobCutoff("Cutoff", apvts, ParamIDs::filterCutoff, SynthVibe::Tokens::filter, " Hz", 0),
          knobResonance("Res", apvts, ParamIDs::filterResonance, SynthVibe::Tokens::filter, "", 2),
          knobFilterEnv("Env", apvts, ParamIDs::filterEnvAmt, SynthVibe::Tokens::filter, "", 2)
    {
        for (auto* c : std::initializer_list<juce::Component*> {
            &osc1Header, &osc2Header, &filterHeader, &ampEnvHeader, &fltEnvHeader,
            &osc1Wave, &osc2Wave, &filterType,
            &osc1Scope, &osc2Scope, &filterResponse,
            &ampEnv, &fltEnv,
            &knobOsc1Oct, &knobOsc1Semi, &knobOsc1Detune, &knobOsc1Level,
            &knobUniVoices, &knobUniDetune, &knobUniSpread,
            &knobOsc2Oct, &knobOsc2Semi, &knobOsc2Detune, &knobOsc2Level,
            &knobCutoff, &knobResonance, &knobFilterEnv
        })
            addAndMakeVisible(c);
    }

    void paint(juce::Graphics& g) override
    {
        using namespace SynthVibe::Tokens;
        for (auto r : { osc1Bounds, osc2Bounds, filterBounds, ampEnvBounds, fltEnvBounds })
        {
            auto f = r.toFloat().reduced(2.f);
            g.setColour(panel);
            g.fillRoundedRectangle(f, radiusMd);
            g.setColour(edge);
            g.drawRoundedRectangle(f, radiusMd, 1.f);
        }
    }

    void resized() override { layoutFor(SynthVibe::breakpointForWidth(getWidth())); }

    void layoutFor(SynthVibe::BP /*bp*/)
    {
        using namespace SynthVibe::Tokens;
        const int headerH = 20;
        const int scopeH  = 60;
        const int selectH = 26;

        auto area = getLocalBounds().reduced(spaceSm);
        const int colW = area.getWidth() / 3;
        auto col1 = area.removeFromLeft(colW).reduced(spaceXs, 0);
        auto col2 = area.removeFromLeft(colW).reduced(spaceXs, 0);
        auto col3 = area.reduced(spaceXs, 0);

        osc1Bounds = col1;
        auto c1 = col1.reduced(spaceSm);
        osc1Header.setBounds(c1.removeFromTop(headerH));
        c1.removeFromTop(spaceXs);
        osc1Scope.setBounds(c1.removeFromTop(scopeH));
        c1.removeFromTop(spaceXs);
        osc1Wave.setBounds(c1.removeFromTop(selectH));
        c1.removeFromTop(spaceSm);
        auto oscRow1 = c1.removeFromTop(c1.getHeight() / 2);
        layoutKnobsRow(oscRow1, { &knobOsc1Oct, &knobOsc1Semi, &knobOsc1Detune, &knobOsc1Level });
        layoutKnobsRow(c1, { &knobUniVoices, &knobUniDetune, &knobUniSpread });

        osc2Bounds = col2;
        auto c2 = col2.reduced(spaceSm);
        osc2Header.setBounds(c2.removeFromTop(headerH));
        c2.removeFromTop(spaceXs);
        osc2Scope.setBounds(c2.removeFromTop(scopeH));
        c2.removeFromTop(spaceXs);
        osc2Wave.setBounds(c2.removeFromTop(selectH));
        c2.removeFromTop(spaceSm);
        layoutKnobsRow(c2, { &knobOsc2Oct, &knobOsc2Semi, &knobOsc2Detune, &knobOsc2Level });

        const int filterH = col3.getHeight() * 38 / 100;
        const int envH    = (col3.getHeight() - filterH) / 2;

        filterBounds = col3.removeFromTop(filterH);
        auto cf = filterBounds.reduced(spaceSm);
        filterHeader.setBounds(cf.removeFromTop(headerH));
        cf.removeFromTop(spaceXs);
        filterResponse.setBounds(cf.removeFromTop(cf.getHeight() / 2));
        cf.removeFromTop(spaceXs);
        filterType.setBounds(cf.removeFromTop(selectH));
        cf.removeFromTop(spaceXs);
        layoutKnobsRow(cf, { &knobCutoff, &knobResonance, &knobFilterEnv });

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
    }

private:
    static void layoutKnobsRow(juce::Rectangle<int> bounds,
                               std::initializer_list<juce::Component*> knobs)
    {
        if (knobs.size() == 0) return;
        const int w = bounds.getWidth() / (int) knobs.size();
        auto b = bounds;
        for (auto* k : knobs) k->setBounds(b.removeFromLeft(w));
    }

    juce::AudioProcessorValueTreeState& apvts;

    juce::Rectangle<int> osc1Bounds, osc2Bounds, filterBounds, ampEnvBounds, fltEnvBounds;

    SynthVibe::PanelHeader       osc1Header, osc2Header, filterHeader, ampEnvHeader, fltEnvHeader;
    SynthVibe::WaveTypeSelect    osc1Wave, osc2Wave;
    SynthVibe::FilterTypeSelect  filterType;
    SynthVibe::OscilloscopeView  osc1Scope, osc2Scope;
    SynthVibe::FilterResponseView filterResponse;
    SynthVibe::EnvelopeEditor    ampEnv, fltEnv;

    SynthVibe::ArcKnob knobOsc1Oct, knobOsc1Semi, knobOsc1Detune, knobOsc1Level;
    SynthVibe::ArcKnob knobUniVoices, knobUniDetune, knobUniSpread;
    SynthVibe::ArcKnob knobOsc2Oct, knobOsc2Semi, knobOsc2Detune, knobOsc2Level;
    SynthVibe::ArcKnob knobCutoff, knobResonance, knobFilterEnv;
};
```

- [ ] **Step 2: Build**

Run: `build-with-vs.bat`
Expected: build succeeds.

- [ ] **Step 3: Run tests**

Run: `build\AISynthTests_artefacts\Release\AI Synth Tests.exe`
Expected: all tests pass.

- [ ] **Step 4: Commit**

```bash
git add Source/UI/SoundTab.h
git commit -m "feat(ui): rewrite SoundTab with new atoms + layoutFor(BP) (Phase 2 Task 10)"
```

---

## Task 11: PluginEditor BP wiring

**Goal:** `PluginEditor::resized()` computes the breakpoint once and hands it to `soundTab.layoutFor(bp)`. Other tabs keep their existing `resized()` until their rewrites.

**Files:**
- Modify: `Source/PluginEditor.h`
- Modify: `Source/PluginEditor.cpp`

- [ ] **Step 1: Edit `Source/PluginEditor.h`**

Add near the other UI includes:
```cpp
#include "UI/Breakpoint.h"
```

- [ ] **Step 2: Edit `Source/PluginEditor.cpp` `resized()`**

At the top of `resized()`, after computing `area`, add:

```cpp
    const auto bp = SynthVibe::breakpointForWidth(getWidth());
    juce::ignoreUnused(bp); // consumed by Phase 3/4 tab rewrites
```

Leave the existing `soundTab.setBounds(area); modTab.setBounds(area); ...` block as is. `SoundTab::resized()` already computes its own BP from `getWidth()` and dispatches to `layoutFor(bp)` — so `setBounds` is sufficient. The explicit `bp` local in `PluginEditor::resized()` is the scaffold placeholder; Phase 3/4 tabs will read it when they gain their own `layoutFor(BP)` methods.

- [ ] **Step 3: Build**

Run: `build-with-vs.bat`
Expected: build succeeds.

- [ ] **Step 4: Commit**

```bash
git add Source/PluginEditor.h Source/PluginEditor.cpp
git commit -m "feat(ui): wire breakpoint scaffold into PluginEditor::resized (Phase 2 Task 11)"
```

---

## Task 12: Manual smoke test + deploy

**Goal:** Confirm the new Sound Tab opens correctly in a DAW at 1280×720, knobs bind, the oscilloscope drifts, the filter curve updates, and the envelope editor is draggable. No automated UI visual test — the user validates in the DAW per their preference.

**Files:** none (manual verification + deploy)

- [ ] **Step 1: Full build**

Run: `build-with-vs.bat`
Expected: `build\AISynth_artefacts\Release\VST3\AI Synth.vst3` exists.

- [ ] **Step 2: Full test run**

Run: `build\AISynthTests_artefacts\Release\AI Synth Tests.exe`
Expected: all tests pass — `ParameterIdMigration`, `DesignTokens`, `UIConstruction`, `Voice`, `SynthEngine`, `Arp`, `ExponentialEnvelope`, `FilterCoefficients`.

- [ ] **Step 3: Deploy**

Run: `deploy.bat`
Expected: VST3 copied to `C:\Program Files\Common Files\VST3\AI Synth.vst3`.

- [ ] **Step 4: Manual DAW check (user)**

Open SynthVibe in a DAW. Verify:
- Sound Tab opens at 1280×720 without distortion
- OSC1 / OSC2 panels show a scope with slow drift; changing the waveform dropdown changes the drawn shape immediately
- FILTER panel shows a magnitude curve; dragging Cutoff or Res updates the curve
- LP/HP/BP pills are mutually exclusive and change the curve shape
- AMP ENV / FLT ENV show four nodes; dragging each node changes the matching param value; releasing updates undo (one entry per gesture)
- Audio still sounds correct when playing; no clicks on noteOff (exponential release)

- [ ] **Step 5: Tag the phase**

```bash
git tag phase2-sound-tab
git push --tags
```

(Optional: skip the tag push if the user prefers local tags only.)

---

## Self-review checklist

- **Spec coverage:** all 7 new components covered by Tasks 3–9, envelope refinement by Task 0, breakpoint scaffold by Task 2, SoundTab rewrite by Task 10, PluginEditor wiring by Task 11. Deferred items (Phase 2b) are excluded as the spec demands.
- **Correction:** the spec's "extract RBJ from Filter.cpp" claim is reconciled at the top — `FilterCoefficients.h` is a visualization-only helper, `Filter.cpp` is untouched.
- **Placeholders:** none. Every step has exact code / commands / expected output.
- **Type consistency:** `SynthVibe::BP`, `FilterCoefficients::Type`, `ParameterAttachment` usage consistent across tasks. `ArcKnob` constructor signature matches Phase 1 (`label, apvts, id, colour, suffix, decimals`).
- **Risk: EnvelopeEditor uses `ParameterAttachment::beginGesture`/`endGesture`** — this is the documented JUCE API (no `beginChangeGesture` on `ParameterAttachment`; that's the old `AudioParameterFloat` API). `setValueAsPartOfGesture` is the correct setter during drag.
- **Risk: filter response refreshes at 30 Hz** — acceptable since biquad math is ~20 flops × 120 points = 2400 flops per repaint, negligible.

---

## Execution handoff

Plan complete. Two execution options:

1. **Subagent-Driven (recommended)** — I dispatch a fresh subagent per task, review between tasks, fast iteration.
2. **Inline Execution** — Execute tasks in this session using `superpowers:executing-plans`, batch execution with checkpoints.

Which approach?
