#pragma once

namespace SynthVibe
{
    // Order MUST match the choice array in ParameterLayout.cpp's fx-slot
    // registration block. Persisted as a choice index — only ever append.
    enum class FxType : int
    {
        None      = 0,
        Drive     = 1,
        Chorus    = 2,
        Delay     = 3,
        Reverb    = 4,
        Eq3       = 5,
        Comp      = 6,    // stub in Phase 4
        Phaser    = 7,    // stub
        Flanger   = 8,    // stub
        Bitcrush  = 9,    // stub
        Filter    = 10,   // stub
    };

    inline constexpr int kFxTypeCount = 11;

    // One slot's worth of state, snapshotted from APVTS once per audio block.
    struct FxSlotParams
    {
        FxType type   = FxType::None;
        bool   bypass = false;
        float  mix    = 1.f;   // 0 = dry, 1 = wet
        float  p1     = 0.5f;
        float  p2     = 0.5f;
        float  p3     = 0.5f;
        float  p4     = 0.5f;
    };
}
