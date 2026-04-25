#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "FX/Eq3.h"
#include <cmath>

struct Eq3Tests : public juce::UnitTest
{
    Eq3Tests() : juce::UnitTest("Eq3", "AISynth") {}

    static void fillSine(juce::AudioBuffer<float>& buf, float freqHz, double sr)
    {
        const int n = buf.getNumSamples();
        for (int i = 0; i < n; ++i)
        {
            const float v = std::sin(2.0f * juce::MathConstants<float>::pi
                                     * freqHz * (float) i / (float) sr);
            for (int ch = 0; ch < buf.getNumChannels(); ++ch)
                buf.setSample(ch, i, v);
        }
    }

    static float rms(const juce::AudioBuffer<float>& buf)
    {
        const int n = buf.getNumSamples();
        double s = 0.0;
        const float* d = buf.getReadPointer(0);
        for (int i = 0; i < n; ++i) s += (double) d[i] * d[i];
        return (float) std::sqrt(s / (double) n);
    }

    void runTest() override
    {
        const double sr = 48000.0;
        const int blocks = 4;     // ramp up settled state
        const int blockN = 1024;

        beginTest("Eq3 with all gains 0 dB approximates passthrough");
        {
            Eq3 eq;
            eq.prepare(sr, blockN);
            Eq3::Params p;
            p.lowGainDb = 0.f;
            p.midGainDb = 0.f;
            p.highGainDb = 0.f;
            p.midFreq = 1000.f;
            eq.setParams(p);

            juce::AudioBuffer<float> buf(2, blockN);
            for (int b = 0; b < blocks; ++b)
            {
                fillSine(buf, 1000.f, sr);
                const float before = rms(buf);
                eq.process(buf);
                const float after = rms(buf);
                if (b == blocks - 1)  // only check last block (filters settled)
                    expectWithinAbsoluteError(after, before, 0.05f);
            }
        }

        beginTest("Eq3 with low +12 dB amplifies a 100 Hz sine");
        {
            Eq3 eq;
            eq.prepare(sr, blockN);
            Eq3::Params p;
            p.lowGainDb = 12.f;
            p.midGainDb = 0.f;
            p.highGainDb = 0.f;
            p.midFreq = 1000.f;
            eq.setParams(p);

            juce::AudioBuffer<float> buf(2, blockN);
            float lastBoost = 0.f;
            for (int b = 0; b < blocks; ++b)
            {
                fillSine(buf, 100.f, sr);
                const float before = rms(buf);
                eq.process(buf);
                const float after = rms(buf);
                lastBoost = after / juce::jmax(before, 1e-6f);
            }
            // +12 dB ≈ 4× linear; allow generous slack because shelf rolloff at 100 Hz
            // doesn't hit the full +12 — we just want to confirm boost direction.
            expect(lastBoost > 1.5f,
                   "+12 dB low shelf should boost 100 Hz sine by >1.5×, got "
                   + juce::String(lastBoost, 3));
        }
    }
};

static Eq3Tests sEq3Tests;
