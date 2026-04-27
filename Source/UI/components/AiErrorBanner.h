#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class AiErrorBanner : public juce::Component
    {
    public:
        enum class Action { None, SetApiKey, Retry };

        std::function<void(Action)> onActionClicked;

        AiErrorBanner()
        {
            messageLabel.setColour(juce::Label::textColourId, Tokens::ink);
            messageLabel.setFont(Fonts::sans(Tokens::Font::body));
            messageLabel.setJustificationType(juce::Justification::centredLeft);
            addAndMakeVisible(messageLabel);

            actionButton.setColour(juce::TextButton::buttonColourId, Tokens::panel);
            actionButton.setColour(juce::TextButton::textColourOnId,  Tokens::accent);
            actionButton.setColour(juce::TextButton::textColourOffId, Tokens::accent);
            actionButton.onClick = [this] {
                if (onActionClicked) onActionClicked(currentAction);
            };
            addChildComponent(actionButton);
            setVisible(false);
        }

        void show(const juce::String& msg, Action action)
        {
            currentAction = action;
            currentMessage = msg;
            messageLabel.setText(msg, juce::dontSendNotification);

            switch (action)
            {
                case Action::SetApiKey:
                    actionButton.setButtonText("Set API key \xE2\x86\x92");   // " → "
                    actionButton.setVisible(true);
                    break;
                case Action::Retry:
                    actionButton.setButtonText("Retry");
                    actionButton.setVisible(true);
                    break;
                case Action::None:
                    actionButton.setVisible(false);
                    break;
            }
            setVisible(true);
            resized();
            repaint();
        }

        void hide()
        {
            currentAction = Action::None;
            currentMessage = {};
            setVisible(false);
        }

        juce::String getMessageText() const { return currentMessage; }
        Action       getAction()      const { return currentAction; }

        void paint(juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat();
            g.setColour(juce::Colour(0xFF3A1F2A));   // muted error background
            g.fillRoundedRectangle(b, Tokens::radiusSm);
            g.setColour(Tokens::filter);             // pink edge — section hue for "warning"
            g.drawRoundedRectangle(b, Tokens::radiusSm, 1.0f);
        }

        void resized() override
        {
            auto b = getLocalBounds().reduced(Tokens::spaceSm, 0);
            if (actionButton.isVisible())
                actionButton.setBounds(b.removeFromRight(110).reduced(0, 4));
            messageLabel.setBounds(b);
        }

    private:
        juce::Label      messageLabel;
        juce::TextButton actionButton;
        Action           currentAction  = Action::None;
        juce::String     currentMessage;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AiErrorBanner)
    };
}
