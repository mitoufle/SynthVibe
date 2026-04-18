#include "PluginEditor.h"

AISynthEditor::AISynthEditor(AISynthProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&laf);
    setSize(900, 560);

    osc1WaveBox.addItemList({ "Sine", "Saw", "Square", "Triangle" }, 1);
    osc1WaveAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "osc1_waveform", osc1WaveBox);

    osc2WaveBox.addItemList({ "Sine", "Saw", "Square", "Triangle" }, 1);
    osc2WaveAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "osc2_waveform", osc2WaveBox);

    filterTypeBox.addItemList({ "Low Pass", "High Pass", "Band Pass" }, 1);
    filterTypeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "filter_type", filterTypeBox);

    lfo1ShapeBox.addItemList({ "Sine", "Saw", "Square", "Triangle" }, 1);
    lfo1ShapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "lfo1_shape", lfo1ShapeBox);
    lfo1DestBox.addItemList({ "Pitch", "Filter", "Amp", "Detune" }, 1);
    lfo1DestAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "lfo1_dest", lfo1DestBox);

    lfo2ShapeBox.addItemList({ "Sine", "Saw", "Square", "Triangle" }, 1);
    lfo2ShapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "lfo2_shape", lfo2ShapeBox);
    lfo2DestBox.addItemList({ "Pitch", "Filter", "Amp", "Detune" }, 1);
    lfo2DestAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "lfo2_dest", lfo2DestBox);

    arpOnAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.apvts, "arp_enabled", arpOnButton);
    arpModeBox.addItemList({ "Up", "Down", "UpDown", "Random" }, 1);
    arpModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "arp_mode", arpModeBox);
    arpRateBox.addItemList({ "1/16", "1/8", "1/4", "1/2" }, 1);
    arpRateAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "arp_rate", arpRateBox);

    styleLabel(lblOsc1,   "OSC 1",   SynthLookAndFeel::colOscAccent);
    styleLabel(lblOsc2,   "OSC 2",   SynthLookAndFeel::colOscAccent);
    styleLabel(lblFilter, "FILTER",  SynthLookAndFeel::colFilterAccent);
    styleLabel(lblAmpEnv, "AMP ENV", SynthLookAndFeel::colEnvAccent);
    styleLabel(lblFltEnv, "FLT ENV", SynthLookAndFeel::colEnvAccent);
    styleLabel(lblLfo1,   "LFO 1",   SynthLookAndFeel::colLfoAccent);
    styleLabel(lblLfo2,   "LFO 2",   SynthLookAndFeel::colLfoAccent);
    styleLabel(lblDelay,  "DELAY",   SynthLookAndFeel::colFxAccent);
    styleLabel(lblChorus, "CHORUS",  SynthLookAndFeel::colFxAccent);
    styleLabel(lblArp,    "ARP",     SynthLookAndFeel::colArpAccent);
    styleLabel(lblMaster, "MASTER",  SynthLookAndFeel::colAccent);

    for (juce::ComboBox* c : std::initializer_list<juce::ComboBox*>{
                     &osc1WaveBox, &osc2WaveBox, &filterTypeBox,
                     &lfo1ShapeBox, &lfo1DestBox, &lfo2ShapeBox, &lfo2DestBox,
                     &arpModeBox, &arpRateBox })
        addAndMakeVisible(c);

    addAndMakeVisible(arpOnButton);

    for (juce::Component* c : std::initializer_list<juce::Component*>{
                     &knobOsc1Oct, &knobOsc1Semi, &knobOsc1Detune, &knobOsc1Level,
                     &knobOsc2Oct, &knobOsc2Semi, &knobOsc2Detune, &knobOsc2Level,
                     &knobCutoff, &knobResonance, &knobFilterEnv,
                     &knobAmpA, &knobAmpD, &knobAmpS, &knobAmpR,
                     &knobFltA, &knobFltD, &knobFltS, &knobFltR,
                     &knobLfo1Rate, &knobLfo1Depth, &knobLfo2Rate, &knobLfo2Depth,
                     &knobDelayTime, &knobDelayFeedback, &knobDelayMix,
                     &knobChorusRate, &knobChorusDepth, &knobChorusMix,
                     &knobArpOct, &knobMaster })
        addAndMakeVisible(c);
}

AISynthEditor::~AISynthEditor()
{
    setLookAndFeel(nullptr);
}

void AISynthEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(SynthLookAndFeel::colBackground));
}

