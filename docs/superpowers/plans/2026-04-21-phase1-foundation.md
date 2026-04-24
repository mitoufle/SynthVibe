# Phase 1 — Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Establish the visual foundation for the new SynthVibe design — design tokens (Night Plum palette), font loader, a new LookAndFeel with ArcKnob geometry, and a redesigned top bar composed of new atomic components. Existing synth tabs stay untouched this phase; they'll be re-skinned in Phase 2.

**Architecture:** Three layers. (1) **Tokens layer** — a header-only `DesignTokens.h` holds all colours/spacing/radii/motion constants; a `Fonts.h` returns JUCE `Font` objects with graceful fallback to JUCE defaults when `.ttf` BinaryData is absent. (2) **Look-and-feel layer** — `SynthLookAndFeel` overrides `drawRotarySlider` with the spec's 225°/270° arc. (3) **Component layer** — a new `Source/UI/components/` directory holds small, state-owning UI atoms (`ArcKnob`, `BrandBadge`, `PresetNameField`, `PromptButton`, `NavArrowButton`, `MasterVolKnob`). `TopBar` is rewritten to compose them.

Before touching UI we migrate all parameter IDs from snake-case (`osc1_waveform`) to the dot-separated scheme the design and AI contract require (`osc1.wave`). Existing synth-engine wiring is unaffected because the code references IDs through named constants — only the string *values* change.

**Tech Stack:** C++17, JUCE 7.0.9, CMake. Tests use `juce::UnitTest` (already wired as `AISynthTests` console app in CMakeLists.txt). No new dependencies.

**Palette:** Night Plum (from `AISynth V1 Hi-Fi.html` PALETTES.plum).

---

## File Structure

**New files:**
- `Source/UI/DesignTokens.h` — colour / spacing / radii / motion constants (header-only)
- `Source/UI/Fonts.h` — font accessors with BinaryData fallback
- `Source/UI/components/ArcKnob.h` — the design-spec knob (slider + label + value readout)
- `Source/UI/components/BrandBadge.h` — logo + wordmark + version tag
- `Source/UI/components/PresetNameField.h` — preset label + title + metadata line
- `Source/UI/components/PromptButton.h` — gold pill, fires a callback (AI modal wired in Phase 4)
- `Source/UI/components/NavArrowButton.h` — single ◀ or ▶ button, used twice in the TopBar
- `Source/UI/components/MasterVolKnob.h` — small (26 px) ArcKnob variant
- `Tests/ParameterIdMigrationTests.cpp` — one-shot check that every renamed ID binds
- `Tests/DesignTokensTests.cpp` — sanity check for palette values
- `Tests/UIConstructionTests.cpp` — headless construction tests for new components

**Modified files:**
- `Source/Parameters/ParameterIDs.h` — string values swapped to dot-separated; constant names unchanged
- `Source/UI/LookAndFeel.h` — rewritten to use tokens and spec arc geometry
- `Source/UI/TopBar.h` — rewritten to compose the new components
- `Source/PluginEditor.cpp` — default size `1280 × 720`, background uses `Tokens::bg`
- `CMakeLists.txt` — add new test files

**Left alone this phase:** `SoundTab.h`, `ModTab.h`, `FXTab.h`, `ArpTab.h`, `KnobWithLabel.h`, `BottomBar.h`. They still render; they get Phase 2's overhaul.

---

## Task 0: Parameter ID migration

**Goal:** Rename every APVTS parameter ID string to the dot-separated scheme. Constant *names* in `ParamIDs` stay the same so no other file changes.

**Files:**
- Modify: `Source/Parameters/ParameterIDs.h`
- Create: `Tests/ParameterIdMigrationTests.cpp`
- Modify: `CMakeLists.txt` (add test file)

- [ ] **Step 1: Write the failing test**

Create `Tests/ParameterIdMigrationTests.cpp`:

```cpp
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters/ParameterLayout.h"
#include "Parameters/ParameterIDs.h"

struct ParameterIdMigrationTests : public juce::UnitTest
{
    ParameterIdMigrationTests() : juce::UnitTest("ParameterIdMigration", "AISynth") {}

    void runTest() override
    {
        beginTest("IDs use dot-separated scheme");
        expectEquals(juce::String(ParamIDs::osc1Waveform),  juce::String("osc1.wave"));
        expectEquals(juce::String(ParamIDs::filterCutoff),  juce::String("filt.cutoff"));
        expectEquals(juce::String(ParamIDs::ampAttack),     juce::String("env.amp.attack"));
        expectEquals(juce::String(ParamIDs::masterVolume),  juce::String("master.vol"));
        expectEquals(juce::String(ParamIDs::delayTime),     juce::String("fx.delay.time"));
    }
};

static ParameterIdMigrationTests sParameterIdMigrationTests;
```

- [ ] **Step 2: Add test to CMake and run — verify it fails**

Edit `CMakeLists.txt`, in the `AISynthTests` `target_sources` list add `Tests/ParameterIdMigrationTests.cpp` right after `Tests/VoiceTests.cpp`.

Run:

```bash
./build.bat && ./build/AISynthTests_artefacts/Release/AISynth\ Tests.exe
```

Expected: `ParameterIdMigration` test fails — actual values still start with `osc1_waveform`.

- [ ] **Step 3: Rewrite ParameterIDs.h**

Replace the whole file contents with:

