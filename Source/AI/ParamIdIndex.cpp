#include "ParamIdIndex.h"

namespace ParamIdIndex
{
    static const std::vector<ParamEntry>& makeEntries()
    {
        // Hand-maintained mirror of ParameterLayout.cpp. Drift is caught by
        // Tests/ParamIdIndexDriftTests.cpp at CI time.
        static const std::vector<ParamEntry> e = {
            // -----------------------------------------------------------------------
            // Oscillator 1
            // -----------------------------------------------------------------------
            { "osc1.wave",   "Osc1 Waveform", ParamEntry::Type::Choice, 0.0, 4.0, 1.0,
              { "Sine", "Saw", "Square", "Triangle", "Wavetable" } },
            { "osc1.octave", "Osc1 Octave",   ParamEntry::Type::Int,    -2.0, 2.0, 0.0, {} },
            { "osc1.semi",   "Osc1 Semitone", ParamEntry::Type::Int,    -12.0, 12.0, 0.0, {} },
            { "osc1.fine",   "Osc1 Detune",   ParamEntry::Type::Float,  -10.0, 10.0, 0.0, {} },
            { "osc1.level",  "Osc1 Level",    ParamEntry::Type::Float,  0.0, 1.0, 1.0, {} },
            { "osc1.phase",  "Osc1 Phase",    ParamEntry::Type::Float,  0.0, 360.0, 0.0, {} },
            { "osc1.pwm",    "Osc1 PWM",      ParamEntry::Type::Float,  0.01, 0.99, 0.5, {} },
            { "osc1.table",  "Osc1 Table",    ParamEntry::Type::Choice, 0.0, 4.0, 0.0,
              { "Organ", "EP", "Bell", "Vocal", "Noise" } },

            // -----------------------------------------------------------------------
            // Oscillator 2
            // -----------------------------------------------------------------------
            { "osc2.wave",   "Osc2 Waveform", ParamEntry::Type::Choice, 0.0, 4.0, 1.0,
              { "Sine", "Saw", "Square", "Triangle", "Wavetable" } },
            { "osc2.octave", "Osc2 Octave",   ParamEntry::Type::Int,    -2.0, 2.0, 0.0, {} },
            { "osc2.semi",   "Osc2 Semitone", ParamEntry::Type::Int,    -12.0, 12.0, 0.0, {} },
            { "osc2.fine",   "Osc2 Detune",   ParamEntry::Type::Float,  -10.0, 10.0, 0.0, {} },
            { "osc2.level",  "Osc2 Level",    ParamEntry::Type::Float,  0.0, 1.0, 0.0, {} },
            { "osc2.phase",  "Osc2 Phase",    ParamEntry::Type::Float,  0.0, 360.0, 0.0, {} },
            { "osc2.pwm",    "Osc2 PWM",      ParamEntry::Type::Float,  0.01, 0.99, 0.5, {} },
            { "osc2.table",  "Osc2 Table",    ParamEntry::Type::Choice, 0.0, 4.0, 0.0,
              { "Organ", "EP", "Bell", "Vocal", "Noise" } },

            // -----------------------------------------------------------------------
            // LFO 1
            // -----------------------------------------------------------------------
            { "lfo1.wave",  "LFO1 Shape", ParamEntry::Type::Choice, 0.0, 3.0, 0.0,
              { "Sine", "Saw", "Square", "Triangle" } },
            { "lfo1.rate",  "LFO1 Rate",  ParamEntry::Type::Float,  0.01, 20.0, 1.0, {} },
            { "lfo1.depth", "LFO1 Depth", ParamEntry::Type::Float,  0.0, 1.0, 0.0, {} },
            { "lfo1.dest",  "LFO1 Dest",  ParamEntry::Type::Choice, 0.0, 3.0, 0.0,
              { "Pitch", "Filter", "Amp", "Detune" } },

            // -----------------------------------------------------------------------
            // LFO 2
            // -----------------------------------------------------------------------
            { "lfo2.wave",  "LFO2 Shape", ParamEntry::Type::Choice, 0.0, 3.0, 0.0,
              { "Sine", "Saw", "Square", "Triangle" } },
            { "lfo2.rate",  "LFO2 Rate",  ParamEntry::Type::Float,  0.01, 20.0, 1.0, {} },
            { "lfo2.depth", "LFO2 Depth", ParamEntry::Type::Float,  0.0, 1.0, 0.0, {} },
            { "lfo2.dest",  "LFO2 Dest",  ParamEntry::Type::Choice, 0.0, 3.0, 0.0,
              { "Pitch", "Filter", "Amp", "Detune" } },

            // -----------------------------------------------------------------------
            // Filter
            // -----------------------------------------------------------------------
            { "filt.type",    "Filter Type",     ParamEntry::Type::Choice, 0.0, 4.0, 0.0,
              { "LP12", "LP24", "HP", "BP", "Notch" } },
            { "filt.cutoff",  "Cutoff",          ParamEntry::Type::Float,  20.0, 20000.0, 8000.0, {} },
            { "filt.res",     "Resonance",       ParamEntry::Type::Float,  0.0, 1.0, 0.1, {} },
            { "filt.envamt",  "Filter Env",      ParamEntry::Type::Float,  -1.0, 1.0, 0.0, {} },
            { "filt.drive",   "Filter Drive",    ParamEntry::Type::Float,  0.0, 1.0, 0.0, {} },
            { "filt.keytrack","Filter Keytrack", ParamEntry::Type::Float,  -1.0, 1.0, 0.0, {} },

            // -----------------------------------------------------------------------
            // Amplitude envelope
            // -----------------------------------------------------------------------
            { "env.amp.attack",  "Amp Attack",  ParamEntry::Type::Float, 0.001, 10.0, 0.01, {} },
            { "env.amp.decay",   "Amp Decay",   ParamEntry::Type::Float, 0.001, 10.0, 0.1,  {} },
            { "env.amp.sustain", "Amp Sustain", ParamEntry::Type::Float, 0.0,   1.0,  0.7,  {} },
            { "env.amp.release", "Amp Release", ParamEntry::Type::Float, 0.001, 10.0, 0.2,  {} },

            // -----------------------------------------------------------------------
            // Filter envelope
            // -----------------------------------------------------------------------
            { "env.filt.attack",  "Flt Attack",  ParamEntry::Type::Float, 0.001, 10.0, 0.01, {} },
            { "env.filt.decay",   "Flt Decay",   ParamEntry::Type::Float, 0.001, 10.0, 0.2,  {} },
            { "env.filt.sustain", "Flt Sustain", ParamEntry::Type::Float, 0.0,   1.0,  0.5,  {} },
            { "env.filt.release", "Flt Release", ParamEntry::Type::Float, 0.001, 10.0, 0.3,  {} },

            // -----------------------------------------------------------------------
            // Master
            // -----------------------------------------------------------------------
            { "master.vol", "Master Volume", ParamEntry::Type::Float, 0.0, 1.0, 0.8, {} },

            // -----------------------------------------------------------------------
            // Osc1 Unison
            // -----------------------------------------------------------------------
            { "osc1.uni.voices", "Osc1 Unison Voices", ParamEntry::Type::Int,   1.0, 7.0,   1.0, {} },
            { "osc1.uni.detune", "Osc1 Unison Detune", ParamEntry::Type::Float, 0.0, 100.0, 0.0, {} },
            { "osc1.uni.spread", "Osc1 Unison Spread", ParamEntry::Type::Float, 0.0, 1.0,   0.5, {} },

            // -----------------------------------------------------------------------
            // Osc2 Unison
            // -----------------------------------------------------------------------
            { "osc2.uni.voices", "Osc2 Unison Voices", ParamEntry::Type::Int,   1.0, 7.0,   1.0, {} },
            { "osc2.uni.detune", "Osc2 Unison Detune", ParamEntry::Type::Float, 0.0, 100.0, 0.0, {} },
            { "osc2.uni.spread", "Osc2 Unison Spread", ParamEntry::Type::Float, 0.0, 1.0,   0.5, {} },

            // -----------------------------------------------------------------------
            // Arp
            // -----------------------------------------------------------------------
            { "arp.on",       "Arp On",        ParamEntry::Type::Bool,   0.0, 1.0, 0.0, {} },
            { "arp.pattern",  "Arp Mode",      ParamEntry::Type::Choice, 0.0, 6.0, 2.0,
              { "up", "down", "updn", "dnup", "rand", "asplayed", "chord" } },
            { "arp.rate",     "Arp Rate",      ParamEntry::Type::Choice, 0.0, 4.0, 2.0,
              { "1/4", "1/8", "1/16", "1/16T", "1/32" } },
            { "arp.octaves",  "Arp Octaves",   ParamEntry::Type::Int,    1.0, 4.0,  2.0, {} },
            { "arp.gate",     "Arp Gate",      ParamEntry::Type::Float,  0.05, 1.0, 0.58, {} },
            { "arp.swing",    "Arp Swing",     ParamEntry::Type::Float,  0.0, 1.0,  0.22, {} },
            { "arp.humanize", "Arp Humanize",  ParamEntry::Type::Float,  0.0, 1.0,  0.4,  {} },
            { "arp.latch",    "Arp Latch",     ParamEntry::Type::Bool,   0.0, 1.0,  0.0,  {} },

            // -----------------------------------------------------------------------
            // Modulation Matrix — 16 slots
            // mod.N.src  : 10 sources (None, LFO 1, ..., Random)
            // mod.N.dst  : 13 destinations (None, Cutoff, ..., Amp Release)
            // mod.N.amount: -1..1
            // mod.N.curve: lin/exp/log/s
            // -----------------------------------------------------------------------
            // Slot 1
            { "mod.1.src",    "Mod 1 Src",    ParamEntry::Type::Choice, 0.0, 9.0,  0.0,
              { "None","LFO 1","LFO 2","Env Amp","Env Filt","Velocity","Modwheel","Aftertouch","Keytrack","Random" } },
            { "mod.1.dst",    "Mod 1 Dst",    ParamEntry::Type::Choice, 0.0, 12.0, 0.0,
              { "None","Cutoff","Resonance","Drive","Osc1 Pitch","Osc2 Pitch","Osc1 Level","Osc2 Level","Osc1 PWM","Osc2 PWM","Master Vol","Amp Attack","Amp Release" } },
            { "mod.1.amount", "Mod 1 Amount", ParamEntry::Type::Float,  -1.0, 1.0, 0.0, {} },
            { "mod.1.curve",  "Mod 1 Curve",  ParamEntry::Type::Choice, 0.0, 3.0,  0.0,
              { "lin", "exp", "log", "s" } },
            // Slot 2
            { "mod.2.src",    "Mod 2 Src",    ParamEntry::Type::Choice, 0.0, 9.0,  0.0,
              { "None","LFO 1","LFO 2","Env Amp","Env Filt","Velocity","Modwheel","Aftertouch","Keytrack","Random" } },
            { "mod.2.dst",    "Mod 2 Dst",    ParamEntry::Type::Choice, 0.0, 12.0, 0.0,
              { "None","Cutoff","Resonance","Drive","Osc1 Pitch","Osc2 Pitch","Osc1 Level","Osc2 Level","Osc1 PWM","Osc2 PWM","Master Vol","Amp Attack","Amp Release" } },
            { "mod.2.amount", "Mod 2 Amount", ParamEntry::Type::Float,  -1.0, 1.0, 0.0, {} },
            { "mod.2.curve",  "Mod 2 Curve",  ParamEntry::Type::Choice, 0.0, 3.0,  0.0,
              { "lin", "exp", "log", "s" } },
            // Slot 3
            { "mod.3.src",    "Mod 3 Src",    ParamEntry::Type::Choice, 0.0, 9.0,  0.0,
              { "None","LFO 1","LFO 2","Env Amp","Env Filt","Velocity","Modwheel","Aftertouch","Keytrack","Random" } },
            { "mod.3.dst",    "Mod 3 Dst",    ParamEntry::Type::Choice, 0.0, 12.0, 0.0,
              { "None","Cutoff","Resonance","Drive","Osc1 Pitch","Osc2 Pitch","Osc1 Level","Osc2 Level","Osc1 PWM","Osc2 PWM","Master Vol","Amp Attack","Amp Release" } },
            { "mod.3.amount", "Mod 3 Amount", ParamEntry::Type::Float,  -1.0, 1.0, 0.0, {} },
            { "mod.3.curve",  "Mod 3 Curve",  ParamEntry::Type::Choice, 0.0, 3.0,  0.0,
              { "lin", "exp", "log", "s" } },
            // Slot 4
            { "mod.4.src",    "Mod 4 Src",    ParamEntry::Type::Choice, 0.0, 9.0,  0.0,
              { "None","LFO 1","LFO 2","Env Amp","Env Filt","Velocity","Modwheel","Aftertouch","Keytrack","Random" } },
            { "mod.4.dst",    "Mod 4 Dst",    ParamEntry::Type::Choice, 0.0, 12.0, 0.0,
              { "None","Cutoff","Resonance","Drive","Osc1 Pitch","Osc2 Pitch","Osc1 Level","Osc2 Level","Osc1 PWM","Osc2 PWM","Master Vol","Amp Attack","Amp Release" } },
            { "mod.4.amount", "Mod 4 Amount", ParamEntry::Type::Float,  -1.0, 1.0, 0.0, {} },
            { "mod.4.curve",  "Mod 4 Curve",  ParamEntry::Type::Choice, 0.0, 3.0,  0.0,
              { "lin", "exp", "log", "s" } },
            // Slot 5
            { "mod.5.src",    "Mod 5 Src",    ParamEntry::Type::Choice, 0.0, 9.0,  0.0,
              { "None","LFO 1","LFO 2","Env Amp","Env Filt","Velocity","Modwheel","Aftertouch","Keytrack","Random" } },
            { "mod.5.dst",    "Mod 5 Dst",    ParamEntry::Type::Choice, 0.0, 12.0, 0.0,
              { "None","Cutoff","Resonance","Drive","Osc1 Pitch","Osc2 Pitch","Osc1 Level","Osc2 Level","Osc1 PWM","Osc2 PWM","Master Vol","Amp Attack","Amp Release" } },
            { "mod.5.amount", "Mod 5 Amount", ParamEntry::Type::Float,  -1.0, 1.0, 0.0, {} },
            { "mod.5.curve",  "Mod 5 Curve",  ParamEntry::Type::Choice, 0.0, 3.0,  0.0,
              { "lin", "exp", "log", "s" } },
            // Slot 6
            { "mod.6.src",    "Mod 6 Src",    ParamEntry::Type::Choice, 0.0, 9.0,  0.0,
              { "None","LFO 1","LFO 2","Env Amp","Env Filt","Velocity","Modwheel","Aftertouch","Keytrack","Random" } },
            { "mod.6.dst",    "Mod 6 Dst",    ParamEntry::Type::Choice, 0.0, 12.0, 0.0,
              { "None","Cutoff","Resonance","Drive","Osc1 Pitch","Osc2 Pitch","Osc1 Level","Osc2 Level","Osc1 PWM","Osc2 PWM","Master Vol","Amp Attack","Amp Release" } },
            { "mod.6.amount", "Mod 6 Amount", ParamEntry::Type::Float,  -1.0, 1.0, 0.0, {} },
            { "mod.6.curve",  "Mod 6 Curve",  ParamEntry::Type::Choice, 0.0, 3.0,  0.0,
              { "lin", "exp", "log", "s" } },
            // Slot 7
            { "mod.7.src",    "Mod 7 Src",    ParamEntry::Type::Choice, 0.0, 9.0,  0.0,
              { "None","LFO 1","LFO 2","Env Amp","Env Filt","Velocity","Modwheel","Aftertouch","Keytrack","Random" } },
            { "mod.7.dst",    "Mod 7 Dst",    ParamEntry::Type::Choice, 0.0, 12.0, 0.0,
              { "None","Cutoff","Resonance","Drive","Osc1 Pitch","Osc2 Pitch","Osc1 Level","Osc2 Level","Osc1 PWM","Osc2 PWM","Master Vol","Amp Attack","Amp Release" } },
            { "mod.7.amount", "Mod 7 Amount", ParamEntry::Type::Float,  -1.0, 1.0, 0.0, {} },
            { "mod.7.curve",  "Mod 7 Curve",  ParamEntry::Type::Choice, 0.0, 3.0,  0.0,
              { "lin", "exp", "log", "s" } },
            // Slot 8
            { "mod.8.src",    "Mod 8 Src",    ParamEntry::Type::Choice, 0.0, 9.0,  0.0,
              { "None","LFO 1","LFO 2","Env Amp","Env Filt","Velocity","Modwheel","Aftertouch","Keytrack","Random" } },
            { "mod.8.dst",    "Mod 8 Dst",    ParamEntry::Type::Choice, 0.0, 12.0, 0.0,
              { "None","Cutoff","Resonance","Drive","Osc1 Pitch","Osc2 Pitch","Osc1 Level","Osc2 Level","Osc1 PWM","Osc2 PWM","Master Vol","Amp Attack","Amp Release" } },
            { "mod.8.amount", "Mod 8 Amount", ParamEntry::Type::Float,  -1.0, 1.0, 0.0, {} },
            { "mod.8.curve",  "Mod 8 Curve",  ParamEntry::Type::Choice, 0.0, 3.0,  0.0,
              { "lin", "exp", "log", "s" } },
            // Slot 9
            { "mod.9.src",    "Mod 9 Src",    ParamEntry::Type::Choice, 0.0, 9.0,  0.0,
              { "None","LFO 1","LFO 2","Env Amp","Env Filt","Velocity","Modwheel","Aftertouch","Keytrack","Random" } },
            { "mod.9.dst",    "Mod 9 Dst",    ParamEntry::Type::Choice, 0.0, 12.0, 0.0,
              { "None","Cutoff","Resonance","Drive","Osc1 Pitch","Osc2 Pitch","Osc1 Level","Osc2 Level","Osc1 PWM","Osc2 PWM","Master Vol","Amp Attack","Amp Release" } },
            { "mod.9.amount", "Mod 9 Amount", ParamEntry::Type::Float,  -1.0, 1.0, 0.0, {} },
            { "mod.9.curve",  "Mod 9 Curve",  ParamEntry::Type::Choice, 0.0, 3.0,  0.0,
              { "lin", "exp", "log", "s" } },
            // Slot 10
            { "mod.10.src",    "Mod 10 Src",    ParamEntry::Type::Choice, 0.0, 9.0,  0.0,
              { "None","LFO 1","LFO 2","Env Amp","Env Filt","Velocity","Modwheel","Aftertouch","Keytrack","Random" } },
            { "mod.10.dst",    "Mod 10 Dst",    ParamEntry::Type::Choice, 0.0, 12.0, 0.0,
              { "None","Cutoff","Resonance","Drive","Osc1 Pitch","Osc2 Pitch","Osc1 Level","Osc2 Level","Osc1 PWM","Osc2 PWM","Master Vol","Amp Attack","Amp Release" } },
            { "mod.10.amount", "Mod 10 Amount", ParamEntry::Type::Float,  -1.0, 1.0, 0.0, {} },
            { "mod.10.curve",  "Mod 10 Curve",  ParamEntry::Type::Choice, 0.0, 3.0,  0.0,
              { "lin", "exp", "log", "s" } },
            // Slot 11
            { "mod.11.src",    "Mod 11 Src",    ParamEntry::Type::Choice, 0.0, 9.0,  0.0,
              { "None","LFO 1","LFO 2","Env Amp","Env Filt","Velocity","Modwheel","Aftertouch","Keytrack","Random" } },
            { "mod.11.dst",    "Mod 11 Dst",    ParamEntry::Type::Choice, 0.0, 12.0, 0.0,
              { "None","Cutoff","Resonance","Drive","Osc1 Pitch","Osc2 Pitch","Osc1 Level","Osc2 Level","Osc1 PWM","Osc2 PWM","Master Vol","Amp Attack","Amp Release" } },
            { "mod.11.amount", "Mod 11 Amount", ParamEntry::Type::Float,  -1.0, 1.0, 0.0, {} },
            { "mod.11.curve",  "Mod 11 Curve",  ParamEntry::Type::Choice, 0.0, 3.0,  0.0,
              { "lin", "exp", "log", "s" } },
            // Slot 12
            { "mod.12.src",    "Mod 12 Src",    ParamEntry::Type::Choice, 0.0, 9.0,  0.0,
              { "None","LFO 1","LFO 2","Env Amp","Env Filt","Velocity","Modwheel","Aftertouch","Keytrack","Random" } },
            { "mod.12.dst",    "Mod 12 Dst",    ParamEntry::Type::Choice, 0.0, 12.0, 0.0,
              { "None","Cutoff","Resonance","Drive","Osc1 Pitch","Osc2 Pitch","Osc1 Level","Osc2 Level","Osc1 PWM","Osc2 PWM","Master Vol","Amp Attack","Amp Release" } },
            { "mod.12.amount", "Mod 12 Amount", ParamEntry::Type::Float,  -1.0, 1.0, 0.0, {} },
            { "mod.12.curve",  "Mod 12 Curve",  ParamEntry::Type::Choice, 0.0, 3.0,  0.0,
              { "lin", "exp", "log", "s" } },
            // Slot 13
            { "mod.13.src",    "Mod 13 Src",    ParamEntry::Type::Choice, 0.0, 9.0,  0.0,
              { "None","LFO 1","LFO 2","Env Amp","Env Filt","Velocity","Modwheel","Aftertouch","Keytrack","Random" } },
            { "mod.13.dst",    "Mod 13 Dst",    ParamEntry::Type::Choice, 0.0, 12.0, 0.0,
              { "None","Cutoff","Resonance","Drive","Osc1 Pitch","Osc2 Pitch","Osc1 Level","Osc2 Level","Osc1 PWM","Osc2 PWM","Master Vol","Amp Attack","Amp Release" } },
            { "mod.13.amount", "Mod 13 Amount", ParamEntry::Type::Float,  -1.0, 1.0, 0.0, {} },
            { "mod.13.curve",  "Mod 13 Curve",  ParamEntry::Type::Choice, 0.0, 3.0,  0.0,
              { "lin", "exp", "log", "s" } },
            // Slot 14
            { "mod.14.src",    "Mod 14 Src",    ParamEntry::Type::Choice, 0.0, 9.0,  0.0,
              { "None","LFO 1","LFO 2","Env Amp","Env Filt","Velocity","Modwheel","Aftertouch","Keytrack","Random" } },
            { "mod.14.dst",    "Mod 14 Dst",    ParamEntry::Type::Choice, 0.0, 12.0, 0.0,
              { "None","Cutoff","Resonance","Drive","Osc1 Pitch","Osc2 Pitch","Osc1 Level","Osc2 Level","Osc1 PWM","Osc2 PWM","Master Vol","Amp Attack","Amp Release" } },
            { "mod.14.amount", "Mod 14 Amount", ParamEntry::Type::Float,  -1.0, 1.0, 0.0, {} },
            { "mod.14.curve",  "Mod 14 Curve",  ParamEntry::Type::Choice, 0.0, 3.0,  0.0,
              { "lin", "exp", "log", "s" } },
            // Slot 15
            { "mod.15.src",    "Mod 15 Src",    ParamEntry::Type::Choice, 0.0, 9.0,  0.0,
              { "None","LFO 1","LFO 2","Env Amp","Env Filt","Velocity","Modwheel","Aftertouch","Keytrack","Random" } },
            { "mod.15.dst",    "Mod 15 Dst",    ParamEntry::Type::Choice, 0.0, 12.0, 0.0,
              { "None","Cutoff","Resonance","Drive","Osc1 Pitch","Osc2 Pitch","Osc1 Level","Osc2 Level","Osc1 PWM","Osc2 PWM","Master Vol","Amp Attack","Amp Release" } },
            { "mod.15.amount", "Mod 15 Amount", ParamEntry::Type::Float,  -1.0, 1.0, 0.0, {} },
            { "mod.15.curve",  "Mod 15 Curve",  ParamEntry::Type::Choice, 0.0, 3.0,  0.0,
              { "lin", "exp", "log", "s" } },
            // Slot 16
            { "mod.16.src",    "Mod 16 Src",    ParamEntry::Type::Choice, 0.0, 9.0,  0.0,
              { "None","LFO 1","LFO 2","Env Amp","Env Filt","Velocity","Modwheel","Aftertouch","Keytrack","Random" } },
            { "mod.16.dst",    "Mod 16 Dst",    ParamEntry::Type::Choice, 0.0, 12.0, 0.0,
              { "None","Cutoff","Resonance","Drive","Osc1 Pitch","Osc2 Pitch","Osc1 Level","Osc2 Level","Osc1 PWM","Osc2 PWM","Master Vol","Amp Attack","Amp Release" } },
            { "mod.16.amount", "Mod 16 Amount", ParamEntry::Type::Float,  -1.0, 1.0, 0.0, {} },
            { "mod.16.curve",  "Mod 16 Curve",  ParamEntry::Type::Choice, 0.0, 3.0,  0.0,
              { "lin", "exp", "log", "s" } },

            // -----------------------------------------------------------------------
            // FX Chain — 10 generic slots
            // type: 11 choices (None..Filter)
            // bypass: Bool
            // mix: Float 0..1
            // p1..p4: Float 0..1
            // -----------------------------------------------------------------------
            // FX Slot 1
            { "fx.1.type",   "FX 1 Type",   ParamEntry::Type::Choice, 0.0, 10.0, 0.0,
              { "None","Drive","Chorus","Delay","Reverb","EQ3","Comp","Phaser","Flanger","Bitcrush","Filter" } },
            { "fx.1.bypass", "FX 1 Bypass", ParamEntry::Type::Bool,   0.0, 1.0,  0.0, {} },
            { "fx.1.mix",    "FX 1 Mix",    ParamEntry::Type::Float,  0.0, 1.0,  1.0, {} },
            { "fx.1.p1",     "FX 1 P1",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.1.p2",     "FX 1 P2",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.1.p3",     "FX 1 P3",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.1.p4",     "FX 1 P4",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            // FX Slot 2
            { "fx.2.type",   "FX 2 Type",   ParamEntry::Type::Choice, 0.0, 10.0, 0.0,
              { "None","Drive","Chorus","Delay","Reverb","EQ3","Comp","Phaser","Flanger","Bitcrush","Filter" } },
            { "fx.2.bypass", "FX 2 Bypass", ParamEntry::Type::Bool,   0.0, 1.0,  0.0, {} },
            { "fx.2.mix",    "FX 2 Mix",    ParamEntry::Type::Float,  0.0, 1.0,  1.0, {} },
            { "fx.2.p1",     "FX 2 P1",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.2.p2",     "FX 2 P2",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.2.p3",     "FX 2 P3",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.2.p4",     "FX 2 P4",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            // FX Slot 3
            { "fx.3.type",   "FX 3 Type",   ParamEntry::Type::Choice, 0.0, 10.0, 0.0,
              { "None","Drive","Chorus","Delay","Reverb","EQ3","Comp","Phaser","Flanger","Bitcrush","Filter" } },
            { "fx.3.bypass", "FX 3 Bypass", ParamEntry::Type::Bool,   0.0, 1.0,  0.0, {} },
            { "fx.3.mix",    "FX 3 Mix",    ParamEntry::Type::Float,  0.0, 1.0,  1.0, {} },
            { "fx.3.p1",     "FX 3 P1",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.3.p2",     "FX 3 P2",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.3.p3",     "FX 3 P3",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.3.p4",     "FX 3 P4",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            // FX Slot 4
            { "fx.4.type",   "FX 4 Type",   ParamEntry::Type::Choice, 0.0, 10.0, 0.0,
              { "None","Drive","Chorus","Delay","Reverb","EQ3","Comp","Phaser","Flanger","Bitcrush","Filter" } },
            { "fx.4.bypass", "FX 4 Bypass", ParamEntry::Type::Bool,   0.0, 1.0,  0.0, {} },
            { "fx.4.mix",    "FX 4 Mix",    ParamEntry::Type::Float,  0.0, 1.0,  1.0, {} },
            { "fx.4.p1",     "FX 4 P1",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.4.p2",     "FX 4 P2",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.4.p3",     "FX 4 P3",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.4.p4",     "FX 4 P4",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            // FX Slot 5
            { "fx.5.type",   "FX 5 Type",   ParamEntry::Type::Choice, 0.0, 10.0, 0.0,
              { "None","Drive","Chorus","Delay","Reverb","EQ3","Comp","Phaser","Flanger","Bitcrush","Filter" } },
            { "fx.5.bypass", "FX 5 Bypass", ParamEntry::Type::Bool,   0.0, 1.0,  0.0, {} },
            { "fx.5.mix",    "FX 5 Mix",    ParamEntry::Type::Float,  0.0, 1.0,  1.0, {} },
            { "fx.5.p1",     "FX 5 P1",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.5.p2",     "FX 5 P2",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.5.p3",     "FX 5 P3",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.5.p4",     "FX 5 P4",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            // FX Slot 6
            { "fx.6.type",   "FX 6 Type",   ParamEntry::Type::Choice, 0.0, 10.0, 0.0,
              { "None","Drive","Chorus","Delay","Reverb","EQ3","Comp","Phaser","Flanger","Bitcrush","Filter" } },
            { "fx.6.bypass", "FX 6 Bypass", ParamEntry::Type::Bool,   0.0, 1.0,  0.0, {} },
            { "fx.6.mix",    "FX 6 Mix",    ParamEntry::Type::Float,  0.0, 1.0,  1.0, {} },
            { "fx.6.p1",     "FX 6 P1",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.6.p2",     "FX 6 P2",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.6.p3",     "FX 6 P3",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.6.p4",     "FX 6 P4",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            // FX Slot 7
            { "fx.7.type",   "FX 7 Type",   ParamEntry::Type::Choice, 0.0, 10.0, 0.0,
              { "None","Drive","Chorus","Delay","Reverb","EQ3","Comp","Phaser","Flanger","Bitcrush","Filter" } },
            { "fx.7.bypass", "FX 7 Bypass", ParamEntry::Type::Bool,   0.0, 1.0,  0.0, {} },
            { "fx.7.mix",    "FX 7 Mix",    ParamEntry::Type::Float,  0.0, 1.0,  1.0, {} },
            { "fx.7.p1",     "FX 7 P1",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.7.p2",     "FX 7 P2",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.7.p3",     "FX 7 P3",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.7.p4",     "FX 7 P4",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            // FX Slot 8
            { "fx.8.type",   "FX 8 Type",   ParamEntry::Type::Choice, 0.0, 10.0, 0.0,
              { "None","Drive","Chorus","Delay","Reverb","EQ3","Comp","Phaser","Flanger","Bitcrush","Filter" } },
            { "fx.8.bypass", "FX 8 Bypass", ParamEntry::Type::Bool,   0.0, 1.0,  0.0, {} },
            { "fx.8.mix",    "FX 8 Mix",    ParamEntry::Type::Float,  0.0, 1.0,  1.0, {} },
            { "fx.8.p1",     "FX 8 P1",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.8.p2",     "FX 8 P2",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.8.p3",     "FX 8 P3",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.8.p4",     "FX 8 P4",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            // FX Slot 9
            { "fx.9.type",   "FX 9 Type",   ParamEntry::Type::Choice, 0.0, 10.0, 0.0,
              { "None","Drive","Chorus","Delay","Reverb","EQ3","Comp","Phaser","Flanger","Bitcrush","Filter" } },
            { "fx.9.bypass", "FX 9 Bypass", ParamEntry::Type::Bool,   0.0, 1.0,  0.0, {} },
            { "fx.9.mix",    "FX 9 Mix",    ParamEntry::Type::Float,  0.0, 1.0,  1.0, {} },
            { "fx.9.p1",     "FX 9 P1",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.9.p2",     "FX 9 P2",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.9.p3",     "FX 9 P3",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.9.p4",     "FX 9 P4",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            // FX Slot 10
            { "fx.10.type",   "FX 10 Type",   ParamEntry::Type::Choice, 0.0, 10.0, 0.0,
              { "None","Drive","Chorus","Delay","Reverb","EQ3","Comp","Phaser","Flanger","Bitcrush","Filter" } },
            { "fx.10.bypass", "FX 10 Bypass", ParamEntry::Type::Bool,   0.0, 1.0,  0.0, {} },
            { "fx.10.mix",    "FX 10 Mix",    ParamEntry::Type::Float,  0.0, 1.0,  1.0, {} },
            { "fx.10.p1",     "FX 10 P1",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.10.p2",     "FX 10 P2",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.10.p3",     "FX 10 P3",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
            { "fx.10.p4",     "FX 10 P4",     ParamEntry::Type::Float,  0.0, 1.0,  0.5, {} },
        };
        return e;
    }

    const std::vector<ParamEntry>& entries() { return makeEntries(); }

    const ParamEntry* find(const juce::String& id)
    {
        for (auto& e : entries())
            if (e.id == id)
                return &e;
        return nullptr;
    }

    static const char* typeLabel(ParamEntry::Type t)
    {
        switch (t) {
            case ParamEntry::Type::Float:  return "float";
            case ParamEntry::Type::Choice: return "choice";
            case ParamEntry::Type::Bool:   return "bool";
            case ParamEntry::Type::Int:    return "int";
        }
        return "unknown";
    }

    juce::String renderForPrompt()
    {
        juce::String out;
        out << "| id | label | type | range / choices | default |\n";
        out << "|---|---|---|---|---|\n";
        for (auto& e : entries())
        {
            juce::String range;
            if (e.type == ParamEntry::Type::Choice)
                range = e.choices.joinIntoString(", ");
            else if (e.type == ParamEntry::Type::Bool)
                range = "0..1";
            else
                range = juce::String(e.minValue, 3) + ".." + juce::String(e.maxValue, 3);

            out << "| " << e.id
                << " | " << e.label
                << " | " << typeLabel(e.type)
                << " | " << range
                << " | " << juce::String(e.defaultValue, 3)
                << " |\n";
        }
        return out;
    }
}
