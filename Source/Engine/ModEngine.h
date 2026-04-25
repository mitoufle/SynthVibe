#pragma once
#include <array>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ModBus.h"

namespace SynthVibe::ModEngine
{
    static constexpr int kNumActiveSlots = 8;   // matches ModMatrixTable::kVisibleRows

    struct Slot
    {
        int   src    = 0;   // 0 = None
        int   dst    = 0;   // 0 = None
        float amount = 0.f; // -1..+1
        int   curve  = 0;   // 0=lin 1=exp 2=log 3=s
    };

    using Snapshot = std::array<Slot, kNumActiveSlots>;

    struct SourceValues
    {
        float lfo1     = 0.f;   // -1..+1, raw LFO output (BEFORE depth scaling)
        float lfo2     = 0.f;
        float envAmp   = 0.f;   // 0..1, current amp env value
        float envFilt  = 0.f;   // 0..1, current filter env value
        float velocity = 0.f;   // 0..1, note velocity
        float keytrack = 0.f;   // -1..+1, signed octaves from C4 / 5 (clamped)
    };

    // Pure functions
    float applyCurve(float v, int curveIdx);
    void  applyToBus(ModBus& bus, int dst, float modVal);
    void  applyMatrix(const Snapshot& snap, const SourceValues& srcs, ModBus& bus);

    // Reads 8 slots from the APVTS into snapshot. Cheap per-block call.
    void  readSnapshot(juce::AudioProcessorValueTreeState& apvts, Snapshot& out);
}
