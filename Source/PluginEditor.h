#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/LookAndFeel.h"

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
        nameLabel.setFont(juce::Font(10.f, juce::Font::bold));
        nameLabel.setColour(juce::Label::textColourId, juce::Colour(SynthLookAndFeel::colTextDim));
        addAndMakeVisible(nameLabel);
        slider.setTextValueSuffix(valueSuffix);
        slider.setNumDecimalPlacesToDisplay(decimalPlaces);
        slider.setTooltip("Double-click or Ctrl+click to enter a value");
        addAndMakeVisible(slider);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramID, slider);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        nameLabel.setBounds(b.removeFromTop(14));
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, b.getWidth(), 16);
        slider.setBounds(b);
    }
};

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

    // --- OSC 1 ---
    juce::ComboBox osc1WaveBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osc1WaveAttach;
    KnobWithLabel knobOsc1Oct    { "Oct",    processor.apvts, "osc1_octave",   "",    0 };
    KnobWithLabel knobOsc1Semi   { "Semi",   processor.apvts, "osc1_semitone", "",    0 };
    KnobWithLabel knobOsc1Detune { "Detune", processor.apvts, "osc1_detune",   " ct", 1 };
    KnobWithLabel knobOsc1Level  { "Level",  processor.apvts, "osc1_level",    "",    2 };

    // --- OSC 2 ---
    juce::ComboBox osc2WaveBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osc2WaveAttach;
    KnobWithLabel knobOsc2Oct    { "Oct",    processor.apvts, "osc2_octave",   "",    0 };
    KnobWithLabel knobOsc2Semi   { "Semi",   processor.apvts, "osc2_semitone", "",    0 };
    KnobWithLabel knobOsc2Detune { "Detune", processor.apvts, "osc2_detune",   " ct", 1 };
    KnobWithLabel knobOsc2Level  { "Level",  processor.apvts, "osc2_level",    "",    2 };

    // --- FILTER ---
    juce::ComboBox filterTypeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterTypeAttach;
    KnobWithLabel knobCutoff    { "Cutoff",  processor.apvts, "filter_cutoff",    " Hz", 0 };
    KnobWithLabel knobResonance { "Res",     processor.apvts, "filter_resonance", "",    2 };
    KnobWithLabel knobFilterEnv { "Env Amt", processor.apvts, "filter_env_amt",   "",    2 };

    // --- AMP ENV ---
    KnobWithLabel knobAmpA { "A", processor.apvts, "amp_attack",  " s", 3 };
    KnobWithLabel knobAmpD { "D", processor.apvts, "amp_decay",   " s", 3 };
    KnobWithLabel knobAmpS { "S", processor.apvts, "amp_sustain", "",   2 };
    KnobWithLabel knobAmpR { "R", processor.apvts, "amp_release", " s", 3 };

    // --- FILTER ENV ---
    KnobWithLabel knobFltA { "A", processor.apvts, "flt_attack",  " s", 3 };
    KnobWithLabel knobFltD { "D", processor.apvts, "flt_decay",   " s", 3 };
    KnobWithLabel knobFltS { "S", processor.apvts, "flt_sustain", "",   2 };
    KnobWithLabel knobFltR { "R", processor.apvts, "flt_release", " s", 3 };

    // --- LFO 1 ---
    juce::ComboBox lfo1ShapeBox, lfo1DestBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfo1ShapeAttach, lfo1DestAttach;
    KnobWithLabel knobLfo1Rate  { "Rate",  processor.apvts, "lfo1_rate",  " Hz", 2 };
    KnobWithLabel knobLfo1Depth { "Depth", processor.apvts, "lfo1_depth", "",    2 };

    // --- LFO 2 ---
    juce::ComboBox lfo2ShapeBox, lfo2DestBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfo2ShapeAttach, lfo2DestAttach;
    KnobWithLabel knobLfo2Rate  { "Rate",  processor.apvts, "lfo2_rate",  " Hz", 2 };
    KnobWithLabel knobLfo2Depth { "Depth", processor.apvts, "lfo2_depth", "",    2 };

    // --- DELAY ---
    KnobWithLabel knobDelayTime     { "Time",     processor.apvts, "delay_time",     " ms", 0 };
    KnobWithLabel knobDelayFeedback { "Feedback", processor.apvts, "delay_feedback", "",    2 };
    KnobWithLabel knobDelayMix      { "Mix",      processor.apvts, "delay_mix",      "",    2 };

    // --- CHORUS ---
    KnobWithLabel knobChorusRate  { "Rate",  processor.apvts, "chorus_rate",  " Hz", 2 };
    KnobWithLabel knobChorusDepth { "Depth", processor.apvts, "chorus_depth", "",    3 };
    KnobWithLabel knobChorusMix   { "Mix",   processor.apvts, "chorus_mix",   "",    2 };

    // --- ARP ---
    juce::ToggleButton arpOnButton { "On" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> arpOnAttach;
    juce::ComboBox arpModeBox, arpRateBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> arpModeAttach, arpRateAttach;
    KnobWithLabel knobArpOct { "Octaves", processor.apvts, "arp_octave_range", "", 0 };

    // --- MASTER ---
    KnobWithLabel knobMaster { "Volume", processor.apvts, "master_volume", "", 2 };

    // Section labels
    juce::Label lblOsc1, lblOsc2, lblFilter, lblAmpEnv, lblFltEnv;
    juce::Label lblLfo1, lblLfo2, lblDelay, lblChorus, lblArp, lblMaster;

    void layoutKnobs(juce::Rectangle<int> bounds, std::initializer_list<juce::Component*> knobs);
    void styleLabel(juce::Label& l, const juce::String& text, juce::uint32 colour);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AISynthEditor)
};
