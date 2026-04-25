#include "ModEngine.h"
#include "../Parameters/ParameterIDs.h"
#include <cmath>

namespace
{
    struct SlotIds { const char* src; const char* dst; const char* amount; const char* curve; };
    constexpr SlotIds kSlotIds[8] = {
        { ParamIDs::mod1Src, ParamIDs::mod1Dst, ParamIDs::mod1Amount, ParamIDs::mod1Curve },
        { ParamIDs::mod2Src, ParamIDs::mod2Dst, ParamIDs::mod2Amount, ParamIDs::mod2Curve },
        { ParamIDs::mod3Src, ParamIDs::mod3Dst, ParamIDs::mod3Amount, ParamIDs::mod3Curve },
        { ParamIDs::mod4Src, ParamIDs::mod4Dst, ParamIDs::mod4Amount, ParamIDs::mod4Curve },
        { ParamIDs::mod5Src, ParamIDs::mod5Dst, ParamIDs::mod5Amount, ParamIDs::mod5Curve },
        { ParamIDs::mod6Src, ParamIDs::mod6Dst, ParamIDs::mod6Amount, ParamIDs::mod6Curve },
        { ParamIDs::mod7Src, ParamIDs::mod7Dst, ParamIDs::mod7Amount, ParamIDs::mod7Curve },
        { ParamIDs::mod8Src, ParamIDs::mod8Dst, ParamIDs::mod8Amount, ParamIDs::mod8Curve },
    };
}

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

    void readSnapshot(juce::AudioProcessorValueTreeState& apvts, Snapshot& out)
    {
        for (int i = 0; i < 8; ++i)
        {
            out[i].src    = (int) *apvts.getRawParameterValue(kSlotIds[i].src);
            out[i].dst    = (int) *apvts.getRawParameterValue(kSlotIds[i].dst);
            out[i].amount =       *apvts.getRawParameterValue(kSlotIds[i].amount);
            out[i].curve  = (int) *apvts.getRawParameterValue(kSlotIds[i].curve);
        }
    }
}
