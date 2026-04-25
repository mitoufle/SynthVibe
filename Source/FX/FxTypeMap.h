#pragma once
#include <array>
#include "FxParams.h"
#include "Drive.h"
#include "Chorus.h"
#include "Delay.h"
#include "Reverb.h"
#include "Eq3.h"

namespace SynthVibe
{
    // Display name shown in the FxSlot type-picker dropdown.
    // Index = (int) FxType.  Keep aligned with ParameterLayout's typeLabels.
    extern const std::array<const char*, kFxTypeCount> kFxTypeNames;

    // Per-type knob labels for p1..p4. "" means "unused" — UI hides the knob.
    // [type][param 0..3]
    extern const std::array<std::array<const char*, 4>, kFxTypeCount> kFxParamLabels;

    // p1..p4 are all in [0..1]. These functions remap them onto the existing
    // effect-class Params struct.  Slot.mix is consumed separately (it's the
    // wet/dry blend already built into each effect class).
    Drive::Params  toDriveParams (const FxSlotParams& s) noexcept;
    Chorus::Params toChorusParams(const FxSlotParams& s) noexcept;
    Delay::Params  toDelayParams (const FxSlotParams& s) noexcept;
    Reverb::Params toReverbParams(const FxSlotParams& s) noexcept;
    Eq3::Params    toEq3Params   (const FxSlotParams& s) noexcept;
}
