#pragma once

// All parameter IDs in one place — add new ones here when extending the synth.
// The AI layer will use these IDs to map prompts → parameter values.
namespace ParamIDs
{
    // Oscillator
    inline constexpr const char* oscWaveform     = "osc_waveform";
    inline constexpr const char* oscDetune       = "osc_detune";
    inline constexpr const char* oscOctave       = "osc_octave";

    // Filter
    inline constexpr const char* filterType      = "filter_type";
    inline constexpr const char* filterCutoff    = "filter_cutoff";
    inline constexpr const char* filterResonance = "filter_resonance";
    inline constexpr const char* filterEnvAmt    = "filter_env_amt";

    // Amplitude envelope
    inline constexpr const char* ampAttack       = "amp_attack";
    inline constexpr const char* ampDecay        = "amp_decay";
    inline constexpr const char* ampSustain      = "amp_sustain";
    inline constexpr const char* ampRelease      = "amp_release";

    // Filter envelope
    inline constexpr const char* fltAttack       = "flt_attack";
    inline constexpr const char* fltDecay        = "flt_decay";
    inline constexpr const char* fltSustain      = "flt_sustain";
    inline constexpr const char* fltRelease      = "flt_release";

    // Master
    inline constexpr const char* masterVolume    = "master_volume";

    // Delay
    inline constexpr const char* delayTime     = "delay_time";
    inline constexpr const char* delayFeedback = "delay_feedback";
    inline constexpr const char* delayMix      = "delay_mix";

    // --- Future parameters go below this line ---
    // inline constexpr const char* lfoRate      = "lfo_rate";
    // inline constexpr const char* lfoDepth     = "lfo_depth";
    // inline constexpr const char* reverbMix    = "reverb_mix";
}
