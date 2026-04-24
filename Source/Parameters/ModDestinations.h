#pragma once
#include <array>

namespace SynthVibe
{
    struct ModDestination
    {
        const char* paramId;  // empty string = "none" sentinel (slot 0)
        const char* label;    // human-readable name shown in the UI
    };

    // Curated 13-entry destination table (index 0 = None, 1..12 = targets).
    // Index is persisted in the APVTS choice param; adding new destinations
    // MUST only append to the end so existing presets keep resolving correctly.
    extern const std::array<ModDestination, 13> kDestinations;

    // 10-entry source table — shared by ParameterLayout (DAW automation labels)
    // and ModSourcePicker (UI dropdown labels). Same append-only rule as kDestinations.
    extern const std::array<const char*, 10> kModSources;
}
