#pragma once
#include <juce_dsp/juce_dsp.h>

// Effects chain — plug in per-block after the synth engine.
// Add effect modules here (Reverb, Delay, Chorus, Distortion…).
// Each effect should be togglable and expose a setParams() method.
//
// TODO: implement and wire up to PluginProcessor::processBlock()
class FXChain
{
public:
    void prepare(const juce::dsp::ProcessSpec& /*spec*/) {}
    void process(juce::AudioBuffer<float>& /*buffer*/)   {}
    void reset() {}
};