```cpp
#pragma once

// All parameter IDs in one place. IDs are dot-separated per the design contract
// (docs/ClaudeDesign/docs/param-audit.md). Do NOT rename string values once the
// plugin ships — user presets bind by string ID and will lose their values on rename.
namespace ParamIDs
{
    // Oscillator 1
    inline constexpr const char* osc1Waveform  = "osc1.wave";
    inline constexpr const char* osc1Octave    = "osc1.octave";
    inline constexpr const char* osc1Semitone  = "osc1.semi";
    inline constexpr const char* osc1Detune    = "osc1.fine";
    inline constexpr const char* osc1Level     = "osc1.level";

    // Oscillator 2
    inline constexpr const char* osc2Waveform  = "osc2.wave";
    inline constexpr const char* osc2Octave    = "osc2.octave";
    inline constexpr const char* osc2Semitone  = "osc2.semi";
    inline constexpr const char* osc2Detune    = "osc2.fine";
    inline constexpr const char* osc2Level     = "osc2.level";

    // LFO 1
    inline constexpr const char* lfo1Shape     = "lfo1.wave";
    inline constexpr const char* lfo1Rate      = "lfo1.rate";
    inline constexpr const char* lfo1Depth     = "lfo1.depth";
    inline constexpr const char* lfo1Dest      = "lfo1.dest";

    // LFO 2
    inline constexpr const char* lfo2Shape     = "lfo2.wave";
    inline constexpr const char* lfo2Rate      = "lfo2.rate";
    inline constexpr const char* lfo2Depth     = "lfo2.depth";
    inline constexpr const char* lfo2Dest      = "lfo2.dest";

    // Filter
    inline constexpr const char* filterType      = "filt.type";
    inline constexpr const char* filterCutoff    = "filt.cutoff";
    inline constexpr const char* filterResonance = "filt.res";
    inline constexpr const char* filterEnvAmt    = "filt.envamt";

    // Amp envelope
    inline constexpr const char* ampAttack       = "env.amp.attack";
    inline constexpr const char* ampDecay        = "env.amp.decay";
    inline constexpr const char* ampSustain      = "env.amp.sustain";
    inline constexpr const char* ampRelease      = "env.amp.release";

    // Filter envelope
    inline constexpr const char* fltAttack       = "env.filt.attack";
    inline constexpr const char* fltDecay        = "env.filt.decay";
    inline constexpr const char* fltSustain      = "env.filt.sustain";
    inline constexpr const char* fltRelease      = "env.filt.release";

    // Master
    inline constexpr const char* masterVolume    = "master.vol";

    // Delay (interim — Phase 3 migrates these into the generic fx.N slot scheme)
    inline constexpr const char* delayTime       = "fx.delay.time";
    inline constexpr const char* delayFeedback   = "fx.delay.feedback";
    inline constexpr const char* delayMix        = "fx.delay.mix";

    // Chorus
    inline constexpr const char* chorusRate      = "fx.chorus.rate";
    inline constexpr const char* chorusDepth     = "fx.chorus.depth";
    inline constexpr const char* chorusMix       = "fx.chorus.mix";

    // Reverb
    inline constexpr const char* reverbRoom      = "fx.reverb.room";
    inline constexpr const char* reverbDamp      = "fx.reverb.damp";
    inline constexpr const char* reverbMix       = "fx.reverb.mix";

    // Drive
    inline constexpr const char* driveType       = "fx.drive.type";
    inline constexpr const char* driveAmount     = "fx.drive.amount";
    inline constexpr const char* driveMix        = "fx.drive.mix";

    // Unison
    inline constexpr const char* unisonVoices       = "osc1.uni.voices";
    inline constexpr const char* unisonDetune       = "osc1.uni.detune";
    inline constexpr const char* unisonStereoSpread = "osc1.uni.spread";

    // Arp
    inline constexpr const char* arpEnabled      = "arp.on";
    inline constexpr const char* arpMode         = "arp.pattern";
    inline constexpr const char* arpRate         = "arp.rate";
    inline constexpr const char* arpOctaveRange  = "arp.octaves";
}
```

- [ ] **Step 4: Run all tests — verify ParameterIdMigration passes and engine tests still pass**

```bash
./build.bat && ./build/AISynthTests_artefacts/Release/AISynth\ Tests.exe
```

Expected: all green. Engine tests (`VoiceTests`, `SynthEngineTests`, `ArpEngineTests`) use `Voice` / `SynthEngine` directly and don't touch IDs, so they're unaffected.

- [ ] **Step 5: Build the plugin and smoke-test in DAW**

```bash
./build.bat && ./deploy.bat
```

Open DAW → load AI Synth → play a note → confirm sound. If anything sounds wrong, `buildVoiceParams()` is reading by constant-name, not by string literal, so any failure here is a test-compile issue, not a run-time one.

- [ ] **Step 6: Commit**

```bash
git add Source/Parameters/ParameterIDs.h Tests/ParameterIdMigrationTests.cpp CMakeLists.txt
git commit -m "refactor: migrate parameter IDs to dot-separated scheme (Phase 1, Task 0)"
```

---

## Task 1: Install DesignTokens.h with corrected Night Plum values

**Goal:** Drop a single header of all design constants into `Source/UI/` so every new UI file can `#include "DesignTokens.h"` and reference colours/spacing/motion by name. The copy in `docs/ClaudeDesign/Source/UI/DesignTokens.h` has the plum *name* but Ember Gold *accent* values — we correct that here from the authoritative HTML palette.

**Files:**
- Create: `Source/UI/DesignTokens.h`
- Create: `Tests/DesignTokensTests.cpp`
- Modify: `CMakeLists.txt` (add test file)

- [ ] **Step 1: Write the failing test**

Create `Tests/DesignTokensTests.cpp`:

```cpp
#include <juce_core/juce_core.h>
#include "UI/DesignTokens.h"

struct DesignTokensTests : public juce::UnitTest
{
    DesignTokensTests() : juce::UnitTest("DesignTokens", "AISynth") {}

    void runTest() override
    {
        beginTest("Night Plum palette values");
        using namespace SynthVibe::Tokens;
        expectEquals(bg.getARGB(),     (juce::uint32) 0xFF0F0D14);
        expectEquals(panel.getARGB(),  (juce::uint32) 0xFF171421);
        expectEquals(accent.getARGB(), (juce::uint32) 0xFFC693E8);
        expectEquals(osc.getARGB(),    (juce::uint32) 0xFF9ABFE8);
        expectEquals(filter.getARGB(), (juce::uint32) 0xFFE8A3C7);

        beginTest("Arc geometry matches spec (225°/270°)");
        expectWithinAbsoluteError(knobArcStartDeg, 225.0f, 0.001f);
        expectWithinAbsoluteError(knobArcSweepDeg, 270.0f, 0.001f);
    }
};

static DesignTokensTests sDesignTokensTests;
```

