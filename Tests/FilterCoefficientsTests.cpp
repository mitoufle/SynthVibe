#include <juce_core/juce_core.h>
#include "Engine/FilterCoefficients.h"
#include <cmath>

struct FilterCoefficientsTests : public juce::UnitTest
{
    FilterCoefficientsTests() : juce::UnitTest("FilterCoefficients", "AISynth") {}

    void runTest() override
    {
        using SynthVibe::FilterCoefficients;
        using Type = SynthVibe::FilterCoefficients::Type;

        beginTest("lowpass magnitude at cutoff is ~ 1/sqrt(2) for Q=0.707");
        {
            auto c = FilterCoefficients::compute(Type::LowPass, 1000.f, 0.707f, 48000.0);
            const float mag = c.magnitudeAt(1000.f, 48000.0);
            expect(std::abs(mag - 0.707f) < 0.05f,
                   juce::String("LP @ fc magnitude was ") + juce::String(mag));
        }

        beginTest("highpass magnitude near DC is ~ 0");
        {
            auto c = FilterCoefficients::compute(Type::HighPass, 1000.f, 0.707f, 48000.0);
            expect(c.magnitudeAt(20.f, 48000.0) < 0.1f, "HP near DC should attenuate");
        }

        beginTest("bandpass peak near cutoff with high Q");
        {
            auto c = FilterCoefficients::compute(Type::BandPass, 1000.f, 8.f, 48000.0);
            const float magAtFc  = c.magnitudeAt(1000.f, 48000.0);
            const float magLow   = c.magnitudeAt(100.f,  48000.0);
            const float magHigh  = c.magnitudeAt(10000.f, 48000.0);
            expect(magAtFc > magLow && magAtFc > magHigh, "BP should peak near fc");
        }

        beginTest("LP24 rolloff is steeper than LP12 at cutoff + 1 octave");
        {
            using SynthVibe::FilterCoefficients;
            using Type = SynthVibe::FilterCoefficients::Type;
            const float fc = 1000.f;
            auto lp12 = FilterCoefficients::compute(Type::LowPass, fc, 0.707f, 48000.0);
            auto lp24 = FilterCoefficients::compute(Type::LP24,    fc, 0.707f, 48000.0);
            const float mag12 = lp12.magnitudeAt(fc * 2.f, 48000.0);
            const float mag24 = lp24.magnitudeAt(fc * 2.f, 48000.0);
            expect(mag24 < mag12 * 0.6f,
                   "LP24 should attenuate more at fc+1oct than LP12 ("
                   + juce::String(mag12) + " vs " + juce::String(mag24) + ")");
        }

        beginTest("Notch attenuates the cutoff frequency");
        {
            using SynthVibe::FilterCoefficients;
            using Type = SynthVibe::FilterCoefficients::Type;
            auto notch = FilterCoefficients::compute(Type::Notch, 1000.f, 4.f, 48000.0);
            const float magAtFc = notch.magnitudeAt(1000.f, 48000.0);
            const float magFar  = notch.magnitudeAt(5000.f, 48000.0);
            expect(magAtFc < 0.2f,
                   "Notch at fc should attenuate, got " + juce::String(magAtFc));
            expect(magFar > 0.8f,
                   "Notch away from fc should pass, got " + juce::String(magFar));
        }
    }
};

static FilterCoefficientsTests sFilterCoefficientsTests;
