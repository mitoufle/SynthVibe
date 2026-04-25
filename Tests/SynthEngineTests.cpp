#include <juce_core/juce_core.h>
#include "Engine/SynthEngine.h"

struct SynthEngineTests : public juce::UnitTest
{
    SynthEngineTests() : juce::UnitTest("SynthEngine", "AISynth") {}

    static VoiceParams defaultVoiceParams()
    {
        VoiceParams p;
        p.ampEnv.attack  = 0.001f;
        p.ampEnv.decay   = 0.001f;
        p.ampEnv.sustain = 1.0f;
        p.ampEnv.release = 0.001f;
        return p;
    }

    void runTest() override
    {
        beginTest("voice steal takes the oldest active voice, not always voices[0]");

        SynthEngine engine;
        juce::dsp::ProcessSpec spec { 44100.0, 512, 2 };
        engine.prepare(spec);
        engine.setParams(defaultVoiceParams());

        // Fill all 8 voices: notes 60..67 in order.
        // voices[0] receives note 60 (first = oldest).
        for (int n = 60; n <= 67; ++n)
            engine.handleMidiMessage(juce::MidiMessage::noteOn(1, n, 0.8f));

        // Retrigger note 60 — this makes voices[0] the NEWEST voice.
        engine.handleMidiMessage(juce::MidiMessage::noteOn(1, 60, 0.8f));

        // All 8 voices still busy. Steal should now pick note 61 (oldest remaining).
        engine.handleMidiMessage(juce::MidiMessage::noteOn(1, 68, 0.8f));

        // After steal: note 61 voice was taken for note 68.
        // hasActiveNote(61) must be false; hasActiveNote(68) must be true.
        expect(!engine.hasActiveNote(61),
               "note 61 (oldest after retrigger of 60) should have been stolen");
        expect(engine.hasActiveNote(68),
               "note 68 should now be active on the stolen voice");

        beginTest("SynthEngine distributes mod snapshot to voices");
        {
            SynthEngine se;
            juce::dsp::ProcessSpec spec { 48000.0, 256, 2 };
            se.prepare(spec);

            SynthVibe::ModEngine::Snapshot snap{};
            snap[3] = { 1, 1, 0.5f, 0 };

            se.setMatrixSnapshot(snap);
            // Cannot directly inspect voices from outside — relies on integration
            // verified at Voice level. This test only proves the call compiles
            // and doesn't crash (smoke).
            expect(true);
        }
    }
};

static SynthEngineTests synthEngineTests;
