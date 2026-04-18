#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Local HTTP server that exposes synth parameters as a REST API.
// This is the integration point for:
//   - The MCP server (external Claude control via tool calls)
//   - The embedded AI prompt UI (direct Claude API calls)
//
// Endpoints to implement:
//   GET  /params              → returns all current parameter values as JSON
//   POST /params              → sets one or more parameters by ID
//   GET  /presets             → lists available preset names
//   POST /presets/load        → loads a named preset
//   POST /presets/save        → saves current state as a named preset
//
// TODO: implement using a lightweight HTTP lib (e.g. cpp-httplib)
class APIServer
{
public:
    explicit APIServer(juce::AudioProcessorValueTreeState& /*apvts*/) {}

    void start(int /*port*/ = 7890) {}
    void stop() {}
    bool isRunning() const { return false; }
};
