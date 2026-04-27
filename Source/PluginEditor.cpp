#include "PluginEditor.h"
#include "UI/DesignTokens.h"

AISynthEditor::AISynthEditor(AISynthProcessor& p)
    : AudioProcessorEditor(&p), processor(p),
      topBar(p.apvts, p.presetManager),
      bottomBar([&p] { return p.getActiveVoiceCount(); }),
      soundTab(p.apvts),
      modTab(p.apvts),
      fxTab(p.apvts),
      arpTab(p.apvts),
      keyboard(p.keyboardState),
      aiPromptModal(p.claudeClient, p.patchApplier, p.apiKeyStore)
{
    setLookAndFeel(&laf);
    setSize(1280, 720);

    const juce::StringArray tabNames { "SOUND", "MOD", "FX", "ARP" };
    for (int i = 0; i < 4; ++i)
    {
        tabButtons[i].setLabel(tabNames[i]);
        tabButtons[i].onClick = [this, i] { showTab(i); };
        addAndMakeVisible(tabButtons[i]);
    }

    addAndMakeVisible(topBar);
    addAndMakeVisible(bottomBar);
    addAndMakeVisible(soundTab);
    addAndMakeVisible(modTab);
    addAndMakeVisible(fxTab);
    addAndMakeVisible(arpTab);
    addAndMakeVisible(keyboard);

    // Modal sits on top of everything, hidden by default.
    addChildComponent(aiPromptModal);

    topBar.onPromptRequested = [this] { showAiModal(); };

    showTab(0);
}

AISynthEditor::~AISynthEditor()
{
    setLookAndFeel(nullptr);
}

void AISynthEditor::paint(juce::Graphics& g)
{
    using namespace SynthVibe::Tokens;
    g.fillAll(bg);

    // Tab-bar strip sits on panel colour (brighter than bg) with a 1 px edge
    // line on its lower border — matches the mock's sub-header band under
    // the top bar.
    if (! tabBarBounds.isEmpty())
    {
        g.setColour(panel);
        g.fillRect(tabBarBounds);
        g.setColour(edge);
        g.drawHorizontalLine(tabBarBounds.getBottom() - 1,
                             (float) tabBarBounds.getX(),
                             (float) tabBarBounds.getRight());
    }
}

void AISynthEditor::resized()
{
    const int topBarH    = 38;
    const int tabBarH    = 30;
    const int bottomBarH = 26;
    const int keyboardH  = 70;
    const int pad        = 6;

    auto area = getLocalBounds().reduced(pad);

    const auto bp = SynthVibe::breakpointForWidth(getWidth());
    juce::ignoreUnused(bp); // consumed by Phase 3/4 tab rewrites

    topBar.setBounds(area.removeFromTop(topBarH));
    area.removeFromTop(pad);

    auto tabRow = area.removeFromTop(tabBarH);
    tabBarBounds = tabRow; // cached for paint()
    const int tabW = tabRow.getWidth() / 4;
    for (int i = 0; i < 4; ++i)
        tabButtons[i].setBounds(tabRow.removeFromLeft(tabW));

    area.removeFromTop(pad);
    bottomBar.setBounds(area.removeFromBottom(bottomBarH));
    area.removeFromBottom(pad);
    keyboard.setBounds(area.removeFromBottom(keyboardH));
    area.removeFromBottom(pad);

    soundTab.setBounds(area);
    modTab.setBounds(area);
    fxTab.setBounds(area);
    arpTab.setBounds(area);

    aiPromptModal.setBounds(getLocalBounds());
}

void AISynthEditor::showTab(int index)
{
    currentTab = index;
    soundTab.setVisible(index == 0);
    modTab  .setVisible(index == 1);
    fxTab   .setVisible(index == 2);
    arpTab  .setVisible(index == 3);

    for (int i = 0; i < 4; ++i)
        tabButtons[i].setActive(i == index);

    repaint();
}

void AISynthEditor::showAiModal()
{
    aiPromptModal.show();
}
