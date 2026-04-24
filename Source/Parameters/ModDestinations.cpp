#include "ModDestinations.h"
#include "ParameterIDs.h"

namespace SynthVibe
{
    // APPEND-ONLY: index = APVTS choice value persisted in user presets.
    // Do NOT insert or reorder entries; only add new entries at the end.
    const std::array<ModDestination, 13> kDestinations = {{
        { "",                        "None"        },
        { ParamIDs::filterCutoff,    "Cutoff"      },
        { ParamIDs::filterResonance, "Resonance"   },
        { ParamIDs::filterDrive,     "Drive"       },
        { ParamIDs::osc1Detune,      "Osc1 Pitch"  },
        { ParamIDs::osc2Detune,      "Osc2 Pitch"  },
        { ParamIDs::osc1Level,       "Osc1 Level"  },
        { ParamIDs::osc2Level,       "Osc2 Level"  },
        { ParamIDs::osc1Pwm,         "Osc1 PWM"    },
        { ParamIDs::osc2Pwm,         "Osc2 PWM"    },
        { ParamIDs::masterVolume,    "Master Vol"  },
        { ParamIDs::ampAttack,       "Amp Attack"  },
        { ParamIDs::ampRelease,      "Amp Release" },
    }};

    // APPEND-ONLY: index = APVTS choice value persisted in user presets.
    const std::array<const char*, 10> kModSources = {{
        "None", "LFO 1", "LFO 2", "Env Amp", "Env Filt",
        "Velocity", "Modwheel", "Aftertouch", "Keytrack", "Random"
    }};
}
