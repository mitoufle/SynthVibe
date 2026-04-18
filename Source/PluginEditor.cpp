#include "PluginEditor.h"

static constexpr int EditorW   = 660;
static constexpr int EditorH   = 500;
static constexpr int TopBar    = 32;
static constexpr int Pad       = 8;
static constexpr int KnobH     = 80;   // name label (14) + rotary + value box (16)
static constexpr int SectionTitleH = 16;
static constexpr int ComboH    = 24;

AISynthEditor::AISynthEditor(AISynthProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&laf);
    setSize(EditorW, EditorH);

    // Oscillator waveform combo
    oscWaveformBox.addItemList({ "Sine", "Saw", "Square", "Triangle" }, 1);
    oscWaveformAttach = std::make_unique<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            p.apvts, "osc_waveform", oscWaveformBox);
    addAndMakeVisible(oscWaveformBox);

    // Filter type combo
    filterTypeBox.addItemList({ "Low Pass", "High Pass", "Band Pass" }, 1);
    filterTypeAttach = std::make_unique<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            p.apvts, "filter_type", filterTypeBox);
    addAndMakeVisible(filterTypeBox);

    for (auto* c : { &knobDetune, &knobOctave,
                     &knobCutoff, &knobResonance, &knobFilterEnv,
                     &knobAmpA, &knobAmpD, &knobAmpS, &knobAmpR,
                     &knobFltA, &knobFltD, &knobFltS, &knobFltR,
                     &knobDelayTime, &knobDelayFeedback, &knobDelayMix,
                     &knobMaster })
        addAndMakeVisible(c);

    styleSection(lblOsc,    "Oscillator");
    styleSection(lblFilter, "Filter");
    styleSection(lblAmpEnv, "Amp Envelope");
    styleSection(lblFltEnv, "Filter Envelope");
    styleSection(lblDelay,  "Delay");
    styleSection(lblMaster, "Master");
}

AISynthEditor::~AISynthEditor()
{
    setLookAndFeel(nullptr);
}

void AISynthEditor::styleSection(juce::Label& l, const juce::String& text)
{
    l.setText(text.toUpperCase(), juce::dontSendNotification);
    l.setFont(juce::Font(9.5f, juce::Font::bold));
    l.setColour(juce::Label::textColourId, juce::Colour(SynthLookAndFeel::colHighlight));
    l.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(l);
}

void AISynthEditor::paintSectionBg(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    auto b = bounds.toFloat().reduced(1.f);
    g.setColour(juce::Colour(SynthLookAndFeel::colPanel));
    g.fillRoundedRectangle(b, 5.f);
    g.setColour(juce::Colour(SynthLookAndFeel::colAccent));
    g.drawRoundedRectangle(b, 5.f, 1.f);
}

void AISynthEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(SynthLookAndFeel::colBackground));

    // Title bar
    g.setColour(juce::Colour(SynthLookAndFeel::colAccent));
    g.fillRect(0, 0, EditorW, TopBar);
    g.setColour(juce::Colour(SynthLookAndFeel::colHighlight));
    g.setFont(juce::Font(15.f, juce::Font::bold));
    g.drawText("AI SYNTH", 14, 0, 200, TopBar, juce::Justification::centredLeft);
    g.setColour(juce::Colour(SynthLookAndFeel::colTextDim));
    g.setFont(juce::Font(9.f));
    g.drawText("double-click knobs to enter values",
               EditorW - 220, 0, 210, TopBar, juce::Justification::centredRight);

    // Section backgrounds
    auto area = getLocalBounds().withTrimmedTop(TopBar).reduced(Pad);
    const int row1H = SectionTitleH + ComboH + Pad + KnobH + Pad;
    const int row2H = SectionTitleH + KnobH + Pad;
    const int row3H = SectionTitleH + KnobH + Pad;

    auto row1 = area.removeFromTop(row1H);
    paintSectionBg(g, row1.removeFromLeft(170));
    row1.removeFromLeft(Pad);
    paintSectionBg(g, row1.removeFromLeft(235));
    row1.removeFromLeft(Pad);
    paintSectionBg(g, row1); // master

    area.removeFromTop(Pad);
    auto row2 = area.removeFromTop(row2H);
    paintSectionBg(g, row2.removeFromLeft(225));
    row2.removeFromLeft(Pad);
    paintSectionBg(g, row2);

    area.removeFromTop(Pad);
    paintSectionBg(g, area.removeFromTop(row3H));
}