- [ ] **Step 2: Add to CMake and run — verify it fails to compile (header doesn't exist yet)**

Edit `CMakeLists.txt`, in the `AISynthTests` sources add `Tests/DesignTokensTests.cpp`.

Run:

```bash
./build.bat
```

Expected: compile error — `UI/DesignTokens.h: No such file or directory`.

- [ ] **Step 3: Create `Source/UI/DesignTokens.h`**

```cpp
#pragma once
#include <juce_graphics/juce_graphics.h>

// SynthVibe — Design Tokens
// Night Plum palette. Sourced from docs/ClaudeDesign/AISynth V1 Hi-Fi.html PALETTES.plum.
// Changing a colour here should be mirrored in that mockup first.
namespace SynthVibe::Tokens
{
    // Surfaces
    inline const juce::Colour bg       { 0xFF0F0D14 };
    inline const juce::Colour panel    { 0xFF171421 };
    inline const juce::Colour panel2   { 0xFF1F1B2B };
    inline const juce::Colour edge     { 0xFF2D2839 };
    inline const juce::Colour edge2    { 0xFF40394F };

    // Ink (text)
    inline const juce::Colour ink      { 0xFFEBE5F0 };
    inline const juce::Colour ink2     { 0xFFB0A6BB };
    inline const juce::Colour ink3     { 0xFF7A7287 };
    inline const juce::Colour ink4     { 0xFF4A4553 };

    // Accent (Night Plum = violet)
    inline const juce::Colour accent   { 0xFFC693E8 };
    inline const juce::Colour accentHi { 0xFFD4ABF0 };   // hover — lightened 10%
    inline const juce::Colour accent2  { 0xFF8C5FA8 };

    // Section hues
    inline const juce::Colour osc      { 0xFF9ABFE8 };   // blue
    inline const juce::Colour filter   { 0xFFE8A3C7 };   // pink
    inline const juce::Colour env      { 0xFFA0D4A8 };   // sage green
    inline const juce::Colour lfo      { 0xFFC693E8 };   // violet (same as accent)
    inline const juce::Colour fx       { 0xFF86D4D4 };   // teal
    inline const juce::Colour arp      { 0xFFE8C06A };   // gold (section-specific)

    // Mod bars
    inline const juce::Colour modPositive { 0xFFC693E8 };
    inline const juce::Colour modNegative { 0xFFE8A3C7 };

    // Spacing
    constexpr int spaceXs = 4;
    constexpr int spaceSm = 8;
    constexpr int spaceMd = 12;
    constexpr int spaceLg = 18;
    constexpr int spaceXl = 24;

    // Radii
    constexpr float radiusSm = 4.0f;
    constexpr float radiusMd = 6.0f;
    constexpr float radiusLg = 10.0f;

    // Knob geometry
    constexpr float knobArcStartDeg   = 225.0f;
    constexpr float knobArcSweepDeg   = 270.0f;
    constexpr float knobArcThickness  = 2.5f;
    constexpr float knobInnerBodyInset = 5.0f;
    constexpr int   knobSizeCompact   = 38;
    constexpr int   knobSizeDefault   = 46;
    constexpr int   knobSizeWide      = 52;

    // Typography sizes
    namespace Font
    {
        constexpr float presetTitle = 17.0f;
        constexpr float panelTitle  = 10.0f;
        constexpr float fxName      = 16.0f;
        constexpr float body        = 12.0f;
        constexpr float label       = 9.0f;
        constexpr float value       = 9.0f;
        constexpr float tiny        = 8.0f;
    }

    // Font family roles
    namespace Family
    {
        inline const juce::String mono  = "JetBrains Mono";
        inline const juce::String sans  = "Inter";
        inline const juce::String serif = "Instrument Serif";
    }

    // Motion (ms)
    namespace Motion
    {
        constexpr int knobDragMs     = 80;
        constexpr int knobTweenMs    = 400;
        constexpr int tickFlashMs    = 140;
        constexpr int resetFlashMs   = 500;
        constexpr int applyFlashMs   = 600;
        constexpr int rippleMs       = 900;
        constexpr int modalOpenMs    = 280;
        constexpr int drawerSlideMs  = 280;
        constexpr int flipSortMs     = 380;
        constexpr int ambientScopeMs = 3200;
        constexpr int lfoDotMs       = 3200;
    }

    // Keyboard
    namespace Keyboard
    {
        constexpr int minHeightCompact = 38;
        constexpr int minHeightDefault = 42;
        constexpr int minHeightWide    = 48;
        constexpr int octavesCompact = 3;
        constexpr int octavesDefault = 4;
        constexpr int octavesWide    = 5;
    }
}
```

- [ ] **Step 4: Run tests — verify DesignTokens passes**

```bash
./build.bat && ./build/AISynthTests_artefacts/Release/AISynth\ Tests.exe
```

Expected: all green.

- [ ] **Step 5: Commit**

```bash
git add Source/UI/DesignTokens.h Tests/DesignTokensTests.cpp CMakeLists.txt
git commit -m "feat: add DesignTokens.h with Night Plum palette (Phase 1, Task 1)"
```

---

## Task 2: Fonts.h accessor with graceful fallback

**Goal:** One place to ask for "the mono font", "the sans font", "the serif font". When `.ttf` files aren't yet packaged via `juce_add_binary_data`, fall back to JUCE's platform defaults so the UI still renders. When the user later drops files in and re-runs CMake, the accessors pick them up with zero code changes.

**Files:**
- Create: `Source/UI/Fonts.h`

- [ ] **Step 1: Create `Source/UI/Fonts.h`**

```cpp
#pragma once
#include <juce_graphics/juce_graphics.h>
#include "DesignTokens.h"

// When the user drops JetBrains Mono / Inter / Instrument Serif .ttf files
// into Resources/Fonts/ and re-runs CMake with juce_add_binary_data, these
// accessors will resolve to the bundled files. Until then they fall back
// to JUCE's platform defaults so nothing breaks.
namespace SynthVibe::Fonts
{
    inline juce::Font mono (float height = Tokens::Font::body)
    {
        // TODO(font-embedding): replace with juce::Typeface::createSystemTypefaceFor(
        //     BinaryData::JetBrainsMono_Regular_ttf, BinaryData::JetBrainsMono_Regular_ttfSize)
        // once Resources/Fonts/JetBrainsMono-Regular.ttf is added to juce_add_binary_data.
        return juce::Font(juce::Font::getDefaultMonospacedFontName(), height, juce::Font::plain);
    }

    inline juce::Font sans (float height = Tokens::Font::body, int style = juce::Font::plain)
    {
        // TODO(font-embedding): replace with Inter BinaryData.
        return juce::Font(juce::Font::getDefaultSansSerifFontName(), height, style);
    }

    inline juce::Font serif (float height = Tokens::Font::presetTitle, int style = juce::Font::italic)
    {
        // TODO(font-embedding): replace with Instrument Serif BinaryData.
        return juce::Font(juce::Font::getDefaultSerifFontName(), height, style);
    }
}
```

- [ ] **Step 2: Verify build**

```bash
./build.bat
```

Expected: builds cleanly.

- [ ] **Step 3: Commit**

```bash
git add Source/UI/Fonts.h
git commit -m "feat: add Fonts.h with platform-default fallbacks (Phase 1, Task 2)"
```

---

## Task 3: Rewrite SynthLookAndFeel using tokens + spec arc geometry

**Goal:** Replace the old navy/red palette wired into `LookAndFeel.h` with the token-based Night Plum palette, and replace the generic `drawRotarySlider` with the design's 225°/270° sweep, 2.5 px stroke, inner body disc.

**Files:**
- Modify: `Source/UI/LookAndFeel.h`

- [ ] **Step 1: Replace `Source/UI/LookAndFeel.h` with the token-driven version**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "DesignTokens.h"
#include "Fonts.h"

// SynthLookAndFeel applies the Night Plum palette and the spec knob geometry.
// Legacy colour constants (colBackground, colPanel, etc.) are preserved as
// aliases so existing tabs (SoundTab, ModTab, FXTab, ArpTab, BottomBar)
// keep compiling until Phase 2 migrates them.
class SynthLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Legacy aliases — remove in Phase 2 once all tabs use Tokens directly.
    static constexpr auto colBackground  = 0xFF0F0D14;
    static constexpr auto colPanel       = 0xFF171421;
    static constexpr auto colAccent      = 0xFF1F1B2B;
    static constexpr auto colHighlight   = 0xFFC693E8;
    static constexpr auto colKnobTrack   = 0xFF2D2839;
    static constexpr auto colText        = 0xFFEBE5F0;
    static constexpr auto colTextDim     = 0xFF7A7287;
    static constexpr auto colOscAccent    = 0xFF9ABFE8;
    static constexpr auto colFilterAccent = 0xFFE8A3C7;
    static constexpr auto colEnvAccent    = 0xFFA0D4A8;
    static constexpr auto colLfoAccent    = 0xFFC693E8;
    static constexpr auto colFxAccent     = 0xFF86D4D4;
    static constexpr auto colArpAccent    = 0xFFE8C06A;

    SynthLookAndFeel()
    {
        using namespace SynthVibe::Tokens;
        setColour(juce::Slider::rotarySliderFillColourId,        accent);
        setColour(juce::Slider::rotarySliderOutlineColourId,     edge);
        setColour(juce::Slider::thumbColourId,                   ink);
        setColour(juce::Label::textColourId,                     ink);
        setColour(juce::ComboBox::backgroundColourId,            panel2);
        setColour(juce::ComboBox::textColourId,                  ink);
        setColour(juce::ComboBox::outlineColourId,               edge);
        setColour(juce::PopupMenu::backgroundColourId,           panel);
        setColour(juce::PopupMenu::textColourId,                 ink);
        setColour(juce::PopupMenu::highlightedBackgroundColourId,accent);
        setDefaultSansSerifTypefaceName(SynthVibe::Fonts::sans(SynthVibe::Tokens::Font::body).getTypefaceName());
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour&, bool, bool) override
    {
        using namespace SynthVibe::Tokens;
        auto bounds = button.getLocalBounds().toFloat().reduced(1.f);
        if (button.getToggleState())
        {
            g.setColour(accent);
            g.fillRoundedRectangle(bounds, radiusSm);
        }
        else
        {
            g.setColour(panel2);
            g.fillRoundedRectangle(bounds, radiusSm);
            g.setColour(edge2);
            g.drawRoundedRectangle(bounds, radiusSm, 1.f);
        }
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool, bool) override
    {
        using namespace SynthVibe::Tokens;
        g.setFont(SynthVibe::Fonts::mono(Font::label));
        g.setColour(button.getToggleState() ? juce::Colour(0xFF1A1A1A) : ink2);
        g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float /*startAngle*/, float /*endAngle*/,
                          juce::Slider& slider) override
    {
        using namespace SynthVibe::Tokens;

        const float size     = (float) juce::jmin(width, height);
        const float cx       = x + width  * 0.5f;
        const float cy       = y + height * 0.5f;
        const float rOuter   = size * 0.5f - 1.f;
        const float rInner   = rOuter - knobInnerBodyInset;

        const float startRad = juce::degreesToRadians(knobArcStartDeg) - juce::MathConstants<float>::halfPi;
        const float sweepRad = juce::degreesToRadians(knobArcSweepDeg);
        const float valueRad = startRad + sliderPos * sweepRad;

        // Track arc
        juce::Path track;
        track.addCentredArc(cx, cy, rOuter, rOuter, 0.f, startRad, startRad + sweepRad, true);
        g.setColour(edge);
        g.strokePath(track, juce::PathStrokeType(knobArcThickness));

        // Value arc
        juce::Path value;
        value.addCentredArc(cx, cy, rOuter, rOuter, 0.f, startRad, valueRad, true);
        const auto arcColour = slider.findColour(juce::Slider::rotarySliderFillColourId);
        g.setColour(arcColour);
        g.strokePath(value, juce::PathStrokeType(knobArcThickness));

        // Inner body
        g.setColour(panel2);
        g.fillEllipse(cx - rInner, cy - rInner, rInner * 2.f, rInner * 2.f);
        g.setColour(edge);
        g.drawEllipse(cx - rInner, cy - rInner, rInner * 2.f, rInner * 2.f, 1.f);

        // Indicator line
        const float tickOuter = rInner - 2.f;
        const float tickInner = rInner * 0.55f;
        juce::Path tick;
        tick.startNewSubPath(cx + tickInner * std::cos(valueRad),
                             cy + tickInner * std::sin(valueRad));
        tick.lineTo         (cx + tickOuter * std::cos(valueRad),
                             cy + tickOuter * std::sin(valueRad));
        g.setColour(arcColour);
        g.strokePath(tick, juce::PathStrokeType(1.5f));
    }
};
```

- [ ] **Step 2: Build and deploy, load in DAW**

```bash
./build.bat && ./deploy.bat
```

Open DAW → load AI Synth → the existing tabs now render with Night Plum background, purple accent knobs, and the new arc geometry (225° → 135° sweep). Existing `KnobWithLabel` instances inherit the new `drawRotarySlider` automatically.

- [ ] **Step 3: Commit**

```bash
git add Source/UI/LookAndFeel.h
git commit -m "feat: rewrite LookAndFeel with Night Plum palette and spec arc geometry (Phase 1, Task 3)"
```

---

## Task 4: ArcKnob component

**Goal:** A new component that will be the canonical knob going forward. Same attachment mechanism as `KnobWithLabel` (so Phase 2 can migrate SoundTab callsites one at a time), but uses the new fonts, colour tokens, and takes an optional section-accent colour.

**Files:**
- Create: `Source/UI/components/ArcKnob.h`
- Create: `Tests/UIConstructionTests.cpp`
- Modify: `CMakeLists.txt` (add test file; headless JUCE component tests need `juce_gui_basics`)

- [ ] **Step 1: Write the failing test**

Create `Tests/UIConstructionTests.cpp`:

```cpp
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters/ParameterLayout.h"
#include "Parameters/ParameterIDs.h"
#include "UI/components/ArcKnob.h"

