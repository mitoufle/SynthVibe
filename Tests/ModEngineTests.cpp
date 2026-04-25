#include <juce_core/juce_core.h>
#include <cmath>
#include "Engine/ModBus.h"
#include "Engine/ModEngine.h"

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

        beginTest("applyCurve preserves identity for lin");
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve( 0.5f, 0), 0.5f, 1e-5f);
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve(-0.5f, 0),-0.5f, 1e-5f);
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve( 0.0f, 0), 0.0f, 1e-5f);

        beginTest("applyCurve exp is signed power-2");
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve( 0.5f, 1), 0.25f, 1e-5f);
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve(-0.5f, 1),-0.25f, 1e-5f);
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve( 1.0f, 1), 1.0f,  1e-5f);

        beginTest("applyCurve log is signed power-0.5");
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve( 0.25f, 2), 0.5f, 1e-5f);
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve(-0.25f, 2),-0.5f, 1e-5f);

        beginTest("applyCurve s is tanh-shaped");
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve( 0.f, 3), 0.f, 1e-5f);
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve( 1.f, 3), std::tanh(2.f), 1e-5f);
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve(-1.f, 3),-std::tanh(2.f), 1e-5f);

        beginTest("applyCurve unknown index falls back to lin");
        expectWithinAbsoluteError(SynthVibe::ModEngine::applyCurve(0.7f, 99), 0.7f, 1e-5f);
    }
};

static ModEngineTests sModEngineTests;
