#pragma once
#include "Variation.h"
#include "Transport.h"
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <atomic>
#include <functional>
#include <memory>

class ApiKeyStore;

enum class ClaudeClientError
{
    None,
    MissingApiKey,
    NetworkFailure,
    Timeout,
    HttpError,
    ParseError,
    SchemaError
};

struct ClaudeResponse
{
    ClaudeClientError error = ClaudeClientError::None;
    int httpStatus = 0;
    juce::String message;
    std::vector<Variation> variations;
};

class ClaudeClient
{
public:
    ClaudeClient(ApiKeyStore& keyStore, Transport& transport);
    ~ClaudeClient();

    void requestPatches(juce::String prompt,
                        int numVariations,
                        std::function<void(ClaudeResponse)> callback);

    void setModel(juce::String model);
    void setTimeoutSeconds(int seconds);

private:
    class Worker;

    ApiKeyStore& keyStore;
    Transport&   transport;
    juce::String model { "claude-sonnet-4-6" };
    int          timeoutSeconds { 40 };
    std::atomic<uint64_t> generation { 0 };
    std::atomic<bool>     shutdownFlag { false };
    std::unique_ptr<Worker> worker;

    JUCE_DECLARE_WEAK_REFERENCEABLE(ClaudeClient)
};
