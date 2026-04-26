#pragma once

// All parameter IDs in one place. IDs are dot-separated per the design contract
// (docs/ClaudeDesign/docs/param-audit.md). Do NOT rename string values once the
// plugin ships — user presets bind by string ID and will lose their values on rename.
namespace ParamIDs
{
    // Oscillator 1
    inline constexpr const char* osc1Waveform  = "osc1.wave";
    inline constexpr const char* osc1Octave    = "osc1.octave";
    inline constexpr const char* osc1Semitone  = "osc1.semi";
    inline constexpr const char* osc1Detune    = "osc1.fine";
    inline constexpr const char* osc1Level     = "osc1.level";
    inline constexpr const char* osc1Phase     = "osc1.phase";
    inline constexpr const char* osc1Pwm       = "osc1.pwm";

    // Oscillator 2
    inline constexpr const char* osc2Waveform  = "osc2.wave";
    inline constexpr const char* osc2Octave    = "osc2.octave";
    inline constexpr const char* osc2Semitone  = "osc2.semi";
    inline constexpr const char* osc2Detune    = "osc2.fine";
    inline constexpr const char* osc2Level     = "osc2.level";
    inline constexpr const char* osc2Phase     = "osc2.phase";
    inline constexpr const char* osc2Pwm       = "osc2.pwm";

    // LFO 1
    inline constexpr const char* lfo1Shape     = "lfo1.wave";
    inline constexpr const char* lfo1Rate      = "lfo1.rate";
    inline constexpr const char* lfo1Depth     = "lfo1.depth";
    inline constexpr const char* lfo1Dest      = "lfo1.dest";

    // LFO 2
    inline constexpr const char* lfo2Shape     = "lfo2.wave";
    inline constexpr const char* lfo2Rate      = "lfo2.rate";
    inline constexpr const char* lfo2Depth     = "lfo2.depth";
    inline constexpr const char* lfo2Dest      = "lfo2.dest";

    // Filter
    inline constexpr const char* filterType      = "filt.type";
    inline constexpr const char* filterCutoff    = "filt.cutoff";
    inline constexpr const char* filterResonance = "filt.res";
    inline constexpr const char* filterEnvAmt    = "filt.envamt";
    inline constexpr const char* filterDrive     = "filt.drive";
    inline constexpr const char* filterKeytrack  = "filt.keytrack";

    // Amp envelope
    inline constexpr const char* ampAttack       = "env.amp.attack";
    inline constexpr const char* ampDecay        = "env.amp.decay";
    inline constexpr const char* ampSustain      = "env.amp.sustain";
    inline constexpr const char* ampRelease      = "env.amp.release";

    // Filter envelope
    inline constexpr const char* fltAttack       = "env.filt.attack";
    inline constexpr const char* fltDecay        = "env.filt.decay";
    inline constexpr const char* fltSustain      = "env.filt.sustain";
    inline constexpr const char* fltRelease      = "env.filt.release";

    // Master
    inline constexpr const char* masterVolume    = "master.vol";

    // Osc1 Unison
    inline constexpr const char* osc1UnisonVoices  = "osc1.uni.voices";
    inline constexpr const char* osc1UnisonDetune  = "osc1.uni.detune";
    inline constexpr const char* osc1UnisonSpread  = "osc1.uni.spread";

    // Osc2 Unison
    inline constexpr const char* osc2UnisonVoices  = "osc2.uni.voices";
    inline constexpr const char* osc2UnisonDetune  = "osc2.uni.detune";
    inline constexpr const char* osc2UnisonSpread  = "osc2.uni.spread";

    // Arp
    inline constexpr const char* arpEnabled      = "arp.on";
    inline constexpr const char* arpMode         = "arp.pattern";
    inline constexpr const char* arpRate         = "arp.rate";
    inline constexpr const char* arpOctaveRange  = "arp.octaves";
    inline constexpr const char* arpGate         = "arp.gate";
    inline constexpr const char* arpSwing        = "arp.swing";
    inline constexpr const char* arpHumanize     = "arp.humanize";
    inline constexpr const char* arpLatch        = "arp.latch";

