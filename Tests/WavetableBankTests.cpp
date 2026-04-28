#include <juce_core/juce_core.h>
#include "Engine/WavetableBank.h"
#include <cmath>

struct WavetableBankTests : public juce::UnitTest
{
    WavetableBankTests() : juce::UnitTest("WavetableBank", "AISynth") {}

    void runTest() override
    {
        WavetableBank bank;

        beginTest("getCycle returns non-null for every table index");
        {
            for (int t = 0; t < WavetableBank::NumTables; ++t)
                expect(bank.getCycle(t) != nullptr,
                       "getCycle(" + juce::String(t) + ") was nullptr");
        }

        beginTest("fetchSample stays bounded in [-1, 1] across the MIDI range");
        {
            const double sr = 48000.0;
            const std::array<double, 5> testFreqs { 27.5, 110.0, 440.0, 2093.0, 8372.0 };
            for (int t = 0; t < WavetableBank::NumTables; ++t)
                for (double f : testFreqs)
                {
                    const double dt = f / sr;
                    double phase = 0.0;
                    for (int i = 0; i < 4096; ++i)
                    {
                        const float s = bank.fetchSample(t, phase, dt);
                        expect(! std::isnan(s), "NaN at table=" + juce::String(t)
                               + " f=" + juce::String(f));
                        expect(std::abs(s) <= 1.001f,
                               "out-of-range sample " + juce::String(s)
                               + " at table=" + juce::String(t) + " f=" + juce::String(f));
                        phase += dt;
                        if (phase >= 1.0) phase -= 1.0;
                    }
                }
        }

        beginTest("mipmap selection drops aliasing-prone harmonics at high notes");
        {
            const double sr = 48000.0;
            // NOTE: With Task 5's stub data (one band-limited sine per mip), this
            // assertion is a structural smoke check that fetchSample doesn't go
            // silent or NaN at high notes — it does NOT prove correct aliasing
            // suppression. The < rmsLow * 2.0 bound becomes a meaningful aliasing
            // guard at Task 15, when harmonic-rich tables (Organ/Vocal/Noise) land.
            auto rms = [&](int tableIdx, double f) {
                const double dt = f / sr;
                double phase = 0.0, sumSq = 0.0;
                const int N = 8192;
                for (int i = 0; i < N; ++i)
                {
                    const float s = bank.fetchSample(tableIdx, phase, dt);
                    sumSq += s * s;
                    phase += dt;
                    if (phase >= 1.0) phase -= 1.0;
                }
                return std::sqrt(sumSq / N);
            };

            for (int t = 0; t < WavetableBank::NumTables; ++t)
            {
                const double rmsLow  = rms(t, 110.0);
                const double rmsHigh = rms(t, 4186.0);
                expect(rmsHigh > 0.05,
                       "table " + juce::String(t) + " mostly silent at high note (rms="
                       + juce::String(rmsHigh) + ")");
                expect(rmsHigh < rmsLow * 2.0,
                       "table " + juce::String(t) + " has more high-note RMS than expected — "
                       + "possible aliasing or mip selection bug");
            }
        }
    }
};

static WavetableBankTests _wavetableBankTests;
