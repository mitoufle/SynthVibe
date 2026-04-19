#pragma once
#include "Voice.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>

class SynthEngine
{
public:
    // Increase NumVoices for more polyphony (power of 2 recommended)
    static constexpr int NumVoices = 8;

    void prepare(const juce::dsp::ProcessSpec& spec);

    // Called each block — pushes the latest APVTS snapshot to all voices
    void setParams(const VoiceParams& p);

    void handleMidiMessage(const juce::MidiMessage& msg);
    void processBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

    int getActiveVoiceCount() const noexcept { return activeVoiceCount.load(std::memory_order_relaxed); }

private:
    std::array<Voice, NumVoices> voices;
    std::atomic<int> activeVoiceCount { 0 };
    VoiceParams currentParams;

    Voice* findFreeVoice() noexcept;
    Voice* findVoiceForNote(int midiNote) noexcept;
};
