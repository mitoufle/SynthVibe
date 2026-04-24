#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class CurveSelect : public juce::Component
    {
    public:
        CurveSelect(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID)
        {
            const char* labels[] = { "lin", "exp", "log", "s" };
            for (int i = 0; i < 4; ++i)
            {
                auto& btn = pills[i];
                btn.setButtonText(labels[i]);
                btn.setClickingTogglesState(true);
                btn.setRadioGroupId(41);
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
                    currentIndex = juce::jlimit(0, 3, (int) std::round(v));
                    for (int i = 0; i < 4; ++i)
                        pills[i].setToggleState(i == currentIndex, juce::dontSendNotification);
                });
            paramAttach->sendInitialUpdate();
        }

        int getCurrentIndex() const noexcept { return currentIndex; }

        void resized() override
        {
            auto b = getLocalBounds();
            const int w = b.getWidth() / 4;
            for (int i = 0; i < 4; ++i)
                pills[i].setBounds(b.removeFromLeft(w).reduced(1));
        }

    private:
        juce::TextButton pills[4];
        int currentIndex = 0;
        std::unique_ptr<juce::ParameterAttachment> paramAttach;
    };
}
