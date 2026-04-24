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

    // Delay (interim — Phase 3 migrates these into the generic fx.N slot scheme)
    inline constexpr const char* delayTime       = "fx.delay.time";
    inline constexpr const char* delayFeedback   = "fx.delay.feedback";
    inline constexpr const char* delayMix        = "fx.delay.mix";

    // Chorus
    inline constexpr const char* chorusRate      = "fx.chorus.rate";
    inline constexpr const char* chorusDepth     = "fx.chorus.depth";
    inline constexpr const char* chorusMix       = "fx.chorus.mix";

    // Reverb
    inline constexpr const char* reverbRoom      = "fx.reverb.room";
    inline constexpr const char* reverbDamp      = "fx.reverb.damp";
    inline constexpr const char* reverbMix       = "fx.reverb.mix";

    // Drive
    inline constexpr const char* driveType       = "fx.drive.type";
    inline constexpr const char* driveAmount     = "fx.drive.amount";
    inline constexpr const char* driveMix        = "fx.drive.mix";

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
}
