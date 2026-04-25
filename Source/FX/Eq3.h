#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

// 3-band EQ: low shelf @ 250 Hz · peak @ midFreq · high shelf @ 5000 Hz.
// Coefficients derived from the RBJ Audio EQ Cookbook. Each band is a
// stereo biquad processed in series (state per channel).
class Eq3
{
public:
    struct Params
    {
        float lowGainDb  = 0.f;     // -12..+12 dB
        float midGainDb  = 0.f;
        float highGainDb = 0.f;
        float midFreq    = 1000.f;  // 200..5000 Hz
    };

    void prepare(double sampleRate, int maxBlockSize);
    void setParams(const Params& p);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    struct Biquad
    {
        // direct-form-1 coefficients
        float b0 = 1.f, b1 = 0.f, b2 = 0.f;
        float a1 = 0.f, a2 = 0.f;     // a0 normalised to 1
        float x1[2] {}, x2[2] {};     // per channel
        float y1[2] {}, y2[2] {};

        void  reset() noexcept { x1[0]=x1[1]=x2[0]=x2[1]=0.f; y1[0]=y1[1]=y2[0]=y2[1]=0.f; }
        float process(float x, int ch) noexcept;
    };

    Biquad lowShelf, peak, highShelf;
    Params params;
    double sampleRate = 44100.0;

    static void designLowShelf (Biquad& b, double sr, float fc, float gainDb) noexcept;
    static void designPeak     (Biquad& b, double sr, float fc, float gainDb,
                                float Q = 0.7f) noexcept;
    static void designHighShelf(Biquad& b, double sr, float fc, float gainDb) noexcept;
};
