#pragma once
#include <juce_core/juce_core.h>

struct HttpResponse
{
    int           status   = 0;       // 0 = network failure or not yet sent
    juce::String  body;               // empty on network failure
    bool          timedOut = false;
};

class Transport
{
public:
    virtual ~Transport() = default;

    virtual HttpResponse post(const juce::URL&             url,
                              const juce::StringPairArray& headers,
                              const juce::String&          body,
                              int                          timeoutMs) = 0;
};
