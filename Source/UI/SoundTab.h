#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "KnobWithLabel.h"
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
        drawPanel(g, osc1Bounds,   "OSC 1",   SynthLookAndFeel::colOscAccent);
        drawPanel(g, osc2Bounds,   "OSC 2",   SynthLookAndFeel::colOscAccent);
        drawPanel(g, filterBounds, "FILTER",  SynthLookAndFeel::colFilterAccent);
        drawPanel(g, ampEnvBounds, "AMP ENV", SynthLookAndFeel::colEnvAccent);
        drawPanel(g, fltEnvBounds, "FLT ENV", SynthLookAndFeel::colEnvAccent);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(6);
        const int pad    = 6;
        const int comboH = 26;
        const int titleH = 20;

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

        // Col 3 stacked
        const int filterH = col3.getHeight() * 35 / 100;
        const int envH    = (col3.getHeight() - filterH) / 2;

        filterBounds = col3.removeFromTop(filterH);
        auto fc = filterBounds.withTrimmedTop(titleH);
        filterTypeBox.setBounds(fc.removeFromTop(comboH));
        layoutKnobs(fc, { &knobCutoff, &knobResonance, &knobFilterEnv });

        ampEnvBounds = col3.removeFromTop(envH);
        layoutKnobs(ampEnvBounds.withTrimmedTop(titleH),
                    { &knobAmpA, &knobAmpD, &knobAmpS, &knobAmpR });

        fltEnvBounds = col3;
        layoutKnobs(fltEnvBounds.withTrimmedTop(titleH),
                    { &knobFltA, &knobFltD, &knobFltS, &knobFltR });
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
        g.setColour(juce::Colour(SynthLookAndFeel::colHighlight));
        g.setFont(juce::Font(11.f, juce::Font::bold));
        g.drawText(title, bounds.getX() + 10, bounds.getY() + 4,
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
