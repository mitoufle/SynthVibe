#pragma once
#include "FxParams.h"
#include "Drive.h"
#include "Chorus.h"
#include "Delay.h"
#include "Reverb.h"
#include "Eq3.h"

namespace SynthVibe
{
    // Owns one instance of every concrete effect class so type changes never
    // allocate on the audio thread. Memory footprint: ~700 KB at 44.1 kHz
    // (the Delay buffer dominates), × 10 slots in the runner.
    class FxSlot
    {
    public:
        void prepare(double sampleRate, int maxBlockSize);
        void setParams(const FxSlotParams& p);
        void process(juce::AudioBuffer<float>& buffer);
        void reset();

    private:
        FxType   activeType = FxType::None;
        bool     bypass     = false;

        Drive  drive;
        Chorus chorus;
        Delay  delay;
        Reverb reverb;
        Eq3    eq3;
    };
}
