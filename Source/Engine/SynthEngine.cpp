#include "SynthEngine.h"

void SynthEngine::prepare(const juce::dsp::ProcessSpec& spec)
{
    for (auto& v : voices)
        v.prepare(spec);
    for (auto& v : voices)
        v.setBankPointer(&wavetableBank);
    smoothPolyGain.reset(spec.sampleRate, 0.025);
    smoothPolyGain.setCurrentAndTargetValue(1.f);
}

void SynthEngine::setParams(const VoiceParams& p)
{
    currentParams = p;
    for (auto& v : voices)
        v.setParams(p);
}

void SynthEngine::setMatrixSnapshot(const SynthVibe::ModEngine::Snapshot& snapshot) noexcept
{
    currentMatrix = snapshot;
    for (auto& v : voices)
        v.setMatrixSnapshot(&currentMatrix);
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
    // Compensation only counts voices the user is actively holding. Release
    // tails are still mixed below (via isActive()), but they no longer pull
    // the per-voice gain down — that's what made held notes appear to swell
    // as old tails decayed. The smoother turns sustain→release transitions
    // into an inaudible 25 ms ramp.
    int sustainCount = 0;
    for (const auto& v : voices) if (v.isSustaining()) ++sustainCount;
    const float targetGain = sustainCount > 0 ? 1.f / std::sqrt(static_cast<float>(sustainCount)) : 1.f;
    smoothPolyGain.setTargetValue(targetGain);

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

        const float g = smoothPolyGain.getNextValue();
        buffer.getWritePointer(0)[i] += sumL * g;
        buffer.getWritePointer(1)[i] += sumR * g;
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
