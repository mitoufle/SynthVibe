#pragma once
#include <cmath>
#include <complex>
#include <algorithm>

namespace SynthVibe
{
    namespace detail { constexpr float kPi = 3.14159265358979323846f; }

    // Visualization-only RBJ biquad coefficients. The audio engine uses
    // juce::dsp::StateVariableTPTFilter in Source/Engine/Filter.cpp — this
    // helper exists so FilterResponseView can draw a magnitude curve without
    // tapping the live filter. The RBJ and TPT frequency responses match
    // closely for LP/HP/BP at moderate Q, which is sufficient for a display.
    struct FilterCoefficients
    {
        enum class Type { LowPass, HighPass, BandPass };

        float b0 = 1.f, b1 = 0.f, b2 = 0.f;
        float a1 = 0.f, a2 = 0.f;

        static FilterCoefficients compute(Type type, float cutoffHz, float q, double sampleRate)
        {
            FilterCoefficients c;
            const float fc = std::max(20.f, std::min(cutoffHz, (float) sampleRate * 0.49f));
            const float w0 = 2.f * detail::kPi * fc / (float) sampleRate;
            const float cosW0 = std::cos(w0);
            const float sinW0 = std::sin(w0);
            const float alpha = sinW0 / (2.f * std::max(0.1f, q));
            const float a0 = 1.f + alpha;

            switch (type)
            {
                case Type::LowPass:
                    c.b0 = (1.f - cosW0) * 0.5f / a0;
                    c.b1 = (1.f - cosW0)        / a0;
                    c.b2 = (1.f - cosW0) * 0.5f / a0;
                    break;
                case Type::HighPass:
                    c.b0 =  (1.f + cosW0) * 0.5f / a0;
                    c.b1 = -(1.f + cosW0)        / a0;
                    c.b2 =  (1.f + cosW0) * 0.5f / a0;
                    break;
                case Type::BandPass: // constant 0 dB peak gain form
                    c.b0 =  alpha           / a0;
                    c.b1 =  0.f;
                    c.b2 = -alpha           / a0;
                    break;
            }
            c.a1 = -2.f * cosW0 / a0;
            c.a2 = (1.f - alpha) / a0;
            return c;
        }

        // |H(e^{jω})| at freq Hz.
        float magnitudeAt(float freqHz, double sampleRate) const
        {
            const float w = 2.f * detail::kPi * freqHz / (float) sampleRate;
            using C = std::complex<float>;
            const C z1 = std::polar(1.f, -w);
            const C z2 = z1 * z1;
            const C num = b0 + b1 * z1 + b2 * z2;
            const C den = 1.f + a1 * z1 + a2 * z2;
            return std::abs(num / den);
        }
    };
}
