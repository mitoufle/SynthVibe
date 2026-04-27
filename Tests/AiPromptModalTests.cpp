#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "UI/AiPromptModal.h"
#include "AI/ClaudeClient.h"
#include "AI/PatchApplier.h"
#include "AI/ApiKeyStore.h"
#include "FakeTransport.h"
#include "DummyProcessor.h"

namespace
{
    template <class Pred>
    bool pumpUntil(Pred pred, int timeoutMs = 5000)
    {
        const auto deadline = juce::Time::getMillisecondCounter() + (uint32_t) timeoutMs;
        while (! pred() && juce::Time::getMillisecondCounter() < deadline)
            juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
        return pred();
    }

    juce::String makeOneVariationResponse()
    {
        // Two real APVTS paramIds (see Source/Parameters/ParameterIDs.h):
        //   osc1.level (float 0..1) and filt.cutoff (float, Hz, real-valued range).
        return "{\"content\":[{\"type\":\"tool_use\",\"name\":\"set_patch\","
               "\"input\":{\"name\":\"P\",\"params\":"
               "{\"osc1.level\":0.6,\"filt.cutoff\":1500.0}}}]}";
    }

    juce::String makeFourVariationResponse()
    {
        juce::String s = "{\"content\":[";
        const char* names[] = { "Soft Pad", "Glass Pad", "Reedy Pad", "Air Pad" };
        for (int i = 0; i < 4; ++i)
        {
            if (i > 0) s << ",";
            s << "{\"type\":\"tool_use\",\"name\":\"set_patch\",\"input\":{\"name\":\""
              << names[i] << "\",\"params\":{\"osc1.level\":0.5}}}";
        }
        s << "]}";
        return s;
    }

    struct GestureCounter : public juce::AudioProcessorParameter::Listener
    {
        std::atomic<int> begins { 0 };
        std::atomic<int> ends   { 0 };
        void parameterValueChanged(int, float) override {}
        void parameterGestureChanged(int, bool starting) override
        {
            if (starting) ++begins; else ++ends;
        }
    };

    struct ModalFixture
    {
        DummyProcessor proc;
        juce::File     keyPath { juce::File::createTempFile("AISynth-modal.json") };
        ApiKeyStore    keyStore;
        FakeTransport  transport;
        ClaudeClient   client;
        PatchApplier   applier;
        AiPromptModal  modal;

        ModalFixture()
            : keyStore(keyPath),
              client(keyStore, transport),
              applier(proc.apvts),
              modal(client, applier, keyStore, proc.apvts)
        {
            keyPath.deleteFile();   // start empty; tests opt-in by calling keyStore.save(...)
            modal.setBounds(0, 0, 1280, 720);
        }

        ~ModalFixture() { keyPath.deleteFile(); }
    };
}

struct AiPromptModalTests : public juce::UnitTest
{
    AiPromptModalTests() : juce::UnitTest("AiPromptModal", "AISynth") {}

