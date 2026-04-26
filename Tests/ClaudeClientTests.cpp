#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "AI/ClaudeClient.h"
#include "AI/ApiKeyStore.h"
#include "FakeTransport.h"

namespace
{
    juce::String makeFourVariationResponse()
    {
        // Anthropic-shaped response with 4 set_patch tool_use blocks.
        juce::String s;
        s << "{\"id\":\"msg_x\",\"type\":\"message\",\"role\":\"assistant\","
          << "\"content\":[";

        const char* names[] = { "Soft Pad", "Glass Pad", "Reedy Pad", "Air Pad" };
        for (int i = 0; i < 4; ++i)
        {
            if (i > 0) s << ",";
            s << "{\"type\":\"tool_use\",\"id\":\"toolu_" << i
              << "\",\"name\":\"set_patch\",\"input\":{"
              << "\"name\":\"" << names[i] << "\","
              << "\"params\":{\"osc1.level\":0.7,\"filt.cutoff\":0." << (3 + i) << "}}}";
        }
        s << "],\"stop_reason\":\"tool_use\"}";
        return s;
    }

    // Spin the message thread until cb fires or timeout.
    template <class Pred>
    bool pumpUntil(Pred pred, int timeoutMs = 5000)
    {
        const auto deadline = juce::Time::getMillisecondCounter() + (uint32_t) timeoutMs;
        while (! pred() && juce::Time::getMillisecondCounter() < deadline)
            juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
        return pred();
    }
}

struct ClaudeClientTests : public juce::UnitTest
{
    ClaudeClientTests() : juce::UnitTest("ClaudeClient", "AISynth") {}

    void runTest() override
    {
        beginTest("requestPatches with 4 valid tool_use blocks parses 4 Variations");
        {
            auto keyPath = juce::File::createTempFile("AISynth-key.json");
            keyPath.deleteFile();
            ApiKeyStore keyStore(keyPath);
            keyStore.save("sk-ant-test");

            FakeTransport transport;
            transport.responseToReturn = HttpResponse{ 200, makeFourVariationResponse(), false };

            ClaudeClient client(keyStore, transport);

            std::atomic<bool> called { false };
            ClaudeResponse received;
            client.requestPatches("warm pad", 4, [&](ClaudeResponse r) {
                received = std::move(r);
                called = true;
            });

            expect(pumpUntil([&] { return called.load(); }), "callback never fired");
            expect(received.error == ClaudeClientError::None, "expected None, got error");
            expectEquals((int) received.variations.size(), 4);
            expectEquals(received.variations[0].name, juce::String("Soft Pad"));
            expect(received.variations[0].params.count("osc1.level") == 1, "missing osc1.level");
            expect(received.variations[0].params.count("filt.cutoff") == 1, "missing filt.cutoff");

            keyPath.deleteFile();
        }
    }
};

static ClaudeClientTests _claudeClientTests;
