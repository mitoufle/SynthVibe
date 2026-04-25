#include "FxRunner.h"

namespace SynthVibe
{
    void FxRunner::prepare(double sr, int maxBlock)
    {
        for (auto& s : slots) s.prepare(sr, maxBlock);
    }

    void FxRunner::setSnapshot(const std::array<FxSlotParams, kNumSlots>& snap)
    {
        for (int i = 0; i < kNumSlots; ++i) slots[i].setParams(snap[i]);
    }

    void FxRunner::process(juce::AudioBuffer<float>& buffer)
    {
        for (auto& s : slots) s.process(buffer);
    }

    void FxRunner::reset()
    {
        for (auto& s : slots) s.reset();
    }
}
