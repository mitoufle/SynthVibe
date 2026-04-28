#include <juce_core/juce_core.h>
#include "Engine/Oscillator.h"
#include <cmath>
#include <numeric>
#include <vector>

struct OscillatorTests : public juce::UnitTest
{
    OscillatorTests() : juce::UnitTest("Oscillator", "AISynth") {}

    void runTest() override
    {
        beginTest("starting phase 180 deg on sine yields first sample ~ 0");
        {
            Oscillator o;
            o.setSampleRate(48000.0);
            o.setFrequency(1.0);      // very slow — first sample dominated by phase
            o.setWaveform(Waveform::Sine);
            o.setStartingPhase(180.f);
            o.resetPhaseToStart();
            const float s = o.getNextSample();
            expect(std::abs(s) < 0.01f,
                   "first sample at 180deg starting phase should be near 0; got "
                   + juce::String(s));
        }

        beginTest("square PWM duty cycle matches requested value");
        auto measureDuty = [&](float pulseWidth) {
            Oscillator o;
            o.setSampleRate(48000.0);
            o.setFrequency(100.0);
            o.setWaveform(Waveform::Square);
            o.setPulseWidth(pulseWidth);
            o.reset();
            // Drop first 480 samples (one full cycle) to avoid the phase=0 transient,
            // then measure over 48 full cycles (23040 samples).
            for (int i = 0; i < 480; ++i) o.getNextSample();
            int pos = 0, total = 0;
            for (int i = 0; i < 23040; ++i)
            {
                const float s = o.getNextSample();
                if (s > 0.f) ++pos;
                ++total;
                expect(std::isfinite(s), "sample must be finite");
                expect(std::abs(s) < 2.0f, "sample must stay bounded");
            }
            return (float) pos / (float) total;
        };

        const float d10 = measureDuty(0.10f);
        const float d50 = measureDuty(0.50f);
        const float d90 = measureDuty(0.90f);
        expect(std::abs(d10 - 0.10f) < 0.05f,
               juce::String("duty 0.10 measured as ") + juce::String(d10));
        expect(std::abs(d50 - 0.50f) < 0.05f,
               juce::String("duty 0.50 measured as ") + juce::String(d50));
        expect(std::abs(d90 - 0.90f) < 0.05f,
               juce::String("duty 0.90 measured as ") + juce::String(d90));

        beginTest("Wavetable enum value exists and oscillator returns silence pre-bank");
        {
            Oscillator o;
            o.setSampleRate(48000.0);
            o.setFrequency(440.0);
            o.setWaveform(Waveform::Wavetable);
            o.reset();
            float maxAbs = 0.f;
            for (int i = 0; i < 1024; ++i)
                maxAbs = std::max(maxAbs, std::abs(o.getNextSample()));
            expect(maxAbs == 0.f,
                   "Waveform::Wavetable should return 0.f when no bank is wired; got max |sample| = "
                   + juce::String(maxAbs));
        }
    }
};

static OscillatorTests sOscillatorTests;
