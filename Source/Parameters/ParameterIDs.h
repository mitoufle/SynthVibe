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
}
