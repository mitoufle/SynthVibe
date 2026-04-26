#pragma once
#include "Transport.h"

class JuceHttpTransport : public Transport
{
public:
    HttpResponse post(const juce::URL&             url,
                      const juce::StringPairArray& headers,
                      const juce::String&          body,
                      int                          timeoutMs) override;
};
