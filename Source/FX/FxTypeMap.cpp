#include "FxTypeMap.h"
#include <juce_core/juce_core.h>

namespace SynthVibe
{
    const std::array<const char*, kFxTypeCount> kFxTypeNames = {{
        "None", "Drive", "Chorus", "Delay", "Reverb",
        "EQ3", "Comp", "Phaser", "Flanger", "Bitcrush", "Filter"
    }};

    const std::array<std::array<const char*, 4>, kFxTypeCount> kFxParamLabels = {{
        /* None     */ { "", "", "", "" },
        /* Drive    */ { "Type", "Amount", "", "" },
        /* Chorus   */ { "Rate", "Depth", "", "" },
        /* Delay    */ { "Time", "Fbk",   "", "" },
        /* Reverb   */ { "Size", "Damp",  "", "" },
        /* EQ3      */ { "Low",  "Mid",   "High", "Mid Hz" },
        /* Comp     */ { "Thr",  "Ratio", "Atk",  "Rel" },     // stub
        /* Phaser   */ { "Rate", "Depth", "Fbk",  "" },        // stub
        /* Flanger  */ { "Rate", "Depth", "Fbk",  "" },        // stub
        /* Bitcrush */ { "Bits", "Rate",  "",     "" },        // stub
        /* Filter   */ { "Cutoff","Reso", "Type", "" }         // stub
    }};

    Drive::Params toDriveParams(const FxSlotParams& s) noexcept
    {
        Drive::Params p;
        // p1 in [0..1] → 3 type slots {Soft=0, Hard=1, Fold=2}
        p.type    = static_cast<Drive::Type>(juce::jlimit(0, 2,
                        (int) std::round(s.p1 * 2.f)));
        p.driveDb = s.p2 * 24.f;       // [0..1] → [0..24] dB
        p.mix     = s.mix;             // wet/dry handled by Drive itself
        return p;
    }

    Chorus::Params toChorusParams(const FxSlotParams& s) noexcept
    {
        Chorus::Params p;
        p.rate  = 0.1f + s.p1 * (5.0f  - 0.1f);          // [0.1..5] Hz
        p.depth = 0.001f + s.p2 * (0.015f - 0.001f);     // [0.001..0.015] s
        p.mix   = s.mix;
        return p;
    }

    Delay::Params toDelayParams(const FxSlotParams& s) noexcept
    {
        Delay::Params p;
        p.timeMs   = 1.f + s.p1 * (2000.f - 1.f);        // [1..2000] ms
        p.feedback = s.p2 * 0.95f;                        // [0..0.95]
        p.mix      = s.mix;
        return p;
    }

    Reverb::Params toReverbParams(const FxSlotParams& s) noexcept
    {
        Reverb::Params p;
        p.room = s.p1;                                    // [0..1]
        p.damp = s.p2;                                    // [0..1]
        p.mix  = s.mix;
        return p;
    }

    Eq3::Params toEq3Params(const FxSlotParams& s) noexcept
    {
        Eq3::Params p;
        p.lowGainDb  = (s.p1 - 0.5f) * 24.f;              // [-12..+12] dB
        p.midGainDb  = (s.p2 - 0.5f) * 24.f;
        p.highGainDb = (s.p3 - 0.5f) * 24.f;
        p.midFreq    = 200.f + s.p4 * (5000.f - 200.f);   // [200..5000] Hz
        return p;
    }
}
