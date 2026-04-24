#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "../DesignTokens.h"

namespace SynthVibe
{
    // Styled MidiKeyboardComponent — uses the Night Plum palette for key
    // surfaces and the accent violet for actively-played keys. We subclass
    // juce::MidiKeyboardComponent instead of rebuilding from scratch so we
    // inherit its hit-testing, note-on/off plumbing and scroll buttons.
    class SynthKeyboard : public juce::MidiKeyboardComponent
    {
    public:
        // Visible octaves — key width is computed in resized() so these fill
        // the component exactly regardless of host/editor width.
        static constexpr int kOctavesVisible = 4;

        SynthKeyboard(juce::MidiKeyboardState& state)
            : juce::MidiKeyboardComponent(state, juce::MidiKeyboardComponent::horizontalKeyboard)
        {
            setAvailableRange(36, 96);  // C2 – C7; what the user can scroll to
            setLowestVisibleKey(48);    // C3 — sits one octave below C4/middle-C
            setScrollButtonsVisible(false);
            setColour(juce::MidiKeyboardComponent::whiteNoteColourId,    Tokens::panel2);
            setColour(juce::MidiKeyboardComponent::blackNoteColourId,    juce::Colour(0xFF0A0B10));
            setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, Tokens::edge);
            setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,   Tokens::accent);
            setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
                      Tokens::accent.withAlpha(0.25f));
            setColour(juce::MidiKeyboardComponent::textLabelColourId,    Tokens::ink4);
        }

        void resized() override
        {
            // Exactly kOctavesVisible octaves (7 * N white keys) fit across
            // the component, so keys grow with the plugin window instead of
            // being cropped or leaving a dead strip on the right.
            const int whiteKeys = 7 * kOctavesVisible;
            setKeyWidth(juce::jmax(1.f, getWidth() / (float) whiteKeys));
            juce::MidiKeyboardComponent::resized();
        }

    protected:
        void drawWhiteNote(int midiNoteNumber, juce::Graphics& g,
                           juce::Rectangle<float> area,
                           bool isDown, bool isOver,
                           juce::Colour /*lineColour*/,
                           juce::Colour /*textColour*/) override
        {
            using namespace Tokens;

            // Base gradient — keys darken towards the bottom like the mock's
            // brushed-metal look.
            juce::ColourGradient base(panel2, area.getX(), area.getY(),
                                      juce::Colour(0xFF0C0D12),
                                      area.getX(), area.getBottom(),
                                      false);
            base.addColour(0.6, panel);
            g.setGradientFill(base);
            g.fillRect(area);

            if (isDown)
            {
                // Playing key: accent wash + soft glow at the bottom so the
                // highlight reads from a distance without blowing out detail.
                juce::ColourGradient play(accent,
                                          area.getX(), area.getY(),
                                          accent.interpolatedWith(panel, 0.4f),
                                          area.getX(), area.getBottom(),
                                          false);
                g.setGradientFill(play);
                g.fillRect(area);
            }
            else if (isOver)
            {
                g.setColour(accent.withAlpha(0.10f));
                g.fillRect(area);
            }

            // Key separator — thin edge on the right.
            g.setColour(edge);
            g.drawVerticalLine((int) area.getRight() - 1,
                               area.getY(), area.getBottom());

            // Label C-notes only, like the mock, to avoid clutter.
            if ((midiNoteNumber % 12) == 0)
            {
                g.setColour(isDown ? juce::Colour(0xFF1A1A1A) : ink4);
                g.setFont(7.f);
                g.drawText("C" + juce::String(midiNoteNumber / 12 - 1),
                           area.reduced(0, 4),
                           juce::Justification::centredBottom);
            }
        }

        void drawBlackNote(int /*midiNoteNumber*/, juce::Graphics& g,
                           juce::Rectangle<float> area,
                           bool isDown, bool isOver,
                           juce::Colour /*noteFillColour*/) override
        {
            using namespace Tokens;

            if (isDown)
            {
                juce::ColourGradient play(accent,
                                          area.getX(), area.getY(),
                                          accent.interpolatedWith(juce::Colours::black, 0.6f),
                                          area.getX(), area.getBottom(),
                                          false);
                g.setGradientFill(play);
                g.fillRect(area);
            }
            else
            {
                juce::ColourGradient base(juce::Colour(0xFF0A0B10),
                                          area.getX(), area.getY(),
                                          juce::Colour(0xFF05060A),
                                          area.getX(), area.getBottom(),
                                          false);
                g.setGradientFill(base);
                g.fillRect(area);

                if (isOver)
                {
                    g.setColour(accent.withAlpha(0.15f));
                    g.fillRect(area);
                }
            }

            // Thin top highlight so the key reads separately from the
            // white keys behind it.
            g.setColour(edge2.withAlpha(0.6f));
            g.drawHorizontalLine((int) area.getY(),
                                 area.getX(), area.getRight());
        }
    };
}
