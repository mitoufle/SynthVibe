#include <juce_core/juce_core.h>
#include "Engine/Envelope.h"
#include <cmath>

struct ExponentialEnvelopeTests : public juce::UnitTest
{
    ExponentialEnvelopeTests() : juce::UnitTest("ExponentialEnvelope", "AISynth") {}

    void runTest() override
    {
        const double sr = 48000.0;

        // Engine convention: after `decay`/`release` seconds the envelope has
        // closed ~99.9 % of the gap to its target (−60 dB, i.e. exp(−6.908)).
        beginTest("decay reaches sustain within −60 dB at t = decay");
        {
            Envelope env;
            env.setSampleRate(sr);
            Envelope::Params p;
            p.attack  = 0.001f;
            p.decay   = 0.050f;
            p.sustain = 0.5f;
            p.release = 0.1f;
            env.setParams(p);
            env.noteOn();

            const int skip = static_cast<int>(0.001 * sr) + 2;
            for (int i = 0; i < skip; ++i) env.getNextSample();

            const int stepsOneDecay = static_cast<int>(p.decay * sr);
            for (int i = 0; i < stepsOneDecay; ++i) env.getNextSample();

            const float level = env.getNextSample();
            // Remaining distance to sustain should be under 2 × 10⁻³.
            expect(std::abs(level - p.sustain) < 0.002f,
                   juce::String("decay level ") + juce::String(level)
                 + " should be within −60 dB of sustain " + juce::String(p.sustain));
        }

        beginTest("release reaches −60 dB at t = release");
        {
            Envelope env;
            env.setSampleRate(sr);
            Envelope::Params p;
            p.attack  = 0.001f;
            p.decay   = 0.001f;
            p.sustain = 1.0f;
            p.release = 0.100f;
            env.setParams(p);
            env.noteOn();

            for (int i = 0; i < 1000; ++i) env.getNextSample();

            env.noteOff();
            const int stepsOneRelease = static_cast<int>(p.release * sr);
            for (int i = 0; i < stepsOneRelease; ++i) env.getNextSample();

            const float level = env.getNextSample();
            // At t = release, level ≈ exp(−6.908) ≈ 10⁻³.
            expect(level < 0.002f,
                   juce::String("release at t=release was ") + juce::String(level)
                 + " (expected below −60 dB)");
        }

        beginTest("noteOn during release retriggers smoothly (no snap)");
        {
            Envelope env;
            env.setSampleRate(sr);
            Envelope::Params p;
            p.attack  = 0.005f;
            p.decay   = 0.050f;
            p.sustain = 0.5f;
            p.release = 0.200f;
            env.setParams(p);
            env.noteOn();
            for (int i = 0; i < 5000; ++i) env.getNextSample();
            env.noteOff();
            for (int i = 0; i < 1000; ++i) env.getNextSample();
            const float beforeRetrigger = env.getNextSample();
            env.noteOn();
            const float afterRetrigger = env.getNextSample();
            // The new attack should continue from the release level upward, not snap.
            expect(afterRetrigger >= beforeRetrigger - 0.01f,
                   "retrigger snapped downward");
        }
    }
};

static ExponentialEnvelopeTests sExponentialEnvelopeTests;