    // Modulation Matrix — 16 slots declared (UI exposes 1..8; 9..16 reserved for future expansion
    // so presets saved today will not need migration when slot count grows).
    // Suffix is 'dst' (paired with 'src') per docs/ClaudeDesign/docs/param-audit.md; this
    // intentionally diverges from the older lfo*Dest naming.
    inline constexpr const char* mod1Src  = "mod.1.src";
    inline constexpr const char* mod1Dst  = "mod.1.dst";
    inline constexpr const char* mod1Amount = "mod.1.amount";
    inline constexpr const char* mod1Curve  = "mod.1.curve";
    inline constexpr const char* mod2Src  = "mod.2.src";
    inline constexpr const char* mod2Dst  = "mod.2.dst";
    inline constexpr const char* mod2Amount = "mod.2.amount";
    inline constexpr const char* mod2Curve  = "mod.2.curve";
    inline constexpr const char* mod3Src  = "mod.3.src";
    inline constexpr const char* mod3Dst  = "mod.3.dst";
    inline constexpr const char* mod3Amount = "mod.3.amount";
    inline constexpr const char* mod3Curve  = "mod.3.curve";
    inline constexpr const char* mod4Src  = "mod.4.src";
    inline constexpr const char* mod4Dst  = "mod.4.dst";
    inline constexpr const char* mod4Amount = "mod.4.amount";
    inline constexpr const char* mod4Curve  = "mod.4.curve";
    inline constexpr const char* mod5Src  = "mod.5.src";
    inline constexpr const char* mod5Dst  = "mod.5.dst";
    inline constexpr const char* mod5Amount = "mod.5.amount";
    inline constexpr const char* mod5Curve  = "mod.5.curve";
    inline constexpr const char* mod6Src  = "mod.6.src";
    inline constexpr const char* mod6Dst  = "mod.6.dst";
    inline constexpr const char* mod6Amount = "mod.6.amount";
    inline constexpr const char* mod6Curve  = "mod.6.curve";
    inline constexpr const char* mod7Src  = "mod.7.src";
    inline constexpr const char* mod7Dst  = "mod.7.dst";
    inline constexpr const char* mod7Amount = "mod.7.amount";
    inline constexpr const char* mod7Curve  = "mod.7.curve";
    inline constexpr const char* mod8Src  = "mod.8.src";
    inline constexpr const char* mod8Dst  = "mod.8.dst";
    inline constexpr const char* mod8Amount = "mod.8.amount";
    inline constexpr const char* mod8Curve  = "mod.8.curve";
    inline constexpr const char* mod9Src  = "mod.9.src";
    inline constexpr const char* mod9Dst  = "mod.9.dst";
    inline constexpr const char* mod9Amount = "mod.9.amount";
    inline constexpr const char* mod9Curve  = "mod.9.curve";
    inline constexpr const char* mod10Src  = "mod.10.src";
    inline constexpr const char* mod10Dst  = "mod.10.dst";
    inline constexpr const char* mod10Amount = "mod.10.amount";
    inline constexpr const char* mod10Curve  = "mod.10.curve";
    inline constexpr const char* mod11Src  = "mod.11.src";
    inline constexpr const char* mod11Dst  = "mod.11.dst";
    inline constexpr const char* mod11Amount = "mod.11.amount";
    inline constexpr const char* mod11Curve  = "mod.11.curve";
    inline constexpr const char* mod12Src  = "mod.12.src";
    inline constexpr const char* mod12Dst  = "mod.12.dst";
    inline constexpr const char* mod12Amount = "mod.12.amount";
    inline constexpr const char* mod12Curve  = "mod.12.curve";
    inline constexpr const char* mod13Src  = "mod.13.src";
    inline constexpr const char* mod13Dst  = "mod.13.dst";
    inline constexpr const char* mod13Amount = "mod.13.amount";
    inline constexpr const char* mod13Curve  = "mod.13.curve";
    inline constexpr const char* mod14Src  = "mod.14.src";
    inline constexpr const char* mod14Dst  = "mod.14.dst";
    inline constexpr const char* mod14Amount = "mod.14.amount";
    inline constexpr const char* mod14Curve  = "mod.14.curve";
    inline constexpr const char* mod15Src  = "mod.15.src";
    inline constexpr const char* mod15Dst  = "mod.15.dst";
    inline constexpr const char* mod15Amount = "mod.15.amount";
    inline constexpr const char* mod15Curve  = "mod.15.curve";
    inline constexpr const char* mod16Src  = "mod.16.src";
    inline constexpr const char* mod16Dst  = "mod.16.dst";
    inline constexpr const char* mod16Amount = "mod.16.amount";
    inline constexpr const char* mod16Curve  = "mod.16.curve";

    // FX Chain — 10 generic slots, each carrying type+bypass+mix+p1..p4 (= 70 params).
    // The meaning of p1..p4 depends on the slot's `type` choice index. See
    // Source/FX/FxTypeMap.cpp for the per-type p1..p4 labels and DSP mapping.
    inline constexpr const char* fx1Type   = "fx.1.type";
    inline constexpr const char* fx1Bypass = "fx.1.bypass";
    inline constexpr const char* fx1Mix    = "fx.1.mix";
    inline constexpr const char* fx1P1     = "fx.1.p1";
    inline constexpr const char* fx1P2     = "fx.1.p2";
    inline constexpr const char* fx1P3     = "fx.1.p3";
    inline constexpr const char* fx1P4     = "fx.1.p4";

