#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/Breakpoint.h"
#include "UI/LookAndFeel.h"
#include "UI/TopBar.h"
#include "UI/BottomBar.h"
#include "UI/SoundTab.h"
#include "UI/ModTab.h"
#include "UI/FxTab.h"
#include "UI/ArpTab.h"
#include "UI/components/MainTab.h"
#include "UI/components/SynthKeyboard.h"

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

    TopBar    topBar;
    BottomBar bottomBar;

    SynthVibe::MainTab tabButtons[4];
    juce::Rectangle<int> tabBarBounds;
    int currentTab = 0;

    SoundTab soundTab;
    ModTab   modTab;
    FxTab    fxTab;
    ArpTab   arpTab;

    SynthVibe::SynthKeyboard keyboard;

    void showTab(int index);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AISynthEditor)
};
