#pragma once

// All parameter IDs in one place — add new ones here when extending the synth.
// The AI layer will use these IDs to map prompts → parameter values.
namespace ParamIDs
{
    // Oscillator 1  (replaces the old osc_* block)
    inline constexpr const char* osc1Waveform  = "osc1_waveform";
    inline constexpr const char* osc1Octave    = "osc1_octave";
    inline constexpr const char* osc1Semitone  = "osc1_semitone";
    inline constexpr const char* osc1Detune    = "osc1_detune";
    inline constexpr const char* osc1Level     = "osc1_level";

    // Oscillator 2
    inline constexpr const char* osc2Waveform  = "osc2_waveform";
    inline constexpr const char* osc2Octave    = "osc2_octave";
    inline constexpr const char* osc2Semitone  = "osc2_semitone";
    inline constexpr const char* osc2Detune    = "osc2_detune";
    inline constexpr const char* osc2Level     = "osc2_level";

    // LFO 1
    inline constexpr const char* lfo1Shape     = "lfo1_shape";
    inline constexpr const char* lfo1Rate      = "lfo1_rate";
    inline constexpr const char* lfo1Depth     = "lfo1_depth";
    inline constexpr const char* lfo1Dest      = "lfo1_dest";

    // LFO 2
    inline constexpr const char* lfo2Shape     = "lfo2_shape";
    inline constexpr const char* lfo2Rate      = "lfo2_rate";
    inline constexpr const char* lfo2Depth     = "lfo2_depth";
    inline constexpr const char* lfo2Dest      = "lfo2_dest";

    // Filter, Envelopes, Master, Delay — unchanged
    inline constexpr const char* filterType      = "filter_type";
    inline constexpr const char* filterCutoff    = "filter_cutoff";
    inline constexpr const char* filterResonance = "filter_resonance";
    inline constexpr const char* filterEnvAmt    = "filter_env_amt";
    inline constexpr const char* ampAttack       = "amp_attack";
    inline constexpr const char* ampDecay        = "amp_decay";
    inline constexpr const char* ampSustain      = "amp_sustain";
    inline constexpr const char* ampRelease      = "amp_release";
    inline constexpr const char* fltAttack       = "flt_attack";
    inline constexpr const char* fltDecay        = "flt_decay";
    inline constexpr const char* fltSustain      = "flt_sustain";
    inline constexpr const char* fltRelease      = "flt_release";
    inline constexpr const char* masterVolume    = "master_volume";
    inline constexpr const char* delayTime       = "delay_time";
    inline constexpr const char* delayFeedback   = "delay_feedback";
    inline constexpr const char* delayMix        = "delay_mix";

    // Chorus
    inline constexpr const char* chorusRate      = "chorus_rate";
    inline constexpr const char* chorusDepth     = "chorus_depth";
    inline constexpr const char* chorusMix       = "chorus_mix";

    // Reverb
    inline constexpr const char* reverbRoom = "reverb_room";
    inline constexpr const char* reverbDamp = "reverb_damp";
    inline constexpr const char* reverbMix  = "reverb_mix";

    // Drive
    inline constexpr const char* driveType   = "drive_type";
    inline constexpr const char* driveAmount = "drive_amount";
    inline constexpr const char* driveMix    = "drive_mix";

    // Unison
    inline constexpr const char* unisonVoices       = "unison_voices";
    inline constexpr const char* unisonDetune       = "unison_detune";
    inline constexpr const char* unisonStereoSpread = "unison_spread";

    // Arp
    inline constexpr const char* arpEnabled      = "arp_enabled";
    inline constexpr const char* arpMode         = "arp_mode";
    inline constexpr const char* arpRate         = "arp_rate";
    inline constexpr const char* arpOctaveRange  = "arp_octave_range";
}
