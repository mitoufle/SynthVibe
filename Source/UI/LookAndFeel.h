#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Central place to define the synth's visual style.
// Override more methods here as the UI grows.
class SynthLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Colour palette
    static constexpr auto colBackground  = 0xFF1A1A2E;
    static constexpr auto colPanel       = 0xFF16213E;
    static constexpr auto colAccent      = 0xFF0F3460;
    static constexpr auto colHighlight   = 0xFFE94560;
    static constexpr auto colKnobTrack   = 0xFF2D4A6B;
    static constexpr auto colText        = 0xFFCCCCDD;
    static constexpr auto colTextDim     = 0xFF667788;

    // Section accent colors
    static constexpr auto colOscAccent    = 0xFF2255AA;
    static constexpr auto colFilterAccent = 0xFF7722AA;
    static constexpr auto colEnvAccent    = 0xFF226622;
    static constexpr auto colLfoAccent    = 0xFFAA2266;
    static constexpr auto colFxAccent     = 0xFF227799;
    static constexpr auto colArpAccent    = 0xFFAA7722;

    SynthLookAndFeel()
    {
        setColour(juce::Slider::rotarySliderFillColourId,   juce::Colour(colHighlight));
        setColour(juce::Slider::rotarySliderOutlineColourId,juce::Colour(colKnobTrack));
        setColour(juce::Slider::thumbColourId,              juce::Colour(colHighlight));
        setColour(juce::Label::textColourId,                juce::Colour(colText));
        setColour(juce::ComboBox::backgroundColourId,       juce::Colour(colAccent));
        setColour(juce::ComboBox::textColourId,             juce::Colour(colText));
        setColour(juce::ComboBox::outlineColourId,          juce::Colour(colKnobTrack));
        setColour(juce::PopupMenu::backgroundColourId,      juce::Colour(colPanel));
        setColour(juce::PopupMenu::textColourId,            juce::Colour(colText));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(colHighlight));
    }

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

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider& slider) override
    {
        const float radius    = juce::jmin(width, height) * 0.45f;
        const float centreX   = x + width  * 0.5f;
        const float centreY   = y + height * 0.5f;
        const float angle     = startAngle + sliderPos * (endAngle - startAngle);

        // Track arc
        juce::Path track;
        track.addCentredArc(centreX, centreY, radius, radius,
                            0.f, startAngle, endAngle, true);
        g.setColour(juce::Colour(colKnobTrack));
        g.strokePath(track, juce::PathStrokeType(3.f));

        // Fill arc
        juce::Path fill;
        fill.addCentredArc(centreX, centreY, radius, radius,
                           0.f, startAngle, angle, true);
        g.setColour(juce::Colour(colHighlight));
        g.strokePath(fill, juce::PathStrokeType(3.f));

        // Thumb dot
        const float thumbX = centreX + radius * std::cos(angle - juce::MathConstants<float>::halfPi);
        const float thumbY = centreY + radius * std::sin(angle - juce::MathConstants<float>::halfPi);
        g.setColour(juce::Colours::white);
        g.fillEllipse(thumbX - 4.f, thumbY - 4.f, 8.f, 8.f);
    }
};