void AISynthEditor::resized()
{
    auto area = getLocalBounds().withTrimmedTop(TopBar).reduced(Pad);

    const int row1H = SectionTitleH + ComboH + Pad + KnobH + Pad;
    const int row2H = SectionTitleH + KnobH + Pad;
    const int row3H = SectionTitleH + KnobH + Pad;

    // -----------------------------------------------------------------------
    // Row 1: Oscillator | Filter | Master
    // -----------------------------------------------------------------------
    auto row1 = area.removeFromTop(row1H);

    // Oscillator
    {
        auto sec = row1.removeFromLeft(170).reduced(4, 4);
        lblOsc.setBounds(sec.removeFromTop(SectionTitleH));
        oscWaveformBox.setBounds(sec.removeFromTop(ComboH).reduced(0, 2));
        sec.removeFromTop(Pad);
        layoutKnobs(sec, { &knobDetune, &knobOctave });
    }
    row1.removeFromLeft(Pad);

    // Filter
    {
        auto sec = row1.removeFromLeft(235).reduced(4, 4);
        lblFilter.setBounds(sec.removeFromTop(SectionTitleH));
        filterTypeBox.setBounds(sec.removeFromTop(ComboH).reduced(0, 2));
        sec.removeFromTop(Pad);
        layoutKnobs(sec, { &knobCutoff, &knobResonance, &knobFilterEnv });
    }
    row1.removeFromLeft(Pad);

    // Master (remaining space in row 1)
    {
        auto sec = row1.reduced(4, 4);
        lblMaster.setBounds(sec.removeFromTop(SectionTitleH));
        sec.removeFromTop(ComboH + Pad); // align knob with other row-1 knobs
        knobMaster.setBounds(sec.withSizeKeepingCentre(80, KnobH));
    }

    area.removeFromTop(Pad);

    // -----------------------------------------------------------------------
    // Row 2: Amp Envelope | Filter Envelope
    // -----------------------------------------------------------------------
    auto row2 = area.removeFromTop(row2H);

    {
        auto sec = row2.removeFromLeft(225).reduced(4, 4);
        lblAmpEnv.setBounds(sec.removeFromTop(SectionTitleH));
        layoutKnobs(sec, { &knobAmpA, &knobAmpD, &knobAmpS, &knobAmpR });
    }
    row2.removeFromLeft(Pad);
    {
        auto sec = row2.reduced(4, 4);
        lblFltEnv.setBounds(sec.removeFromTop(SectionTitleH));
        layoutKnobs(sec, { &knobFltA, &knobFltD, &knobFltS, &knobFltR });
    }

    area.removeFromTop(Pad);

    // -----------------------------------------------------------------------
    // Row 3: Delay
    // -----------------------------------------------------------------------
    {
        auto sec = area.removeFromTop(row3H).reduced(4, 4);
        lblDelay.setBounds(sec.removeFromTop(SectionTitleH));
        // Delay knobs left-aligned, rest stays empty for future FX
        auto delayKnobArea = sec.removeFromLeft(3 * 80);
        layoutKnobs(delayKnobArea, { &knobDelayTime, &knobDelayFeedback, &knobDelayMix });
    }
}

void AISynthEditor::layoutKnobs(juce::Rectangle<int> bounds,
                                 std::initializer_list<juce::Component*> knobs)
{
    const int n = static_cast<int>(knobs.size());
    if (n == 0) return;
    const int w = bounds.getWidth() / n;
    int x = bounds.getX();
    for (auto* k : knobs) {
        k->setBounds(x, bounds.getY(), w, bounds.getHeight());
        x += w;
    }
}
