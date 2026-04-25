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

    void applyToBus(ModBus& bus, int dst, float modVal)
    {
        switch (dst)
        {
            case 1:  bus.cutoffSemitones += modVal * 60.f;   break;  // ±5 octaves at modVal=1
            case 2:  bus.resonanceDelta  += modVal * 0.5f;   break;
            case 3:  bus.driveDelta      += modVal * 0.5f;   break;
            case 4:  bus.osc1FineCents   += modVal * 100.f;  break;  // ±1 semitone at modVal=1
            case 5:  bus.osc2FineCents   += modVal * 100.f;  break;
            case 6:  bus.osc1LevelMul    *= (1.f + modVal);  break;
            case 7:  bus.osc2LevelMul    *= (1.f + modVal);  break;
            case 8:  bus.osc1PwmDelta    += modVal * 0.4f;   break;
            case 9:  bus.osc2PwmDelta    += modVal * 0.4f;   break;
            case 10: bus.masterVolMul    *= (1.f + modVal);  break;
            // 11 = env.amp.attack, 12 = env.amp.release: deferred (envelope timing
            // mid-cycle modulation needs careful state handling; freeze for V1).
            case 11: case 12: break;
            default: break;  // 0 = None, anything else = unknown → no-op
        }
    }

    void applyMatrix(const Snapshot& snap, const SourceValues& srcs, ModBus& bus)
    {
        for (const auto& slot : snap)
        {
            if (slot.src == 0 || slot.amount == 0.f) continue;

            float srcValue = 0.f;
            switch (slot.src)
            {
                case 1: srcValue = srcs.lfo1;     break;
                case 2: srcValue = srcs.lfo2;     break;
                case 3: srcValue = srcs.envAmp;   break;
                case 4: srcValue = srcs.envFilt;  break;
                case 5: srcValue = srcs.velocity; break;
                // 6 = Modwheel, 7 = Aftertouch: deferred V2
                case 8: srcValue = srcs.keytrack; break;
                // 9 = Random: deferred V2
                default: continue;
            }

            const float shaped = applyCurve(srcValue, slot.curve);
            applyToBus(bus, slot.dst, shaped * slot.amount);
        }
    }

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
