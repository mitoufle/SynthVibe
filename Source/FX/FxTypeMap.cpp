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

    // Musical defaults per type. Index aligns with FxType enum.
    // mix/p1..p4 are normalized [0..1] APVTS values; conversion to engineering
    // units happens through the to{Drive,Chorus,...}Params functions below.
    // Stubs (Comp/Phaser/Flanger/Bitcrush/Filter) get neutral 0.5 placeholders
    // since they don't process; mix=1 is fine because the slot passes through.
    const std::array<FxSlotParams, kFxTypeCount> kFxTypeDefaults = {{
        /* None     */ { FxType::None,     false, 1.f,    0.5f,    0.5f,     0.5f, 0.5f     },
        /* Drive    */ { FxType::Drive,    false, 1.f,    0.f,     0.5f,     0.5f, 0.5f     }, // Soft, +12 dB
        /* Chorus   */ { FxType::Chorus,   false, 0.4f,   0.1837f, 0.2857f,  0.5f, 0.5f     }, // 1 Hz, 5 ms
        /* Delay    */ { FxType::Delay,    false, 0.3f,   0.1246f, 0.3684f,  0.5f, 0.5f     }, // 250 ms, fb 0.35
        /* Reverb   */ { FxType::Reverb,   false, 0.3f,   0.5f,    0.5f,     0.5f, 0.5f     }, // mid room/damp
        /* Eq3      */ { FxType::Eq3,      false, 1.f,    0.5f,    0.5f,     0.5f, 0.1667f  }, // flat, mid 1 kHz
        /* Comp     */ { FxType::Comp,     false, 1.f,    0.5f,    0.5f,     0.5f, 0.5f     }, // stub
        /* Phaser   */ { FxType::Phaser,   false, 0.5f,   0.5f,    0.5f,     0.5f, 0.5f     }, // stub
        /* Flanger  */ { FxType::Flanger,  false, 0.5f,   0.5f,    0.5f,     0.5f, 0.5f     }, // stub
        /* Bitcrush */ { FxType::Bitcrush, false, 1.f,    0.5f,    0.5f,     0.5f, 0.5f     }, // stub
        /* Filter   */ { FxType::Filter,   false, 1.f,    0.5f,    0.5f,     0.5f, 0.5f     }  // stub
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
        p.depth = 0.001f + s.p2 * (0.005f - 0.001f);     // [0.001..0.005] s; beyond 5 ms = wobble
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
