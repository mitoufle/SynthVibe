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

        beginTest("requestPatches with 1 valid tool_use block returns 1 Variation");
        {
            auto keyPath = juce::File::createTempFile("AISynth-key.json");
            keyPath.deleteFile();
            ApiKeyStore keyStore(keyPath);
            keyStore.save("sk-ant-test");

            FakeTransport transport;
            transport.responseToReturn = HttpResponse{ 200,
                "{\"content\":[{\"type\":\"tool_use\",\"name\":\"set_patch\","
                "\"input\":{\"name\":\"X\",\"params\":{\"osc1.level\":0.5}}}]}",
                false };

            ClaudeClient client(keyStore, transport);
            std::atomic<bool> called { false };
            ClaudeResponse received;
            client.requestPatches("p", 1, [&](ClaudeResponse r) { received = std::move(r); called = true; });
            expect(pumpUntil([&] { return called.load(); }));
            expect(received.error == ClaudeClientError::None);
            expectEquals((int) received.variations.size(), 1);
            keyPath.deleteFile();
        }

        beginTest("tool_use blocks with name != set_patch are filtered out");
        {
            auto keyPath = juce::File::createTempFile("AISynth-key.json");
            keyPath.deleteFile();
            ApiKeyStore keyStore(keyPath);
            keyStore.save("sk-ant-test");

            FakeTransport transport;
            transport.responseToReturn = HttpResponse{ 200,
                "{\"content\":["
                  "{\"type\":\"tool_use\",\"name\":\"other_tool\",\"input\":{}},"
                  "{\"type\":\"text\",\"text\":\"hi\"},"
                  "{\"type\":\"tool_use\",\"name\":\"set_patch\","
                    "\"input\":{\"name\":\"Y\",\"params\":{\"osc2.level\":0.4}}}"
                "]}",
                false };

            ClaudeClient client(keyStore, transport);
            std::atomic<bool> called { false };
            ClaudeResponse received;
            client.requestPatches("p", 1, [&](ClaudeResponse r) { received = std::move(r); called = true; });
            expect(pumpUntil([&] { return called.load(); }));
            expect(received.error == ClaudeClientError::None);
            expectEquals((int) received.variations.size(), 1);
            expectEquals(received.variations[0].name, juce::String("Y"));
            keyPath.deleteFile();
        }

        beginTest("response with no tool_use blocks returns SchemaError");
        {
            auto keyPath = juce::File::createTempFile("AISynth-key.json");
            keyPath.deleteFile();
            ApiKeyStore keyStore(keyPath);
            keyStore.save("sk-ant-test");

            FakeTransport transport;
            transport.responseToReturn = HttpResponse{ 200,
                "{\"content\":[{\"type\":\"text\",\"text\":\"sorry\"}]}", false };

            ClaudeClient client(keyStore, transport);
            std::atomic<bool> called { false };
            ClaudeResponse received;
            client.requestPatches("p", 1, [&](ClaudeResponse r) { received = std::move(r); called = true; });
            expect(pumpUntil([&] { return called.load(); }));
            expect(received.error == ClaudeClientError::SchemaError);
            expect(received.variations.empty());
            keyPath.deleteFile();
        }

        beginTest("malformed JSON returns ParseError");
        {
            auto keyPath = juce::File::createTempFile("AISynth-key.json");
            keyPath.deleteFile();
            ApiKeyStore keyStore(keyPath);
            keyStore.save("sk-ant-test");

            FakeTransport transport;
            transport.responseToReturn = HttpResponse{ 200, "{not_valid_json", false };

            ClaudeClient client(keyStore, transport);
            std::atomic<bool> called { false };
            ClaudeResponse received;
            client.requestPatches("p", 1, [&](ClaudeResponse r) { received = std::move(r); called = true; });
            expect(pumpUntil([&] { return called.load(); }));
            expect(received.error == ClaudeClientError::ParseError);
            keyPath.deleteFile();
        }

        beginTest("non-2xx HTTP returns HttpError with status");
        {
            auto keyPath = juce::File::createTempFile("AISynth-key.json");
            keyPath.deleteFile();
            ApiKeyStore keyStore(keyPath);
            keyStore.save("sk-ant-test");

            FakeTransport transport;
            transport.responseToReturn = HttpResponse{ 401, "{\"error\":\"unauthorized\"}", false };

            ClaudeClient client(keyStore, transport);
            std::atomic<bool> called { false };
            ClaudeResponse received;
            client.requestPatches("p", 1, [&](ClaudeResponse r) { received = std::move(r); called = true; });
            expect(pumpUntil([&] { return called.load(); }));
            expect(received.error == ClaudeClientError::HttpError);
            expectEquals(received.httpStatus, 401);
            keyPath.deleteFile();
        }

        beginTest("network failure (status==0) returns NetworkFailure");
        {
            auto keyPath = juce::File::createTempFile("AISynth-key.json");
            keyPath.deleteFile();
            ApiKeyStore keyStore(keyPath);
            keyStore.save("sk-ant-test");

            FakeTransport transport;
            transport.responseToReturn = HttpResponse{ 0, {}, false };

            ClaudeClient client(keyStore, transport);
            std::atomic<bool> called { false };
            ClaudeResponse received;
            client.requestPatches("p", 1, [&](ClaudeResponse r) { received = std::move(r); called = true; });
            expect(pumpUntil([&] { return called.load(); }));
            expect(received.error == ClaudeClientError::NetworkFailure);
            keyPath.deleteFile();
        }

        beginTest("timedOut response returns Timeout");
        {
            auto keyPath = juce::File::createTempFile("AISynth-key.json");
            keyPath.deleteFile();
            ApiKeyStore keyStore(keyPath);
            keyStore.save("sk-ant-test");

            FakeTransport transport;
            transport.responseToReturn = HttpResponse{ 0, {}, true };

            ClaudeClient client(keyStore, transport);
            std::atomic<bool> called { false };
            ClaudeResponse received;
            client.requestPatches("p", 1, [&](ClaudeResponse r) { received = std::move(r); called = true; });
            expect(pumpUntil([&] { return called.load(); }));
            expect(received.error == ClaudeClientError::Timeout);
            keyPath.deleteFile();
        }

        beginTest("missing API key returns MissingApiKey, transport not called");
        {
            auto keyPath = juce::File::createTempFile("AISynth-key.json");
            keyPath.deleteFile();
            ApiKeyStore keyStore(keyPath);   // never saved

            FakeTransport transport;
            transport.responseToReturn = HttpResponse{ 200, "{}", false };

            ClaudeClient client(keyStore, transport);
            std::atomic<bool> called { false };
            ClaudeResponse received;
            client.requestPatches("p", 1, [&](ClaudeResponse r) { received = std::move(r); called = true; });
            expect(pumpUntil([&] { return called.load(); }));
            expect(received.error == ClaudeClientError::MissingApiKey);
            expectEquals(transport.callCount.load(), 0);
            keyPath.deleteFile();
        }
    }
};

static ClaudeClientTests _claudeClientTests;
