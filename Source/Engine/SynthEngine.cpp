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

bool SynthEngine::hasActiveNote(int midiNote) const noexcept
{
    for (const auto& v : voices)
        if (v.getMidiNote() == midiNote && v.isActive()) return true;
    return false;
}

void SynthEngine::handleMidiMessage(const juce::MidiMessage& msg)
{
    if (msg.isNoteOn())
    {
        Voice* v = findVoiceForNote(msg.getNoteNumber());
        bool stolen = false;
        if (!v)
            v = findFreeVoice(&stolen);
        if (v)
        {
            v->setNoteOnOrder(++voiceOrderCounter);
            v->noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), stolen);
        }
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
        float sumL = 0.f, sumR = 0.f;
        for (auto& v : voices)
        {
            if (v.isActive())
            {
                auto [l, r] = v.getNextSample();
                sumL += l;
                sumR += r;
            }
        }

        buffer.getWritePointer(0)[i] += sumL * gain;
        buffer.getWritePointer(1)[i] += sumR * gain;
    }

    // Update active voice count for the UI (message-thread read via atomic)
    int count = 0;
    for (const auto& v : voices)
        if (v.isActive()) ++count;
    activeVoiceCount.store(count, std::memory_order_relaxed);
}

Voice* SynthEngine::findFreeVoice(bool* stolen) noexcept
{
    for (auto& v : voices)
        if (!v.isActive()) { if (stolen) *stolen = false; return &v; }

    // All voices busy — steal the one that has been playing longest.
    Voice* oldest = &voices[0];
    for (auto& v : voices)
        if (v.getNoteOnOrder() < oldest->getNoteOnOrder()) oldest = &v;

    if (stolen) *stolen = true;
    return oldest;
}

Voice* SynthEngine::findVoiceForNote(int midiNote) noexcept
{
    for (auto& v : voices)
        if (v.getMidiNote() == midiNote && v.isActive()) return &v;
    return nullptr;
}
