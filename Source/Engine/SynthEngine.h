#pragma once
#include "Voice.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

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

private:
    std::array<Voice, NumVoices> voices;
    VoiceParams currentParams;

    Voice* findFreeVoice() noexcept;
    Voice* findVoiceForNote(int midiNote) noexcept;
};