    void runTest() override
    {
        beginTest("requestGenerate with empty prompt is no-op (transport never called)");
        {
            ModalFixture fx;
            fx.keyStore.save("sk-ant-test");
            fx.transport.responseToReturn = HttpResponse{ 200, makeFourVariationResponse(), false };

            fx.modal.requestGenerate("   ");
            // No async call expected — transport.callCount stays 0.
            juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
            expectEquals(fx.transport.callCount.load(), 0);
            expect(! fx.modal.isLoading());
        }

        beginTest("requestGenerate with no API key sets MissingApiKey banner, transport never called");
        {
            ModalFixture fx;
            // No keyStore.save() — store is empty.
            fx.transport.responseToReturn = HttpResponse{ 200, makeFourVariationResponse(), false };

            fx.modal.requestGenerate("warm pad");
            juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
            expectEquals(fx.transport.callCount.load(), 0);
            expectEquals(fx.modal.getErrorBannerText(), juce::String("No API key set."));
        }

        beginTest("requestGenerate happy path: 4 valid Variations populate 4 cards");
        {
            ModalFixture fx;
            fx.keyStore.save("sk-ant-test");
            fx.transport.responseToReturn = HttpResponse{ 200, makeFourVariationResponse(), false };

            fx.modal.requestGenerate("warm pad");
            expect(pumpUntil([&] { return ! fx.modal.isLoading() && fx.modal.getVariationCount() > 0; }),
                   "callback never fired");
            expectEquals(fx.modal.getVariationCount(), 4);
            expect(fx.modal.getErrorBannerText().isEmpty(), "banner should be empty on happy path");
        }

        beginTest("Generate during in-flight is debounced (state == Loading prevents re-entry)");
        {
            ModalFixture fx;
            fx.keyStore.save("sk-ant-test");
            fx.transport.responseToReturn = HttpResponse{ 200, makeFourVariationResponse(), false };
            fx.transport.onPost = [] { juce::Thread::sleep(150); };

            fx.modal.requestGenerate("first");        // sets Loading; transport call 1
            juce::Thread::sleep(20);
            fx.modal.requestGenerate("second");       // gated; no transport call
            expect(pumpUntil([&] { return ! fx.modal.isLoading() && fx.modal.getVariationCount() > 0; }));
            // Only one transport call total.
            expectEquals(fx.transport.callCount.load(), 1);
        }

        beginTest("requestGenerate with HttpError 401 sets ErrorBanner to API key rejected");
        {
            ModalFixture fx;
            fx.keyStore.save("sk-ant-bad");
            fx.transport.responseToReturn = HttpResponse{ 401, "{\"error\":\"unauthorized\"}", false };

            fx.modal.requestGenerate("warm pad");
            expect(pumpUntil([&] { return ! fx.modal.isLoading()
                                          && ! fx.modal.getErrorBannerText().isEmpty(); }));
            expectEquals(fx.modal.getErrorBannerText(),
                         juce::String("API key rejected by Anthropic."));
        }

        beginTest("requestGenerate with NetworkFailure sets ErrorBanner to Could not reach");
        {
            ModalFixture fx;
            fx.keyStore.save("sk-ant-test");
            fx.transport.responseToReturn = HttpResponse{ 0, {}, false };

            fx.modal.requestGenerate("warm pad");
            expect(pumpUntil([&] { return ! fx.modal.isLoading()
                                          && ! fx.modal.getErrorBannerText().isEmpty(); }));
            expectEquals(fx.modal.getErrorBannerText(),
                         juce::String("Could not reach Anthropic API."));
        }

        beginTest("requestGenerate with SchemaError sets ErrorBanner to Claude returned no patches");
        {
            ModalFixture fx;
            fx.keyStore.save("sk-ant-test");
            fx.transport.responseToReturn = HttpResponse{ 200,
                "{\"content\":[{\"type\":\"text\",\"text\":\"sorry\"}]}", false };

            fx.modal.requestGenerate("warm pad");
            expect(pumpUntil([&] { return ! fx.modal.isLoading()
                                          && ! fx.modal.getErrorBannerText().isEmpty(); }));
            expectEquals(fx.modal.getErrorBannerText(),
                         juce::String("Claude returned no patches. Try a different prompt."));
        }

        beginTest("selectAndApply(i) writes APVTS via gestures (1 begin + 1 end per applied param)");
        {
            ModalFixture fx;
            fx.keyStore.save("sk-ant-test");
            // Variation has 2 known params: osc1.level (float) + filt.cutoff (float).
            fx.transport.responseToReturn = HttpResponse{ 200, makeOneVariationResponse(), false };

            fx.modal.requestGenerate("warm");
            expect(pumpUntil([&] { return ! fx.modal.isLoading() && fx.modal.getVariationCount() > 0; }));

            GestureCounter counter;
            for (auto* p : fx.proc.getParameters()) p->addListener(&counter);

            fx.modal.selectAndApply(0);

            for (auto* p : fx.proc.getParameters()) p->removeListener(&counter);

            expectEquals(counter.begins.load(), 2);
            expectEquals(counter.ends  .load(), 2);
            expectEquals(fx.modal.getSelectedCardIndex(), 0);
        }

        beginTest("selectAndApply on a card with empty params still highlights but fires no gestures");
        {
            ModalFixture fx;
            fx.keyStore.save("sk-ant-test");
            // Single Variation with NO params.
            fx.transport.responseToReturn = HttpResponse{ 200,
                "{\"content\":[{\"type\":\"tool_use\",\"name\":\"set_patch\","
                "\"input\":{\"name\":\"E\",\"params\":{}}}]}", false };

            fx.modal.requestGenerate("x");
            expect(pumpUntil([&] { return ! fx.modal.isLoading() && fx.modal.getVariationCount() > 0; }));

            GestureCounter counter;
            for (auto* p : fx.proc.getParameters()) p->addListener(&counter);

            fx.modal.selectAndApply(0);

            for (auto* p : fx.proc.getParameters()) p->removeListener(&counter);

            expectEquals(counter.begins.load(), 0);
            expectEquals(counter.ends  .load(), 0);
            // Per spec: empty-params variations still highlight (selection recorded)
            // even though no gestures fire. The audition snapshot is still captured
            // so a subsequent click on a non-empty card cleanly restores baseline.
            expectEquals(fx.modal.getSelectedCardIndex(), 0);
        }

        beginTest("re-clicking the same card from baseline produces identical APVTS state");
        {
            // Audition invariant: V1 -> V2 -> V1 must produce the same state as
            // a fresh V1 click, even when V2 mentions params V1 doesn't.
            // PatchApplier expects 0..1 normalized values for non-index params,
            // so we use 0.3 for filt.cutoff (lands somewhere mid-range, distinct
            // from baseline).
            ModalFixture fx;
            fx.keyStore.save("sk-ant-test");
            fx.transport.responseToReturn = HttpResponse{ 200,
                "{\"content\":["
                  "{\"type\":\"tool_use\",\"name\":\"set_patch\",\"input\":{\"name\":\"V1\","
                    "\"params\":{\"osc1.level\":0.6}}},"
                  "{\"type\":\"tool_use\",\"name\":\"set_patch\",\"input\":{\"name\":\"V2\","
                    "\"params\":{\"filt.cutoff\":0.3}}}"
                "]}", false };

            fx.modal.requestGenerate("p");
            expect(pumpUntil([&] { return ! fx.modal.isLoading() && fx.modal.getVariationCount() == 2; }));

            // Capture the param value that V2 touches but V1 doesn't.
            const float baselineCutoff = fx.proc.apvts.getRawParameterValue("filt.cutoff")->load();

            fx.modal.selectAndApply(0);   // V1: sets osc1.level only
            const float cutoffAfterV1   = fx.proc.apvts.getRawParameterValue("filt.cutoff")->load();
            fx.modal.selectAndApply(1);   // V2: sets filt.cutoff (osc1.level should restore to baseline first)
            const float cutoffAfterV2   = fx.proc.apvts.getRawParameterValue("filt.cutoff")->load();
            fx.modal.selectAndApply(0);   // V1 again: filt.cutoff should be back to baseline
            const float cutoffAfterV1b  = fx.proc.apvts.getRawParameterValue("filt.cutoff")->load();

            expectWithinAbsoluteError(cutoffAfterV1, baselineCutoff, 0.01f);   // V1 didn't touch cutoff
            expect(cutoffAfterV2 != baselineCutoff, "V2 must change cutoff");
            expectWithinAbsoluteError(cutoffAfterV1b, baselineCutoff, 0.01f);  // V1 revisit restores cutoff
        }
    }
};

static AiPromptModalTests _aiPromptModalTests;