void AISynthEditor::resized()
{
    auto area  = getLocalBounds().reduced(8);
    const int pad    = 6;
    const int comboH = 22;
    const int labelH = 16;

    const int row1H = 110;
    const int row2H = 120;
    const int row3H = 100;
    const int row4H = 90;

    // ── ROW 1: OSC 1 | OSC 2 ──────────────────────────────────────────────
    auto row1     = area.removeFromTop(row1H).reduced(0, pad / 2);
    auto osc1Area = row1.removeFromLeft(row1.getWidth() / 2).reduced(pad, 0);
    auto osc2Area = row1.reduced(pad, 0);

    lblOsc1.setBounds(osc1Area.removeFromTop(labelH));
    osc1WaveBox.setBounds(osc1Area.removeFromLeft(90).removeFromTop(comboH));
    layoutKnobs(osc1Area, { &knobOsc1Oct, &knobOsc1Semi, &knobOsc1Detune, &knobOsc1Level });

    lblOsc2.setBounds(osc2Area.removeFromTop(labelH));
    osc2WaveBox.setBounds(osc2Area.removeFromLeft(90).removeFromTop(comboH));
    layoutKnobs(osc2Area, { &knobOsc2Oct, &knobOsc2Semi, &knobOsc2Detune, &knobOsc2Level });

    // ── ROW 2: Filter | AmpEnv | FltEnv ───────────────────────────────────
    auto row2       = area.removeFromTop(row2H).reduced(0, pad / 2);
    auto filterArea = row2.removeFromLeft(260).reduced(pad, 0);
    auto ampEnvArea = row2.removeFromLeft(row2.getWidth() / 2).reduced(pad, 0);
    auto fltEnvArea = row2.reduced(pad, 0);

    lblFilter.setBounds(filterArea.removeFromTop(labelH));
    filterTypeBox.setBounds(filterArea.removeFromTop(comboH));
    layoutKnobs(filterArea, { &knobCutoff, &knobResonance, &knobFilterEnv });

    lblAmpEnv.setBounds(ampEnvArea.removeFromTop(labelH));
    layoutKnobs(ampEnvArea, { &knobAmpA, &knobAmpD, &knobAmpS, &knobAmpR });

    lblFltEnv.setBounds(fltEnvArea.removeFromTop(labelH));
    layoutKnobs(fltEnvArea, { &knobFltA, &knobFltD, &knobFltS, &knobFltR });

    // ── ROW 3: LFO 1 | LFO 2 ──────────────────────────────────────────────
    auto row3     = area.removeFromTop(row3H).reduced(0, pad / 2);
    auto lfo1Area = row3.removeFromLeft(row3.getWidth() / 2).reduced(pad, 0);
    auto lfo2Area = row3.reduced(pad, 0);

    lblLfo1.setBounds(lfo1Area.removeFromTop(labelH));
    lfo1ShapeBox.setBounds(lfo1Area.removeFromLeft(80).withHeight(comboH));
    lfo1DestBox.setBounds (lfo1Area.removeFromLeft(80).withHeight(comboH));
    layoutKnobs(lfo1Area, { &knobLfo1Rate, &knobLfo1Depth });

    lblLfo2.setBounds(lfo2Area.removeFromTop(labelH));
    lfo2ShapeBox.setBounds(lfo2Area.removeFromLeft(80).withHeight(comboH));
    lfo2DestBox.setBounds (lfo2Area.removeFromLeft(80).withHeight(comboH));
    layoutKnobs(lfo2Area, { &knobLfo2Rate, &knobLfo2Depth });

    // ── ROW 4: Delay | Chorus | Arp | Master ──────────────────────────────
    auto row4       = area.removeFromTop(row4H).reduced(0, pad / 2);
    auto delayArea  = row4.removeFromLeft(180).reduced(pad, 0);
    auto chorusArea = row4.removeFromLeft(180).reduced(pad, 0);
    auto arpArea    = row4.removeFromLeft(200).reduced(pad, 0);
    auto masterArea = row4.reduced(pad, 0);

    lblDelay.setBounds(delayArea.removeFromTop(labelH));
    layoutKnobs(delayArea, { &knobDelayTime, &knobDelayFeedback, &knobDelayMix });

    lblChorus.setBounds(chorusArea.removeFromTop(labelH));
    layoutKnobs(chorusArea, { &knobChorusRate, &knobChorusDepth, &knobChorusMix });

    lblArp.setBounds(arpArea.removeFromTop(labelH));
    arpOnButton.setBounds(arpArea.removeFromLeft(30).removeFromTop(22));
    arpModeBox.setBounds(arpArea.removeFromTop(comboH).removeFromLeft(90));
    arpRateBox.setBounds(arpArea.removeFromTop(comboH).removeFromLeft(70));
    knobArpOct.setBounds(arpArea);

    lblMaster.setBounds(masterArea.removeFromTop(labelH));
    knobMaster.setBounds(masterArea);
}

void AISynthEditor::layoutKnobs(juce::Rectangle<int> bounds,
                                 std::initializer_list<juce::Component*> knobs)
{
    if (knobs.size() == 0) return;
    const int w = bounds.getWidth() / static_cast<int>(knobs.size());
    for (auto* k : knobs)
        k->setBounds(bounds.removeFromLeft(w));
}

void AISynthEditor::styleLabel(juce::Label& l, const juce::String& text, juce::uint32 colour)
{
    l.setText(text, juce::dontSendNotification);
    l.setFont(juce::Font(10.f, juce::Font::bold));
    l.setColour(juce::Label::textColourId, juce::Colour(colour));
    addAndMakeVisible(l);
}