struct UIConstructionTests : public juce::UnitTest
{
    UIConstructionTests() : juce::UnitTest("UIConstruction", "AISynth") {}

    void runTest() override
    {
        beginTest("ArcKnob constructs and attaches to APVTS");

        juce::AudioProcessorGraph dummyGraph;
        juce::AudioProcessorValueTreeState apvts(
            dummyGraph, nullptr, "AISynthState", ParameterLayout::create());

        SynthVibe::ArcKnob knob("Cutoff", apvts, ParamIDs::filterCutoff,
                                SynthVibe::Tokens::filter, " Hz", 0);
        knob.setBounds(0, 0, 60, 80);

        expectEquals(knob.getSlider().getValue(), 8000.0);    // default from ParameterLayout
    }
};

static UIConstructionTests sUIConstructionTests;
```

UI construction needs a message thread. Update `Tests/main.cpp`:

```cpp
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_gui_basics/juce_gui_basics.h>

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;    // spins up MessageManager for UI-using tests

    juce::UnitTestRunner runner;
    runner.runAllTests();

    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        failures += runner.getResult(i)->failures;

    return failures > 0 ? 1 : 0;
}
```

Edit `CMakeLists.txt` — the `AISynthTests` target needs to gain several things at once:

1. Add `Tests/UIConstructionTests.cpp` to `AISynthTests` sources.
2. Add `Source/Parameters/ParameterLayout.cpp` to `AISynthTests` sources (currently absent).
3. Add to `AISynthTests` link libraries: `juce::juce_gui_basics`, `juce::juce_events`, `juce::juce_audio_processors` (all currently absent).

- [ ] **Step 2: Run — verify it fails (header missing)**

```bash
./build.bat
```

Expected: compile error `UI/components/ArcKnob.h: No such file or directory`.

- [ ] **Step 3: Create `Source/UI/components/ArcKnob.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class ArcKnob : public juce::Component
    {
    public:
        ArcKnob(const juce::String& labelText,
                juce::AudioProcessorValueTreeState& apvts,
                const juce::String& paramID,
                juce::Colour arcColour    = Tokens::accent,
                const juce::String& suffix = "",
                int decimalPlaces          = 2)
        {
            label.setText(labelText.toUpperCase(), juce::dontSendNotification);
            label.setJustificationType(juce::Justification::centredTop);
            label.setFont(Fonts::mono(Tokens::Font::label));
            label.setColour(juce::Label::textColourId, Tokens::ink3);
            addAndMakeVisible(label);

            slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
            slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 14);
            slider.setColour(juce::Slider::rotarySliderFillColourId, arcColour);
            slider.setColour(juce::Slider::textBoxTextColourId, Tokens::ink2);
            slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            slider.setTextValueSuffix(suffix);
            slider.setNumDecimalPlacesToDisplay(decimalPlaces);
            addAndMakeVisible(slider);

            attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, paramID, slider);
        }

        juce::Slider&       getSlider()       { return slider; }
        const juce::Slider& getSlider() const { return slider; }

        void resized() override
        {
            auto b = getLocalBounds();
            label.setBounds(b.removeFromTop(12));
            slider.setBounds(b);
        }

    private:
        juce::Label  label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
}
```

- [ ] **Step 4: Run tests — verify green**

```bash
./build.bat && ./build/AISynthTests_artefacts/Release/AISynth\ Tests.exe
```

Expected: `UIConstruction::ArcKnob constructs and attaches to APVTS` passes.

- [ ] **Step 5: Commit**

```bash
git add Source/UI/components/ArcKnob.h Tests/UIConstructionTests.cpp CMakeLists.txt
git commit -m "feat: add ArcKnob component (Phase 1, Task 4)"
```

---

## Task 5: BrandBadge component

**Goal:** The logo + wordmark + version tag shown top-left.

**Files:**
- Create: `Source/UI/components/BrandBadge.h`

- [ ] **Step 1: Create the file**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class BrandBadge : public juce::Component
    {
    public:
        explicit BrandBadge(const juce::String& versionTag = "V1")
            : version(versionTag) {}

        void paint(juce::Graphics& g) override
        {
            auto b = getLocalBounds();
            const int dotSize = 18;
            auto dot = b.removeFromLeft(dotSize).withSizeKeepingCentre(dotSize, dotSize);

            // Conic-gradient-like look: two-tone filled disc.
            juce::ColourGradient grad(Tokens::accent,   dot.getX() + dotSize * 0.3f, dot.getY(),
                                      Tokens::accent2,  dot.getRight(),              dot.getBottom(),
                                      false);
            g.setGradientFill(grad);
            g.fillEllipse(dot.toFloat());
            g.setColour(Tokens::panel);
            g.fillEllipse(dot.toFloat().reduced(3.f));

            b.removeFromLeft(Tokens::spaceSm);

            auto wordmark = b.removeFromLeft(120);
            g.setColour(Tokens::ink);
            g.setFont(Fonts::serif(20.f, juce::Font::plain));
            g.drawText("AI", wordmark.removeFromLeft(22), juce::Justification::centredLeft);
            g.setColour(Tokens::accent);
            g.setFont(Fonts::serif(20.f, juce::Font::italic));
            g.drawText("Synth", wordmark, juce::Justification::centredLeft);

            g.setColour(Tokens::ink3);
            g.setFont(Fonts::mono(Tokens::Font::label));
            g.drawText(version, b, juce::Justification::centredLeft);
        }

    private:
        juce::String version;
    };
}
```

