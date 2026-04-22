#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class FilterTypeSelect : public juce::Component
    {
    public:
        FilterTypeSelect(juce::AudioProcessorValueTreeState& apvts,
                         const juce::String& paramID)
        {
            const char* labels[] = { "LP", "HP", "BP" };
            for (int i = 0; i < 3; ++i)
            {
                auto& btn = pills[i];
                btn.setButtonText(labels[i]);
                btn.setClickingTogglesState(true);
                btn.setRadioGroupId(1);
                btn.onClick = [this, i, &apvts, paramID]
                {
                    if (auto* p = apvts.getParameter(paramID))
                        p->setValueNotifyingHost(p->convertTo0to1((float) i));
                };
                addAndMakeVisible(btn);
            }

            paramAttach = std::make_unique<juce::ParameterAttachment>(
                *apvts.getParameter(paramID),
                [this](float v) {
                    const int idx = juce::jlimit(0, 2, (int) std::round(v));
                    for (int i = 0; i < 3; ++i)
                        pills[i].setToggleState(i == idx, juce::dontSendNotification);
                });
            paramAttach->sendInitialUpdate();
        }

        void resized() override
        {
            auto b = getLocalBounds();
            const int w = b.getWidth() / 3;
            for (int i = 0; i < 3; ++i)
                pills[i].setBounds(b.removeFromLeft(w).reduced(2));
        }

    private:
        juce::TextButton pills[3];
        std::unique_ptr<juce::ParameterAttachment> paramAttach;
    };
}
