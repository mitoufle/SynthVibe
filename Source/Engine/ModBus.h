#pragma once

namespace SynthVibe
{
    // Modulation bus — accumulates the matrix's contribution per destination.
    // Reset to identity (zero deltas, one multipliers) before each control-rate
    // recompute, then applyToBus() adds slot contributions in.
    struct ModBus
    {
        float cutoffSemitones = 0.f;   // additive offset; +12 = +1 octave
        float resonanceDelta  = 0.f;   // additive in [0..1] domain
        float driveDelta      = 0.f;   // additive in [0..1] domain
        float osc1FineCents   = 0.f;   // additive cents
        float osc2FineCents   = 0.f;
        float osc1LevelMul    = 1.f;   // multiplier
        float osc2LevelMul    = 1.f;
        float osc1PwmDelta    = 0.f;   // additive in [0.01..0.99] domain
        float osc2PwmDelta    = 0.f;
        float masterVolMul    = 1.f;   // multiplier
    };
}