- [ ] **Step 2: Build**

```bash
./build.bat
```

Expected: compiles cleanly (no test yet — pure paint code).

- [ ] **Step 3: Commit**

```bash
git add Source/UI/components/BrandBadge.h
git commit -m "feat: add BrandBadge component (Phase 1, Task 5)"
```

---

## Task 6: PresetNameField component

**Goal:** The centred preset label + italic title + small metadata line. For Phase 1 it's display-only; inline rename is Phase 5.

**Files:**
- Create: `Source/UI/components/PresetNameField.h`

- [ ] **Step 1: Create the file**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class PresetNameField : public juce::Component
    {
    public:
        void setPresetName (const juce::String& n) { name = n; repaint(); }
        void setMetaText   (const juce::String& m) { meta = m; repaint(); }
        void setDirty      (bool d)                { dirty = d; repaint(); }

        void paint(juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat();
            g.setColour(juce::Colour(0x33000000));
            g.fillRoundedRectangle(b, Tokens::radiusLg);
            g.setColour(Tokens::edge);
            g.drawRoundedRectangle(b, Tokens::radiusLg, 1.f);

            auto inner = getLocalBounds().reduced(14, 4);
            auto labelRow = inner.removeFromTop(12);
            g.setColour(Tokens::ink4);
            g.setFont(Fonts::mono(Tokens::Font::label));
            g.drawText("PRESET", labelRow, juce::Justification::centred);

            auto titleRow = inner.removeFromTop(20);
            g.setColour(Tokens::ink);
            g.setFont(Fonts::serif(Tokens::Font::presetTitle, juce::Font::italic));
            const auto display = dirty ? name + " " + juce::String(juce::CharPointer_UTF8("\xE2\x97\x8F"))
                                       : name;
            g.drawText(display.isEmpty() ? "Init" : display, titleRow, juce::Justification::centred);

            g.setColour(Tokens::ink3);
            g.setFont(Fonts::mono(Tokens::Font::tiny));
            g.drawText(meta, inner, juce::Justification::centred);
        }

    private:
        juce::String name, meta;
        bool dirty { false };
    };
}
```

- [ ] **Step 2: Build and commit**

```bash
./build.bat
git add Source/UI/components/PresetNameField.h
git commit -m "feat: add PresetNameField component (Phase 1, Task 6)"
```

---

## Task 7: PromptButton component

**Goal:** Gold (accent) pill labelled `✦ prompt`. Fires `onClick`. Actual AI-modal wiring is Phase 4; this phase just needs the visual.

**Files:**
- Create: `Source/UI/components/PromptButton.h`

- [ ] **Step 1: Create the file**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class PromptButton : public juce::Button
    {
    public:
        PromptButton() : juce::Button("prompt") {}

        void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override
        {
            auto b = getLocalBounds().toFloat().reduced(1.f);
            auto fill = isButtonDown ? Tokens::accentHi.darker(0.1f)
                      : isMouseOver  ? Tokens::accentHi
                      :                Tokens::accent;
            g.setColour(fill);
            g.fillRoundedRectangle(b, Tokens::radiusSm);

            g.setColour(juce::Colour(0xFF1A1A1A));
            g.setFont(Fonts::mono(Tokens::Font::label).withStyle(juce::Font::bold));
            const juce::String text = juce::String(juce::CharPointer_UTF8("\xE2\x9C\xA6")) + "  PROMPT";
            g.drawText(text, getLocalBounds(), juce::Justification::centred);
        }
    };
}
```

