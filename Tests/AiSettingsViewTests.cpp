#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "AI/ApiKeyStore.h"
#include "UI/components/AiSettingsView.h"

struct AiSettingsViewTests : public juce::UnitTest
{
    AiSettingsViewTests() : juce::UnitTest("AiSettingsView", "AISynth") {}

    juce::File makeTempPath()
    {
        return juce::File::createTempFile("AISynth-aikey-view-test.json");
    }

    void runTest() override
    {
        beginTest("save persists trimmed key to ApiKeyStore (round-trip via load)");
        {
            auto path = makeTempPath();
            path.deleteFile();
            ApiKeyStore store(path);

            SynthVibe::AiSettingsView view(store);
            view.setBounds(0, 0, 480, 200);
            view.getApiKeyEditor().setText("sk-ant-roundtrip-1", juce::dontSendNotification);
            view.getSaveButton().triggerClick();

            ApiKeyStore store2(path);   // fresh handle to confirm disk persistence
            expectEquals(store2.load(), juce::String("sk-ant-roundtrip-1"));
            path.deleteFile();
        }

        beginTest("clear empties the store");
        {
            auto path = makeTempPath();
            path.deleteFile();
            ApiKeyStore store(path);
            store.save("sk-ant-prefilled");

            SynthVibe::AiSettingsView view(store);
            view.setBounds(0, 0, 480, 200);
            view.refreshFromStore();   // editor pulls "sk-ant-prefilled"
            view.getClearButton().triggerClick();

            ApiKeyStore store2(path);
            expect(store2.load().isEmpty(), "store should be empty after clear");
            expect(view.getApiKeyEditor().getText().isEmpty(), "editor should be empty after clear");
            path.deleteFile();
        }

        beginTest("save trims leading/trailing whitespace on input");
        {
            auto path = makeTempPath();
            path.deleteFile();
            ApiKeyStore store(path);

            SynthVibe::AiSettingsView view(store);
            view.setBounds(0, 0, 480, 200);
            view.getApiKeyEditor().setText("   sk-ant-trim-me   ", juce::dontSendNotification);
            view.getSaveButton().triggerClick();

            ApiKeyStore store2(path);
            expectEquals(store2.load(), juce::String("sk-ant-trim-me"));
            path.deleteFile();
        }
    }
};

static AiSettingsViewTests _aiSettingsViewTests;
