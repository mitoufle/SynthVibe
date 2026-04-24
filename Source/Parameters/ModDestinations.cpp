#include "ModDestinations.h"
#include "ParameterIDs.h"

namespace SynthVibe
{
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
}
