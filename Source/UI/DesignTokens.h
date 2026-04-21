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
