#include <juce_core/juce_core.h>
#include "Engine/Envelope.h"
#include <cmath>

struct ExponentialEnvelopeTests : public juce::UnitTest
{
    ExponentialEnvelopeTests() : juce::UnitTest("ExponentialEnvelope", "AISynth") {}

    void runTest() override
    {
        const double sr = 48000.0;

        beginTest("decay converges exponentially toward sustain");
        {
            Envelope env;
            env.setSampleRate(sr);
            Envelope::Params p;
            p.attack  = 0.001f;
            p.decay   = 0.050f;   // tau = 50 ms
            p.sustain = 0.5f;
            p.release = 0.1f;
            env.setParams(p);
            env.noteOn();

            // Skip the 48-sample attack then sample 4 tau into decay.
            const int skip = static_cast<int>(0.001 * sr) + 2;
            for (int i = 0; i < skip; ++i) env.getNextSample();

            const int stepsFourTau = static_cast<int>(4.0 * p.decay * sr);
            for (int i = 0; i < stepsFourTau; ++i) env.getNextSample();

            const float level = env.getNextSample();
            // After 4 tau, (1 - sustain) * exp(-4) ≈ 0.5 * 0.0183 ≈ 0.00916.
            const float expected = p.sustain + (1.f - p.sustain) * std::exp(-4.f);
            expect(std::abs(level - expected) < 0.01f,
                   juce::String("decay level ") + juce::String(level)
                 + " differs from expected " + juce::String(expected));
        }

        beginTest("release decays exponentially from noteOff level");
        {
            Envelope env;
            env.setSampleRate(sr);
            Envelope::Params p;
            p.attack  = 0.001f;
            p.decay   = 0.001f;
            p.sustain = 1.0f;
            p.release = 0.100f;   // tau = 100 ms
            env.setParams(p);
            env.noteOn();

            // Reach sustain (=1) within a few ms.
            for (int i = 0; i < 1000; ++i) env.getNextSample();

            env.noteOff();
            const int stepsOneTau = static_cast<int>(p.release * sr);
            for (int i = 0; i < stepsOneTau; ++i) env.getNextSample();

            const float level = env.getNextSample();
            // After 1 tau, level should be ~ 1.0 * exp(-1) ≈ 0.368.
            expect(std::abs(level - std::exp(-1.f)) < 0.02f,
                   juce::String("release at 1 tau was ") + juce::String(level));
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
