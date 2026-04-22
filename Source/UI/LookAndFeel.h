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

        // addCentredArc: 0 = 12 o'clock, CW. std::cos/sin on screen coords (y-down)
        // traces CW from 3 o'clock, so the tick angle is the arc angle shifted by -pi/2.
        const float arcStart  = juce::degreesToRadians(knobArcStartDeg);
        const float arcSweep  = juce::degreesToRadians(knobArcSweepDeg);
        const float arcValue  = arcStart + sliderPos * arcSweep;
        const float tickAngle = arcValue - juce::MathConstants<float>::halfPi;

        // Track arc
        juce::Path track;
        track.addCentredArc(cx, cy, rOuter, rOuter, 0.f, arcStart, arcStart + arcSweep, true);
        g.setColour(edge);
        g.strokePath(track, juce::PathStrokeType(knobArcThickness));

        // Value arc
        juce::Path value;
        value.addCentredArc(cx, cy, rOuter, rOuter, 0.f, arcStart, arcValue, true);
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
        tick.startNewSubPath(cx + tickInner * std::cos(tickAngle),
                             cy + tickInner * std::sin(tickAngle));
        tick.lineTo         (cx + tickOuter * std::cos(tickAngle),
                             cy + tickOuter * std::sin(tickAngle));
        g.setColour(arcColour);
        g.strokePath(tick, juce::PathStrokeType(1.5f));
    }
};
