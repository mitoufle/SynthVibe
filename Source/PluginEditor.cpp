#include "PluginEditor.h"
#include "UI/DesignTokens.h"

AISynthEditor::AISynthEditor(AISynthProcessor& p)
    : AudioProcessorEditor(&p), processor(p),
      topBar(p.apvts, p.presetManager),
      bottomBar([&p] { return p.getActiveVoiceCount(); }),
      soundTab(p.apvts),
      modTab(p.apvts),
      fxTab(p.apvts),
      arpTab(p.apvts)
{
    setLookAndFeel(&laf);
    setSize(1280, 720);

    const juce::StringArray tabNames { "SOUND", "MOD", "FX", "ARP" };
    for (int i = 0; i < 4; ++i)
    {
        tabButtons[i].setButtonText(tabNames[i]);
        tabButtons[i].setClickingTogglesState(false);
        tabButtons[i].onClick = [this, i] { showTab(i); };
        addAndMakeVisible(tabButtons[i]);
    }

    addAndMakeVisible(topBar);
    addAndMakeVisible(bottomBar);
    addAndMakeVisible(soundTab);
    addAndMakeVisible(modTab);
    addAndMakeVisible(fxTab);
    addAndMakeVisible(arpTab);

    showTab(0);
}

AISynthEditor::~AISynthEditor()
{
    setLookAndFeel(nullptr);
}

void AISynthEditor::paint(juce::Graphics& g)
{
    g.fillAll(SynthVibe::Tokens::bg);
}

void AISynthEditor::resized()
{
    const int topBarH    = 38;
    const int tabBarH    = 30;
    const int bottomBarH = 26;
    const int pad        = 6;

    auto area = getLocalBounds().reduced(pad);

    topBar.setBounds(area.removeFromTop(topBarH));
    area.removeFromTop(pad);

    auto tabRow = area.removeFromTop(tabBarH);
    const int tabW = tabRow.getWidth() / 4;
    for (int i = 0; i < 4; ++i)
        tabButtons[i].setBounds(tabRow.removeFromLeft(tabW).reduced(2, 0));

    area.removeFromTop(pad);
    bottomBar.setBounds(area.removeFromBottom(bottomBarH));
    area.removeFromBottom(pad);

    soundTab.setBounds(area);
    modTab.setBounds(area);
    fxTab.setBounds(area);
    arpTab.setBounds(area);
}

void AISynthEditor::showTab(int index)
{
    currentTab = index;
    soundTab.setVisible(index == 0);
    modTab  .setVisible(index == 1);
    fxTab   .setVisible(index == 2);
    arpTab  .setVisible(index == 3);

    for (int i = 0; i < 4; ++i)
        tabButtons[i].setToggleState(i == index, juce::dontSendNotification);

    repaint();
}