- [ ] **Step 2: Build and commit**

```bash
./build.bat
git add Source/UI/components/PromptButton.h
git commit -m "feat: add PromptButton component (Phase 1, Task 7)"
```

---

## Task 8: NavArrowButton component

**Goal:** A single arrow button (◀ or ▶) for preset prev/next navigation. Used in two instances in the TopBar, one on each side of the preset name field. Single-arrow rather than a composite so the TopBar can lay them out independently around the name field without bounds overlap.

**Files:**
- Create: `Source/UI/components/NavArrowButton.h`

- [ ] **Step 1: Create the file**

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../DesignTokens.h"

namespace SynthVibe
{
    class NavArrowButton : public juce::Component
    {
    public:
        enum class Direction { Prev, Next };

        std::function<void()> onClick;

        explicit NavArrowButton(Direction dir)
        {
            const char* glyph = (dir == Direction::Prev)
                ? "\xE2\x97\x80"   // ◀
                : "\xE2\x96\xB6";  // ▶
            button.setButtonText(juce::String(juce::CharPointer_UTF8(glyph)));
            button.setColour(juce::TextButton::buttonColourId, Tokens::panel2);
            button.setColour(juce::TextButton::textColourOffId, Tokens::ink2);
            button.onClick = [this] { if (onClick) onClick(); };
            addAndMakeVisible(button);
        }

