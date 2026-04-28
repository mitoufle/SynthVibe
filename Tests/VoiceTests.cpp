#include <juce_core/juce_core.h>
#include <cmath>
#include <vector>
#include "Engine/Voice.h"
#include "Engine/WavetableBank.h"

struct VoiceTests : public juce::UnitTest
{
    VoiceTests() : juce::UnitTest("Voice", "AISynth") {}

    void runTest() override
    {
        beginTest("detune LFO modulates osc2 as well as osc1");

        // Strategy: compare two fresh Voice runs that are identical except for
        // lfo1.depth (1.0 vs 0.0).  osc1.level = 0 so osc1 contributes nothing
        // to the output; only osc2 is audible.
        //
        // With the BUG: hasDetuneLfo is true for run A, but only osc1.setDetuneCents
        // is called — and osc1 is silent.  So both runs produce the same osc2 output.
        // diff ≈ 0  → test FAILS (as required before the fix).
        //
        // With the FIX: run A also calls osc2.setDetuneCents(detuneMod), shifting
        // osc2's pitch sample-by-sample.  The two runs produce different osc2 output.
        // diff > 0  → test PASSES.

        auto collectSamples = [&](float lfoDepth) -> std::vector<float>
        {
            Voice voice;
            juce::dsp::ProcessSpec spec { 44100.0, 512, 1 };
            voice.prepare(spec);

            VoiceParams p;
            p.osc1.level   = 0.f;   // silence osc1
            p.osc2.level   = 1.f;   // osc2 only
            p.osc2.detune  = 0.f;
            p.ampEnv.attack  = 0.001f;
            p.ampEnv.decay   = 0.001f;
            p.ampEnv.sustain = 1.0f;
            p.ampEnv.release = 0.001f;
            p.fltEnv.attack  = 0.001f;
            p.fltEnv.decay   = 0.001f;
            p.fltEnv.sustain = 1.0f;
            p.fltEnv.release = 0.001f;
            p.lfo1.shape  = Waveform::Sine;
            p.lfo1.rate   = 2000.f;   // fast LFO — many cycles in 512 samples
            p.lfo1.depth  = lfoDepth;
            p.lfo1.dest   = LfoDest::Detune;
            voice.setParams(p);
            voice.noteOn(60, 1.0f);

            std::vector<float> buf(512);
            for (auto& s : buf)
                s = voice.getNextSample().first;
            return buf;
        };

        const auto withLfo    = collectSamples(1.0f);
        const auto withoutLfo = collectSamples(0.0f);

        float diff = 0.f;
        for (int i = 0; i < 512; ++i)
            diff += std::abs(withLfo[i] - withoutLfo[i]);

        // With a real detune LFO on osc2, the frequency wobbles every sample →
        // accumulated difference over 512 samples is large (typically > 10).
        // Without modulation (bug), both buffers are identical → diff = 0.
        expect(diff > 0.01f,
               "osc2 output must differ when detune LFO is active vs inactive (diff=" +
               juce::String(diff) + ")");

        beginTest("filter keytrack = 1 shifts effective cutoff by one octave per octave");
        {
            auto energyAt = [&](int midi, float keytrack) {
                Voice voice;
                juce::dsp::ProcessSpec spec { 48000.0, 512, 1 };
                voice.prepare(spec);

                VoiceParams p;
                p.osc1.waveform = Waveform::Saw;
                p.osc1.level = 1.f;
                p.osc2.level = 0.f;
                p.filterType = FilterType::LP12;
                p.filterCutoff = 300.f;
                p.filterResonance = 0.f;
                p.filterKeytrack = keytrack;
                p.ampEnv.attack  = 0.001f;
                p.ampEnv.decay   = 0.001f;
                p.ampEnv.sustain = 1.0f;
                p.ampEnv.release = 0.001f;
                p.fltEnv.attack  = 0.001f;
                p.fltEnv.decay   = 0.001f;
                p.fltEnv.sustain = 1.0f;
                p.fltEnv.release = 0.001f;
                voice.setParams(p);
                voice.noteOn(midi, 1.0f);

                for (int i = 0; i < 1024; ++i) voice.getNextSample();
                double sumSq = 0.0;
                for (int i = 0; i < 2048; ++i)
                {
                    const float s = voice.getNextSample().first;
                    sumSq += double(s) * double(s);
                }
                return std::sqrt(sumSq / 2048.0);
            };

            const double rms_60_kt0 = energyAt(60, 0.f);
            const double rms_72_kt0 = energyAt(72, 0.f);
            const double rms_60_kt1 = energyAt(60, 1.f);
            const double rms_72_kt1 = energyAt(72, 1.f);

            const double ratio_kt0 = rms_72_kt0 / juce::jmax(1e-9, rms_60_kt0);
            const double ratio_kt1 = rms_72_kt1 / juce::jmax(1e-9, rms_60_kt1);

            expect(ratio_kt1 > ratio_kt0 * 1.3,
                   "keytrack=1 should preserve more energy at higher pitch than keytrack=0 ("
                   + juce::String(ratio_kt0, 3) + " vs " + juce::String(ratio_kt1, 3) + ")");
        }

        beginTest("Voice exposes per-voice mod sources");
        {
            Voice v;
            juce::dsp::ProcessSpec spec { 48000.0, 256, 1 };
            v.prepare(spec);
            VoiceParams p;
            p.lfo1.depth = 0.5f;
            p.lfo1.rate  = 5.f;
            v.setParams(p);
            v.noteOn(72, 0.8f);   // C5, velocity 0.8

            // Velocity captured directly from noteOn
            expectWithinAbsoluteError(v.getVelocity(), 0.8f, 1e-4f);
            // Keytrack: note 72 (C5) is +12 semitones above C4 (60) → +12/60 = +0.2
            expectWithinAbsoluteError(v.getKeytrackOctaves(), 0.2f, 1e-3f);
            // Env values pre-getNextSample: ampEnv has been triggered → in attack range
            expect(v.getEnvAmpValue() >= 0.f && v.getEnvAmpValue() <= 1.f);
            expect(v.getEnvFiltValue() >= 0.f && v.getEnvFiltValue() <= 1.f);
        }

        beginTest("LFO1 -> Cutoff via mod matrix oscillates audible cutoff");
        {
            Voice v;
            juce::dsp::ProcessSpec spec { 48000.0, 256, 1 };
            v.prepare(spec);
            VoiceParams p;
            p.filterCutoff = 1000.f;       // base 1 kHz
            p.lfo1.depth   = 1.f;
            p.lfo1.rate    = 10.f;          // 10 Hz LFO
            v.setParams(p);

            // Configure matrix: LFO1 -> Cutoff at amount=1.0 (+/-5 octaves max)
            SynthVibe::ModEngine::Snapshot snap{};
            snap[0] = { 1, 1, 1.0f, 0 };    // src=lfo1, dst=cutoff, amount=1, curve=lin
            v.setMatrixSnapshot(&snap);

            v.noteOn(60, 1.f);

            // Capture effective cutoff over ~0.1 s (5000 samples) — spans more
            // than half an LFO cycle so we see both extremes.
            float minCutoff =  1e9f;
            float maxCutoff = -1e9f;
            for (int i = 0; i < 5000; ++i)
            {
                v.getNextSample();
                const float c = v.getCurrentEffectiveCutoff();
                minCutoff = std::min(minCutoff, c);
                maxCutoff = std::max(maxCutoff, c);
            }

            expect(minCutoff < 500.f,  "min cutoff should drop below 500 Hz");
            expect(maxCutoff > 4000.f, "max cutoff should exceed 4000 Hz");
        }

        beginTest("Voice with Wavetable wave produces audible output when bank is wired");
        {
            WavetableBank bank;
            Voice v;
            juce::dsp::ProcessSpec spec { 48000.0, 256u, 2u };
            v.prepare(spec);
            v.setBankPointer(&bank);

            VoiceParams p;
            p.osc1.waveform   = Waveform::Wavetable;
            p.osc1.tableIdx   = 0;     // Organ
            p.osc1.level      = 1.f;
            p.ampEnv.attack   = 0.001f;
            p.ampEnv.decay    = 0.05f;
            p.ampEnv.sustain  = 0.8f;
            p.ampEnv.release  = 0.05f;
            p.osc2.level = 0.f;   // isolate osc1 — osc2 default Saw at level=1.f would mask wiring bugs
            v.setParams(p);
            v.noteOn(60, 1.f);

            float maxAbs = 0.f;
            for (int i = 0; i < 4096; ++i)
            {
                auto [l, r] = v.getNextSample();
                maxAbs = std::max(maxAbs, std::max(std::abs(l), std::abs(r)));
            }
            expect(maxAbs > 0.01f,
                   "Voice should render Wavetable audibly; max |sample| = "
                   + juce::String(maxAbs));
        }
    }
};

static VoiceTests voiceTests;
