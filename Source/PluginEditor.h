#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/LookAndFeel.h"

// -----------------------------------------------------------------------
// Labelled rotary knob with live value display.
// Double-click or Ctrl+click to type an exact value.
// -----------------------------------------------------------------------
struct KnobWithLabel : public juce::Component
{
    juce::Slider slider { juce::Slider::RotaryVerticalDrag,
                          juce::Slider::TextBoxBelow };
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
        nameLabel.setFont(juce::Font(10.f, juce::Font::bold));
        nameLabel.setColour(juce::Label::textColourId,
                            juce::Colour(SynthLookAndFeel::colTextDim));
        addAndMakeVisible(nameLabel);

        slider.setTextValueSuffix(valueSuffix);
        slider.setNumDecimalPlacesToDisplay(decimalPlaces);
        slider.setTooltip("Double-click or Ctrl+click to enter a value");
        addAndMakeVisible(slider);

        attachment = std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramID, slider);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        nameLabel.setBounds(b.removeFromTop(14));
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, b.getWidth(), 16);
        slider.setBounds(b);
    }
};

// -----------------------------------------------------------------------
// Main editor
// -----------------------------------------------------------------------
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

    // Oscillator
    juce::ComboBox oscWaveformBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> oscWaveformAttach;
    KnobWithLabel knobDetune  { "Detune",  processor.apvts, "osc_detune",  " ct",  1 };
    KnobWithLabel knobOctave  { "Octave",  processor.apvts, "osc_octave",  " oct", 0 };

    // Filter
    juce::ComboBox filterTypeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterTypeAttach;
    KnobWithLabel knobCutoff    { "Cutoff",  processor.apvts, "filter_cutoff",    " Hz", 0 };
    KnobWithLabel knobResonance { "Res",     processor.apvts, "filter_resonance", "",    2 };
    KnobWithLabel knobFilterEnv { "Env Amt", processor.apvts, "filter_env_amt",   "",    2 };

    // Amp Envelope
    KnobWithLabel knobAmpA { "A", processor.apvts, "amp_attack",  " s", 3 };
    KnobWithLabel knobAmpD { "D", processor.apvts, "amp_decay",   " s", 3 };
    KnobWithLabel knobAmpS { "S", processor.apvts, "amp_sustain", "",   2 };
    KnobWithLabel knobAmpR { "R", processor.apvts, "amp_release", " s", 3 };

    // Filter Envelope
    KnobWithLabel knobFltA { "A", processor.apvts, "flt_attack",  " s", 3 };
    KnobWithLabel knobFltD { "D", processor.apvts, "flt_decay",   " s", 3 };
    KnobWithLabel knobFltS { "S", processor.apvts, "flt_sustain", "",   2 };
    KnobWithLabel knobFltR { "R", processor.apvts, "flt_release", " s", 3 };

    // Delay
    KnobWithLabel knobDelayTime     { "Time",     processor.apvts, "delay_time",     " ms", 0 };
    KnobWithLabel knobDelayFeedback { "Feedback", processor.apvts, "delay_feedback", "",    2 };
    KnobWithLabel knobDelayMix      { "Mix",      processor.apvts, "delay_mix",      "",    2 };

    // Master
    KnobWithLabel knobMaster { "Volume", processor.apvts, "master_volume", "", 2 };

    // Section title labels
    juce::Label lblOsc, lblFilter, lblAmpEnv, lblFltEnv, lblDelay, lblMaster;

    void layoutKnobs(juce::Rectangle<int> bounds,
                     std::initializer_list<juce::Component*> knobs);
    void styleSection(juce::Label& l, const juce::String& text);
    void paintSectionBg(juce::Graphics& g, juce::Rectangle<int> bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AISynthEditor)
};
