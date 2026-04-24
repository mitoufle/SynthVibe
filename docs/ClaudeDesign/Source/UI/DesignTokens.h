#pragma once
#include <juce_graphics/juce_graphics.h>

/*
    SynthVibe — Design Tokens
    Generated from the HTML mockup (see docs/AISynth V1 Hi-Fi.html).
    Do not edit colors here without updating the mockup first.

    Palette = "plum" (the chosen default).
*/

namespace SynthVibe::Tokens
{
    // -------- Color: surfaces --------
    inline const juce::Colour bg       { 0xFF0A0B10 }; // deepest background
    inline const juce::Colour panel    { 0xFF15171E }; // top bar, tab bar
    inline const juce::Colour panel2   { 0xFF1C1F28 }; // panel bodies
    inline const juce::Colour edge     { 0xFF2A2D38 }; // subtle border
    inline const juce::Colour edge2    { 0xFF3A3E4B }; // hover border

    // -------- Color: ink (text) --------
    inline const juce::Colour ink      { 0xFFEFEADE }; // primary text
    inline const juce::Colour ink2     { 0xFFBCB6A6 }; // secondary
    inline const juce::Colour ink3     { 0xFF7D7868 }; // tertiary / labels
    inline const juce::Colour ink4     { 0xFF545061 }; // faint / placeholders

    // -------- Color: accent (palette = plum) --------
    inline const juce::Colour accent   { 0xFFE8C06A }; // gold — primary accent
    inline const juce::Colour accentHi { 0xFFF2CE7A }; // hover

    // -------- Color: section hues --------
    inline const juce::Colour osc      { 0xFFE8C06A }; // gold
    inline const juce::Colour filter   { 0xFFE88BB0 }; // pink/plum
    inline const juce::Colour env      { 0xFF88B892 }; // sage
    inline const juce::Colour lfo      { 0xFFB48EC8 }; // violet
    inline const juce::Colour fx       { 0xFF6FB2B2 }; // teal
    inline const juce::Colour arp      { 0xFFD9A85F }; // burnt gold
    inline const juce::Colour vel      { 0xFFE8C06A };

    // Helpers for signed mod bars
    inline const juce::Colour modPositive { 0xFFB48EC8 };
    inline const juce::Colour modNegative { 0xFFE88BB0 };

    // -------- Spacing --------
    constexpr int spaceXs = 4;
    constexpr int spaceSm = 8;
    constexpr int spaceMd = 12;
    constexpr int spaceLg = 18;
    constexpr int spaceXl = 24;

    // -------- Radii --------
    constexpr float radiusSm = 4.0f;
    constexpr float radiusMd = 6.0f;
    constexpr float radiusLg = 10.0f;

    // -------- Knob geometry --------
    // Arc sweeps from 225° (7 o'clock, value=0) clockwise 270° to 135° (5 o'clock, value=1).
    constexpr float knobArcStartDeg = 225.0f;
    constexpr float knobArcSweepDeg = 270.0f;
    constexpr float knobArcThickness = 2.5f;           // visible arc stroke
    constexpr float knobInnerBodyInset = 5.0f;         // pixels inside the outer arc
    constexpr int   knobSizeCompact = 38;
    constexpr int   knobSizeDefault = 46;
    constexpr int   knobSizeWide    = 52;

    // -------- Typography sizes (pt-ish floats) --------
    namespace Font
    {
        // Display / titles
        constexpr float presetTitle = 17.0f;
        constexpr float panelTitle  = 10.0f; // uppercase mono
        constexpr float fxName      = 16.0f; // serif italic

        // Body
        constexpr float body        = 12.0f;
        constexpr float label       = 9.0f;  // uppercase mono, letter-spaced
        constexpr float value       = 9.0f;  // mono
        constexpr float tiny        = 8.0f;  // voice-LED meta
    }

    // -------- Font family roles --------
    namespace Family
    {
        inline const juce::String mono   = "JetBrains Mono";
        inline const juce::String sans   = "Inter";
        inline const juce::String serif  = "Instrument Serif";
    }

    // -------- Motion --------
    namespace Motion
    {
        constexpr int knobDragMs     = 80;
        constexpr int knobTweenMs    = 400;   // preset apply
        constexpr int tickFlashMs    = 140;
        constexpr int resetFlashMs   = 500;
        constexpr int applyFlashMs   = 600;
        constexpr int rippleMs       = 900;
        constexpr int modalOpenMs    = 280;
        constexpr int drawerSlideMs  = 280;
        constexpr int flipSortMs     = 380;
        constexpr int ambientScopeMs = 3200;  // scope glow period
        constexpr int lfoDotMs       = 3200;  // one LFO1 cycle
    }

    // -------- Keyboard --------
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
