#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "FX/FxRunner.h"

struct FxRunnerTests : public juce::UnitTest
{
    FxRunnerTests() : juce::UnitTest("FxRunner", "AISynth") {}

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

    static float maxAbs(const juce::AudioBuffer<float>& buf)
    {
        float m = 0.f;
        const int n = buf.getNumSamples();
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        {
            const float* d = buf.getReadPointer(ch);
            for (int i = 0; i < n; ++i) m = juce::jmax(m, std::abs(d[i]));
        }
        return m;
    }

    void runTest() override
    {
        const double sr = 48000.0;
        const int blockN = 512;

        beginTest("Empty chain (all None) is exact passthrough");
        {
            SynthVibe::FxRunner runner;
            runner.prepare(sr, blockN);
            std::array<SynthVibe::FxSlotParams, 10> snap{};   // all type=None, mix=1
            runner.setSnapshot(snap);

            juce::AudioBuffer<float> buf(2, blockN);
            fillSine(buf, 1000.f, sr);
            const float before = maxAbs(buf);
            runner.process(buf);
            const float after = maxAbs(buf);
            expectWithinAbsoluteError(after, before, 0.0001f);
        }

        beginTest("All-bypassed chain is passthrough even with non-None types");
        {
            SynthVibe::FxRunner runner;
            runner.prepare(sr, blockN);
            std::array<SynthVibe::FxSlotParams, 10> snap{};
            for (auto& s : snap)
            {
                s.type   = SynthVibe::FxType::Drive;
                s.bypass = true;
                s.p1     = 1.f;   // would be Fold @ max amount otherwise
                s.p2     = 1.f;
            }
            runner.setSnapshot(snap);

            juce::AudioBuffer<float> buf(2, blockN);
            fillSine(buf, 1000.f, sr);
            const float before = maxAbs(buf);
            runner.process(buf);
            const float after = maxAbs(buf);
            expectWithinAbsoluteError(after, before, 0.0001f);
        }

        beginTest("Stub types (Phaser/Flanger/Bitcrush/Filter/Comp) are passthrough");
        {
            SynthVibe::FxRunner runner;
            runner.prepare(sr, blockN);
            std::array<SynthVibe::FxSlotParams, 10> snap{};
            const SynthVibe::FxType stubs[] = {
                SynthVibe::FxType::Comp,    SynthVibe::FxType::Phaser,
                SynthVibe::FxType::Flanger, SynthVibe::FxType::Bitcrush,
                SynthVibe::FxType::Filter
            };
            for (size_t i = 0; i < std::size(stubs); ++i)
                snap[i].type = stubs[i];
            runner.setSnapshot(snap);

            juce::AudioBuffer<float> buf(2, blockN);
            fillSine(buf, 1000.f, sr);
            const float before = maxAbs(buf);
            runner.process(buf);
            const float after = maxAbs(buf);
            expectWithinAbsoluteError(after, before, 0.0001f);
        }
    }
};

static FxRunnerTests sFxRunnerTests;
