#include "JuceHttpTransport.h"

namespace
{
    juce::String buildExtraHeaders(const juce::StringPairArray& headers)
    {
        juce::String s;
        for (auto& k : headers.getAllKeys())
            s << k << ": " << headers[k] << "\r\n";
        return s;
    }
}

HttpResponse JuceHttpTransport::post(const juce::URL&             url,
                                     const juce::StringPairArray& headers,
                                     const juce::String&          body,
                                     int                          timeoutMs)
{
    HttpResponse out;

    auto withBody = url.withPOSTData(body);

    auto stream = withBody.createInputStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
            .withConnectionTimeoutMs(timeoutMs)
            .withExtraHeaders(buildExtraHeaders(headers))
            .withHttpRequestCmd("POST"));

    if (stream == nullptr)
    {
        out.status = 0;
        out.timedOut = false;   // best we can know with the basic API; treat null as network failure
        return out;
    }

    if (auto* webStream = dynamic_cast<juce::WebInputStream*>(stream.get()))
        out.status = webStream->getStatusCode();

    out.body = stream->readEntireStreamAsString();
    return out;
}
