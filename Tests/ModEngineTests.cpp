#include <juce_core/juce_core.h>
#include "Engine/ModBus.h"

struct ModEngineTests : public juce::UnitTest
{
    ModEngineTests() : juce::UnitTest("ModEngine", "AISynth") {}

    void runTest() override
    {
        beginTest("ModBus default-constructs to identity");
        SynthVibe::ModBus bus;
        expectWithinAbsoluteError(bus.cutoffSemitones, 0.f, 1e-6f);
        expectWithinAbsoluteError(bus.resonanceDelta,  0.f, 1e-6f);
        expectWithinAbsoluteError(bus.driveDelta,      0.f, 1e-6f);
        expectWithinAbsoluteError(bus.osc1FineCents,   0.f, 1e-6f);
        expectWithinAbsoluteError(bus.osc2FineCents,   0.f, 1e-6f);
        expectWithinAbsoluteError(bus.osc1LevelMul,    1.f, 1e-6f);
        expectWithinAbsoluteError(bus.osc2LevelMul,    1.f, 1e-6f);
        expectWithinAbsoluteError(bus.osc1PwmDelta,    0.f, 1e-6f);
        expectWithinAbsoluteError(bus.osc2PwmDelta,    0.f, 1e-6f);
        expectWithinAbsoluteError(bus.masterVolMul,    1.f, 1e-6f);
    }
};

static ModEngineTests sModEngineTests;
