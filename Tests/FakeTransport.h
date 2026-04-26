#pragma once
#include "AI/Transport.h"
#include <atomic>
#include <functional>

// Test double for Transport. Returns a pre-set response; counts calls;
// optionally invokes a per-call hook (used for cancellation tests to
// inject delays or side-effects).
class FakeTransport : public Transport
{
public:
    HttpResponse responseToReturn;
    std::atomic<int> callCount { 0 };
    std::function<void()> onPost;   // optional, runs inside post() after counting

    HttpResponse post(const juce::URL&,
                      const juce::StringPairArray&,
                      const juce::String&,
                      int) override
    {
        ++callCount;
        if (onPost) onPost();
        return responseToReturn;
    }
};