        // Overrides juce::Component::setEnabled so the whole component (and the inner
        // TextButton) track enabled-state together, rather than silently shadowing it.
        void setEnabled(bool shouldBeEnabled) override
        {
            juce::Component::setEnabled(shouldBeEnabled);
            button.setEnabled(shouldBeEnabled);
        }

        void resized() override
        {
            button.setBounds(getLocalBounds());
        }

    private:
        juce::TextButton button;
    };
}
```

- [ ] **Step 2: Build and commit**

```bash
./build.bat
git add Source/UI/components/NavArrowButton.h
git commit -m "feat: add NavArrowButton component (Phase 1, Task 8)"
```

---

## Task 9: MasterVolKnob component

**Goal:** Smaller (26 px) knob for the top-bar master volume, using the same attachment mechanism as ArcKnob.

**Files:**
- Create: `Source/UI/components/MasterVolKnob.h`

- [ ] **Step 1: Create the file**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class MasterVolKnob : public juce::Component
    {
    public:
        MasterVolKnob(juce::AudioProcessorValueTreeState& apvts,
                      const juce::String& paramID)
        {
            label.setText("VOL", juce::dontSendNotification);
            label.setFont(Fonts::mono(Tokens::Font::label));
            label.setColour(juce::Label::textColourId, Tokens::ink4);
            label.setJustificationType(juce::Justification::centredRight);
            addAndMakeVisible(label);

            slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
            slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            slider.setColour(juce::Slider::rotarySliderFillColourId, Tokens::accent);
            addAndMakeVisible(slider);

            attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                apvts, paramID, slider);
        }

        void resized() override
        {
            auto b = getLocalBounds();
            auto knobBox = b.removeFromRight(Tokens::knobSizeCompact - 12);
            slider.setBounds(knobBox);
            label.setBounds(b);
        }

    private:
        juce::Label  label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
}
```

- [ ] **Step 2: Build and commit**

```bash
./build.bat
git add Source/UI/components/MasterVolKnob.h
git commit -m "feat: add MasterVolKnob component (Phase 1, Task 9)"
```

---

## Task 10: Rewrite TopBar composing the new components

**Goal:** Replace the old `TopBar` with a 3-column grid (260 px | 1fr | 260 px) matching the HTML mockup: brand on the left, preset-name cell in the middle (with arrows flanking it), prompt button + master volume on the right.

**Files:**
- Modify: `Source/UI/TopBar.h`

- [ ] **Step 1: Rewrite `Source/UI/TopBar.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DesignTokens.h"
#include "../Presets/PresetManager.h"
#include "../Parameters/ParameterIDs.h"
#include "components/BrandBadge.h"
#include "components/PresetNameField.h"
#include "components/PromptButton.h"
#include "components/NavArrowButton.h"
#include "components/MasterVolKnob.h"

class TopBar : public juce::Component
{
public:
    std::function<void()> onPromptRequested;   // Phase 4 wires the AI modal here

    TopBar(juce::AudioProcessorValueTreeState& apvts, PresetManager& pm)
        : presetManager(pm),
          prevArrow(SynthVibe::NavArrowButton::Direction::Prev),
          nextArrow(SynthVibe::NavArrowButton::Direction::Next),
          masterKnob(apvts, ParamIDs::masterVolume)
    {
        addAndMakeVisible(brand);
        addAndMakeVisible(prevArrow);
        addAndMakeVisible(nameField);
        addAndMakeVisible(nextArrow);
        addAndMakeVisible(promptBtn);
        addAndMakeVisible(masterKnob);

        prevArrow.onClick = [this] { navigatePreset(-1); };
        nextArrow.onClick = [this] { navigatePreset(+1); };
        promptBtn.onClick = [this] { if (onPromptRequested) onPromptRequested(); };

        refreshPresetLabel();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace SynthVibe::Tokens;
        auto b = getLocalBounds().toFloat();
        g.setColour(panel);
        g.fillRect(b);
        g.setColour(edge);
        g.drawHorizontalLine(getHeight() - 1, 0.f, (float) getWidth());
    }

    void resized() override
    {
        using namespace SynthVibe::Tokens;
        auto area = getLocalBounds().reduced(Tokens::spaceLg, 0);

        // Left column: brand (260 px)
        auto left = area.removeFromLeft(260);
        brand.setBounds(left.withSizeKeepingCentre(left.getWidth(), 28));

        // Right column: prompt + master volume (260 px)
        auto right = area.removeFromRight(260);
        masterKnob.setBounds(right.removeFromRight(80));
        right.removeFromRight(spaceSm);
        promptBtn.setBounds(right.withSizeKeepingCentre(100, 28));

        // Centre column: [prev-arrow | name-field | next-arrow]
        // Vertically centre a 38 px tall row inside the centre region.
        auto centre = area.withSizeKeepingCentre(area.getWidth(), 38);
        const int arrowW = 36;   // pill-ish; tweak in Phase 2 alongside the full top-bar breakpoint sweep
        prevArrow.setBounds(centre.removeFromLeft(arrowW));
        centre.removeFromLeft(spaceSm);
        nextArrow.setBounds(centre.removeFromRight(arrowW));
        centre.removeFromRight(spaceSm);
        nameField.setBounds(centre);
    }

private:
    PresetManager& presetManager;
    int currentPresetIndex = 0;

    SynthVibe::BrandBadge       brand;
    SynthVibe::NavArrowButton   prevArrow;
    SynthVibe::PresetNameField  nameField;
    SynthVibe::NavArrowButton   nextArrow;
    SynthVibe::PromptButton     promptBtn;
    SynthVibe::MasterVolKnob    masterKnob;

    void refreshPresetLabel()
    {
        auto names = presetManager.getPresetNames();
        if (names.isEmpty())
        {
            nameField.setPresetName("Init");
            nameField.setMetaText("—");
            currentPresetIndex = 0;
            prevArrow.setEnabled(false);
            nextArrow.setEnabled(false);
            return;
        }
        currentPresetIndex = juce::jlimit(0, names.size() - 1, currentPresetIndex);
        nameField.setPresetName(names[currentPresetIndex]);
        nameField.setMetaText(juce::String(currentPresetIndex + 1) + " / " + juce::String(names.size()));
        prevArrow.setEnabled(currentPresetIndex > 0);
        nextArrow.setEnabled(currentPresetIndex < names.size() - 1);
    }

    void navigatePreset(int delta)
    {
        auto names = presetManager.getPresetNames();
        if (names.isEmpty()) return;
        currentPresetIndex = (currentPresetIndex + delta + names.size()) % names.size();
        presetManager.loadPreset(names[currentPresetIndex]);
        refreshPresetLabel();
    }
};
```

