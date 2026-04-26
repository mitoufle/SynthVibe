#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "AI/ClaudeClient.h"
#include "AI/ApiKeyStore.h"
#include "AI/JuceHttpTransport.h"
#include "AI/ParamIdIndex.h"

struct ClaudeClientLiveTests : public juce::UnitTest
{
    ClaudeClientLiveTests() : juce::UnitTest("ClaudeClientLive", "AISynth") {}

    void runTest() override
    {
        const auto envKey = juce::SystemStats::getEnvironmentVariable("ANTHROPIC_API_KEY", "");
        if (envKey.isEmpty())
        {
            beginTest("live: skipped (ANTHROPIC_API_KEY not set)");
            // No expects — skipping by design. CI runs offline.
            return;
        }

        beginTest("live: real Anthropic call returns at least 1 parseable Variation");
        {
            auto keyPath = juce::File::createTempFile("AISynth-livekey.json");
            keyPath.deleteFile();
            ApiKeyStore keyStore(keyPath);
            keyStore.save(envKey);

            JuceHttpTransport transport;
            ClaudeClient client(keyStore, transport);
            client.setTimeoutSeconds(30);

            std::atomic<bool> called { false };
            ClaudeResponse received;
            client.requestPatches("warm pad with slow attack", 1,
                                  [&](ClaudeResponse r) { received = std::move(r); called = true; });

            const auto deadline = juce::Time::getMillisecondCounter() + 35000u;
            while (! called.load() && juce::Time::getMillisecondCounter() < deadline)
                juce::MessageManager::getInstance()->runDispatchLoopUntil(50);

            expect(called.load(), "live: callback never fired (timed out 35s)");
            expect(received.error == ClaudeClientError::None,
                   "live: expected error=None, got " + juce::String((int) received.error));
            expect(received.variations.size() >= 1, "live: expected >=1 variation");

            // Sanity: at least one returned paramId should resolve in ParamIdIndex.
            int known = 0;
            for (auto& [id, _] : received.variations[0].params)
                if (ParamIdIndex::find(id) != nullptr)
                    ++known;
            expect(known > 0, "live: returned variation has no known paramIds");

            keyPath.deleteFile();
        }
    }
};

static ClaudeClientLiveTests _claudeClientLiveTests;