    inline constexpr const char* fx2Type   = "fx.2.type";
    inline constexpr const char* fx2Bypass = "fx.2.bypass";
    inline constexpr const char* fx2Mix    = "fx.2.mix";
    inline constexpr const char* fx2P1     = "fx.2.p1";
    inline constexpr const char* fx2P2     = "fx.2.p2";
    inline constexpr const char* fx2P3     = "fx.2.p3";
    inline constexpr const char* fx2P4     = "fx.2.p4";

    inline constexpr const char* fx3Type   = "fx.3.type";
    inline constexpr const char* fx3Bypass = "fx.3.bypass";
    inline constexpr const char* fx3Mix    = "fx.3.mix";
    inline constexpr const char* fx3P1     = "fx.3.p1";
    inline constexpr const char* fx3P2     = "fx.3.p2";
    inline constexpr const char* fx3P3     = "fx.3.p3";
    inline constexpr const char* fx3P4     = "fx.3.p4";

    inline constexpr const char* fx4Type   = "fx.4.type";
    inline constexpr const char* fx4Bypass = "fx.4.bypass";
    inline constexpr const char* fx4Mix    = "fx.4.mix";
    inline constexpr const char* fx4P1     = "fx.4.p1";
    inline constexpr const char* fx4P2     = "fx.4.p2";
    inline constexpr const char* fx4P3     = "fx.4.p3";
    inline constexpr const char* fx4P4     = "fx.4.p4";

    inline constexpr const char* fx5Type   = "fx.5.type";
    inline constexpr const char* fx5Bypass = "fx.5.bypass";
    inline constexpr const char* fx5Mix    = "fx.5.mix";
    inline constexpr const char* fx5P1     = "fx.5.p1";
    inline constexpr const char* fx5P2     = "fx.5.p2";
    inline constexpr const char* fx5P3     = "fx.5.p3";
    inline constexpr const char* fx5P4     = "fx.5.p4";

    inline constexpr const char* fx6Type   = "fx.6.type";
    inline constexpr const char* fx6Bypass = "fx.6.bypass";
    inline constexpr const char* fx6Mix    = "fx.6.mix";
    inline constexpr const char* fx6P1     = "fx.6.p1";
    inline constexpr const char* fx6P2     = "fx.6.p2";
    inline constexpr const char* fx6P3     = "fx.6.p3";
    inline constexpr const char* fx6P4     = "fx.6.p4";

    inline constexpr const char* fx7Type   = "fx.7.type";
    inline constexpr const char* fx7Bypass = "fx.7.bypass";
    inline constexpr const char* fx7Mix    = "fx.7.mix";
    inline constexpr const char* fx7P1     = "fx.7.p1";
    inline constexpr const char* fx7P2     = "fx.7.p2";
    inline constexpr const char* fx7P3     = "fx.7.p3";
    inline constexpr const char* fx7P4     = "fx.7.p4";

    inline constexpr const char* fx8Type   = "fx.8.type";
    inline constexpr const char* fx8Bypass = "fx.8.bypass";
    inline constexpr const char* fx8Mix    = "fx.8.mix";
    inline constexpr const char* fx8P1     = "fx.8.p1";
    inline constexpr const char* fx8P2     = "fx.8.p2";
    inline constexpr const char* fx8P3     = "fx.8.p3";
    inline constexpr const char* fx8P4     = "fx.8.p4";

    inline constexpr const char* fx9Type   = "fx.9.type";
    inline constexpr const char* fx9Bypass = "fx.9.bypass";
    inline constexpr const char* fx9Mix    = "fx.9.mix";
    inline constexpr const char* fx9P1     = "fx.9.p1";
    inline constexpr const char* fx9P2     = "fx.9.p2";
    inline constexpr const char* fx9P3     = "fx.9.p3";
    inline constexpr const char* fx9P4     = "fx.9.p4";

    inline constexpr const char* fx10Type   = "fx.10.type";
    inline constexpr const char* fx10Bypass = "fx.10.bypass";
    inline constexpr const char* fx10Mix    = "fx.10.mix";
    inline constexpr const char* fx10P1     = "fx.10.p1";
    inline constexpr const char* fx10P2     = "fx.10.p2";
    inline constexpr const char* fx10P3     = "fx.10.p3";
    inline constexpr const char* fx10P4     = "fx.10.p4";
}
