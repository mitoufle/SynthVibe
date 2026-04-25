#include "ModEngine.h"
#include <cmath>

namespace SynthVibe::ModEngine
{
    float applyCurve(float v, int curveIdx)
    {
        switch (curveIdx)
        {
            case 0: return v;                                          // lin
            case 1: return std::copysign(v * v, v);                    // exp (signed v^2)
            case 2: return std::copysign(std::sqrt(std::abs(v)), v);   // log (signed sqrt)
            case 3: return std::tanh(v * 2.f);                         // s
            default: return v;
        }
    }

    // Stubs — implemented in later tasks.
    void applyToBus(ModBus&, int, float) {}
    void applyMatrix(const Snapshot&, const SourceValues&, ModBus&) {}
    void readSnapshot(juce::AudioProcessorValueTreeState&, Snapshot&) {}
}
