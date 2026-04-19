#include "SynthEngine.h"

void SynthEngine::prepare(const juce::dsp::ProcessSpec& spec)
{
    for (auto& v : voices)
        v.prepare(spec);
}

void SynthEngine::setParams(const VoiceParams& p)
{
    currentParams = p;
    for (auto& v : voices)
        v.setParams(p);
}

void SynthEngine::handleMidiMessage(const juce::MidiMessage& msg)
{
    if (msg.isNoteOn())
    {
        Voice* v = findVoiceForNote(msg.getNoteNumber()); // retrigger same note
        if (!v) v = findFreeVoice();
        if (v)  v->noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
    }
    else if (msg.isNoteOff())
    {
        if (Voice* v = findVoiceForNote(msg.getNoteNumber()))
            v->noteOff();
    }
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
    {
        for (auto& v : voices) v.noteOff();
    }
}

void SynthEngine::processBlock(juce::AudioBuffer<float>& buffer,
                                int startSample, int numSamples)
{
    const float gain = 1.f / static_cast<float>(NumVoices);

    for (int i = startSample; i < startSample + numSamples; ++i)
    {
        float sample = 0.f;
        for (auto& v : voices)
            if (v.isActive()) sample += v.getNextSample();

        sample *= gain;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.getWritePointer(ch)[i] += sample;
    }

    // Update active voice count for the UI (message-thread read via atomic)
    int count = 0;
    for (const auto& v : voices)
        if (v.isActive()) ++count;
    activeVoiceCount.store(count, std::memory_order_relaxed);
}

Voice* SynthEngine::findFreeVoice() noexcept
{
    for (auto& v : voices)
        if (!v.isActive()) return &v;
    return &voices[0]; // voice steal: grab oldest
}

Voice* SynthEngine::findVoiceForNote(int midiNote) noexcept
{
    for (auto& v : voices)
        if (v.getMidiNote() == midiNote && v.isActive()) return &v;
    return nullptr;
}
