#include "Eq3.h"
#include <cmath>

void Eq3::prepare(double sr, int /*maxBlockSize*/)
{
    sampleRate = sr;
    reset();
    setParams(params);   // re-design with current params
}

void Eq3::setParams(const Params& p)
{
    params = p;
    designLowShelf (lowShelf,  sampleRate, 250.f,
                    juce::jlimit(-12.f, 12.f, p.lowGainDb));
    designPeak     (peak,      sampleRate, juce::jlimit(20.f, 20000.f, p.midFreq),
                    juce::jlimit(-12.f, 12.f, p.midGainDb));
    designHighShelf(highShelf, sampleRate, 5000.f,
                    juce::jlimit(-12.f, 12.f, p.highGainDb));
}

void Eq3::reset()
{
    lowShelf.reset();
    peak.reset();
    highShelf.reset();
}

void Eq3::process(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
    const int numSamples  = buffer.getNumSamples();
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            float v = data[i];
            v = lowShelf.process (v, ch);
            v = peak.process     (v, ch);
            v = highShelf.process(v, ch);
            data[i] = v;
        }
    }
}

float Eq3::Biquad::process(float x, int ch) noexcept
{
    const float y = b0 * x + b1 * x1[ch] + b2 * x2[ch] - a1 * y1[ch] - a2 * y2[ch];
    x2[ch] = x1[ch]; x1[ch] = x;
    y2[ch] = y1[ch]; y1[ch] = y;
    return y;
}

// RBJ Audio EQ Cookbook — low shelf, slope S = 1 (Butterworth-ish).
void Eq3::designLowShelf(Biquad& b, double sr, float fc, float gainDb) noexcept
{
    const double A     = std::pow(10.0, gainDb / 40.0);
    const double w0    = 2.0 * juce::MathConstants<double>::pi * fc / sr;
    const double cosw0 = std::cos(w0);
    const double sinw0 = std::sin(w0);
    const double S     = 1.0;
    const double alpha = sinw0 / 2.0 * std::sqrt((A + 1.0/A) * (1.0/S - 1.0) + 2.0);
    const double sqrtA = std::sqrt(A);

    const double b0 =        A * ((A + 1) - (A - 1) * cosw0 + 2 * sqrtA * alpha);
    const double b1 =  2.0 * A * ((A - 1) - (A + 1) * cosw0);
    const double b2 =        A * ((A + 1) - (A - 1) * cosw0 - 2 * sqrtA * alpha);
    const double a0 =             (A + 1) + (A - 1) * cosw0 + 2 * sqrtA * alpha;
    const double a1 = -2.0      * ((A - 1) + (A + 1) * cosw0);
    const double a2 =             (A + 1) + (A - 1) * cosw0 - 2 * sqrtA * alpha;

    b.b0 = (float)(b0 / a0);
    b.b1 = (float)(b1 / a0);
    b.b2 = (float)(b2 / a0);
    b.a1 = (float)(a1 / a0);
    b.a2 = (float)(a2 / a0);
}

// RBJ Audio EQ Cookbook — peaking EQ.
void Eq3::designPeak(Biquad& b, double sr, float fc, float gainDb, float Q) noexcept
{
    const double A     = std::pow(10.0, gainDb / 40.0);
    const double w0    = 2.0 * juce::MathConstants<double>::pi * fc / sr;
    const double cosw0 = std::cos(w0);
    const double alpha = std::sin(w0) / (2.0 * Q);

    const double b0 = 1 + alpha * A;
    const double b1 = -2 * cosw0;
    const double b2 = 1 - alpha * A;
    const double a0 = 1 + alpha / A;
    const double a1 = -2 * cosw0;
    const double a2 = 1 - alpha / A;

    b.b0 = (float)(b0 / a0);
    b.b1 = (float)(b1 / a0);
    b.b2 = (float)(b2 / a0);
    b.a1 = (float)(a1 / a0);
    b.a2 = (float)(a2 / a0);
}

// RBJ Audio EQ Cookbook — high shelf, slope S = 1.
void Eq3::designHighShelf(Biquad& b, double sr, float fc, float gainDb) noexcept
{
    const double A     = std::pow(10.0, gainDb / 40.0);
    const double w0    = 2.0 * juce::MathConstants<double>::pi * fc / sr;
    const double cosw0 = std::cos(w0);
    const double sinw0 = std::sin(w0);
    const double S     = 1.0;
    const double alpha = sinw0 / 2.0 * std::sqrt((A + 1.0/A) * (1.0/S - 1.0) + 2.0);
    const double sqrtA = std::sqrt(A);

    const double b0 =        A * ((A + 1) + (A - 1) * cosw0 + 2 * sqrtA * alpha);
    const double b1 = -2.0 * A * ((A - 1) + (A + 1) * cosw0);
    const double b2 =        A * ((A + 1) + (A - 1) * cosw0 - 2 * sqrtA * alpha);
    const double a0 =             (A + 1) - (A - 1) * cosw0 + 2 * sqrtA * alpha;
    const double a1 =  2.0      * ((A - 1) - (A + 1) * cosw0);
    const double a2 =             (A + 1) - (A - 1) * cosw0 - 2 * sqrtA * alpha;

    b.b0 = (float)(b0 / a0);
    b.b1 = (float)(b1 / a0);
    b.b2 = (float)(b2 / a0);
    b.a1 = (float)(a1 / a0);
    b.a2 = (float)(a2 / a0);
}
