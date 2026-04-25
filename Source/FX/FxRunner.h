#pragma once
#include <array>
#include "FxSlot.h"

namespace SynthVibe
{
    // 10 serial slots driven by per-block snapshots from APVTS.
    class FxRunner
    {
    public:
        static constexpr int kNumSlots = 10;

        void prepare(double sampleRate, int maxBlockSize);
        void setSnapshot(const std::array<FxSlotParams, kNumSlots>& snap);
        void process(juce::AudioBuffer<float>& buffer);
        void reset();

    private:
        std::array<FxSlot, kNumSlots> slots;
    };
}