> **Known Phase 5 gap:** Save/Load buttons from the old TopBar are removed. The preset name field will get an inline-rename gesture + save/load via PromptModal's preset drawer in Phase 5. Until then, presets are read-only from disk.

- [ ] **Step 2: Build and deploy, visual verify**

```bash
./build.bat && ./deploy.bat
```

Open DAW → load AI Synth → top bar shows BrandBadge on left, preset cell centered with working ◀▶, purple PROMPT pill and mini vol knob on right.

- [ ] **Step 3: Commit**

```bash
git add Source/UI/TopBar.h
git commit -m "feat: rewrite TopBar with new design components (Phase 1, Task 10)"
```

---

## Task 11: Default window size + editor background

**Goal:** Set the editor to the design's Default breakpoint (1280 × 720) and paint the background with `Tokens::bg` directly.

**Files:**
- Modify: `Source/PluginEditor.cpp`
- Modify: `Source/PluginEditor.h` (only if the file uses `SynthLookAndFeel::colBackground` instead of tokens — switch the include)

- [ ] **Step 1: Edit `Source/PluginEditor.cpp`**

Change `setSize(1200, 750);` to `setSize(1280, 720);`.

Change `paint()` from:

```cpp
g.fillAll(juce::Colour(SynthLookAndFeel::colBackground));
```

to:

```cpp
g.fillAll(SynthVibe::Tokens::bg);
```

Add the include near the top of the file:

```cpp
#include "UI/DesignTokens.h"
```

- [ ] **Step 2: Build, deploy, visual verify at Default size**

```bash
./build.bat && ./deploy.bat
```

Open DAW → plugin window opens at 1280 × 720. Background is `#0F0D14`.

- [ ] **Step 3: Commit**

```bash
git add Source/PluginEditor.cpp
git commit -m "feat: default window to 1280x720 and Night Plum bg (Phase 1, Task 11)"
```

---

## Task 12: Smoke-test checklist + deploy

**Goal:** End-to-end verification before declaring Phase 1 done.

**No new files.**

- [ ] **Step 1: Run every test**

```bash
./build.bat && ./build/AISynthTests_artefacts/Release/AISynth\ Tests.exe
```

Expected:
- `ParameterIdMigration` — pass
- `DesignTokens` — pass
- `UIConstruction` — pass
- `Voice`, `SynthEngine`, `ArpEngine` — pass (unchanged)

- [ ] **Step 2: Deploy and open in DAW**

```bash
./deploy.bat
```

Verify the following by eye in the DAW:
- Window is 1280 × 720, background `#0F0D14` (near-black plum)
- Top bar: brand + "AI Synth V1" left, "PRESET Init" centred, purple PROMPT pill + VOL knob on right
- ◀▶ arrows are dimmed when the preset list is empty
- ◀▶ arrows render with enough visual weight (36 px wide pills — if they look too thin, bump `arrowW` in `TopBar::resized`)
- Tab row, Sound tab, and Bottom bar render without crashes (still old styling — Phase 2)
- Click PROMPT — no crash (callback logs nothing yet; Phase 4)
- Play a note — sound is correct (engine untouched)

- [ ] **Step 3: Final commit (if any cleanup was needed)**

If visual-check turned up nothing fixable, skip this step. Otherwise fix + commit:

```bash
git add -A
git commit -m "chore: Phase 1 smoke-test cleanup"
```

---

## Self-review notes for the executor

- **Font embedding is a future task.** Every font call goes through `SynthVibe::Fonts::{mono,sans,serif}`; replacing the defaults with BinaryData later is a one-file change to `Fonts.h`.
- **SoundTab / ModTab / FXTab / ArpTab keep working unchanged** because `KnobWithLabel` uses the new `SynthLookAndFeel::drawRotarySlider` transparently. They look "half-styled" (new palette, old layout) — that's intentional; Phase 2 rewrites them.
- **The removed Save/Load buttons** from the old TopBar are the only user-facing regression. If that's unacceptable before Phase 5, add them back as token-styled `juce::TextButton` instances in Task 10 — the new `SynthLookAndFeel::drawButtonBackground` already handles them.
- **Colour-token legacy aliases** in `LookAndFeel.h` (`colBackground`, `colOscAccent`, …) are kept so tabs compile. Remove them in Phase 2's first task.
