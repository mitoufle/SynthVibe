#include "ParameterLayout.h"
#include "ParameterIDs.h"
#include "ModDestinations.h"

using namespace juce;

AudioProcessorValueTreeState::ParameterLayout ParameterLayout::create()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    // -----------------------------------------------------------------------
    // Oscillator 1
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParamIDs::osc1Waveform, "Osc1 Waveform",
        StringArray { "Sine", "Saw", "Square", "Triangle" }, 1));
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::osc1Octave, "Osc1 Octave", -2, 2, 0));
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::osc1Semitone, "Osc1 Semitone", -12, 12, 0));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::osc1Detune, "Osc1 Detune",
        NormalisableRange<float>(-100.f, 100.f, 0.1f), 0.f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::osc1Level, "Osc1 Level",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 1.f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::osc1Phase, "Osc1 Phase",
        NormalisableRange<float>(0.f, 360.f, 1.f), 0.f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::osc1Pwm, "Osc1 PWM",
        NormalisableRange<float>(0.01f, 0.99f, 0.001f), 0.5f));

    // -----------------------------------------------------------------------
    // Oscillator 2
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParamIDs::osc2Waveform, "Osc2 Waveform",
        StringArray { "Sine", "Saw", "Square", "Triangle" }, 1));
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::osc2Octave, "Osc2 Octave", -2, 2, 0));
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::osc2Semitone, "Osc2 Semitone", -12, 12, 0));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::osc2Detune, "Osc2 Detune",
        NormalisableRange<float>(-100.f, 100.f, 0.1f), 0.f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::osc2Level, "Osc2 Level",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 0.f));  // 0 = silent by default
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::osc2Phase, "Osc2 Phase",
        NormalisableRange<float>(0.f, 360.f, 1.f), 0.f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::osc2Pwm, "Osc2 PWM",
        NormalisableRange<float>(0.01f, 0.99f, 0.001f), 0.5f));

    // -----------------------------------------------------------------------
    // LFO 1
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParamIDs::lfo1Shape, "LFO1 Shape",
        StringArray { "Sine", "Saw", "Square", "Triangle" }, 0));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::lfo1Rate, "LFO1 Rate",
        NormalisableRange<float>(0.01f, 20.f, 0.01f, 0.4f), 1.f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::lfo1Depth, "LFO1 Depth",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 0.f));
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParamIDs::lfo1Dest, "LFO1 Dest",
        StringArray { "Pitch", "Filter", "Amp", "Detune" }, 0));

    // -----------------------------------------------------------------------
    // LFO 2
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParamIDs::lfo2Shape, "LFO2 Shape",
        StringArray { "Sine", "Saw", "Square", "Triangle" }, 0));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::lfo2Rate, "LFO2 Rate",
        NormalisableRange<float>(0.01f, 20.f, 0.01f, 0.4f), 1.f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::lfo2Depth, "LFO2 Depth",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 0.f));
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParamIDs::lfo2Dest, "LFO2 Dest",
        StringArray { "Pitch", "Filter", "Amp", "Detune" }, 0));

    // -----------------------------------------------------------------------
    // Filter
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParamIDs::filterType, "Filter Type",
        StringArray { "LP12", "LP24", "HP", "BP", "Notch" }, 0));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::filterCutoff, "Cutoff",
        NormalisableRange<float>(20.f, 20000.f, 0.1f, 0.3f), 8000.f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::filterResonance, "Resonance",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 0.1f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::filterEnvAmt, "Filter Env",
        NormalisableRange<float>(-1.f, 1.f, 0.001f), 0.f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::filterDrive, "Filter Drive",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 0.f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::filterKeytrack, "Filter Keytrack",
        NormalisableRange<float>(-1.f, 1.f, 0.001f), 0.f));

    // -----------------------------------------------------------------------
    // Amplitude envelope
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::ampAttack, "Amp Attack",
        NormalisableRange<float>(0.001f, 10.f, 0.001f, 0.4f), 0.01f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::ampDecay, "Amp Decay",
        NormalisableRange<float>(0.001f, 10.f, 0.001f, 0.4f), 0.1f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::ampSustain, "Amp Sustain",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 0.7f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::ampRelease, "Amp Release",
        NormalisableRange<float>(0.001f, 10.f, 0.001f, 0.4f), 0.2f));

    // -----------------------------------------------------------------------
    // Filter envelope
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::fltAttack, "Flt Attack",
        NormalisableRange<float>(0.001f, 10.f, 0.001f, 0.4f), 0.01f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::fltDecay, "Flt Decay",
        NormalisableRange<float>(0.001f, 10.f, 0.001f, 0.4f), 0.2f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::fltSustain, "Flt Sustain",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 0.5f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::fltRelease, "Flt Release",
        NormalisableRange<float>(0.001f, 10.f, 0.001f, 0.4f), 0.3f));

    // -----------------------------------------------------------------------
    // Master
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::masterVolume, "Master Volume",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 0.8f));

    // -----------------------------------------------------------------------
    // Delay
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::delayTime, "Delay Time",
        NormalisableRange<float>(1.f, 2000.f, 0.1f, 0.4f), 250.f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::delayFeedback, "Delay Feedback",
        NormalisableRange<float>(0.f, 0.95f, 0.001f), 0.35f));

    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::delayMix, "Delay Mix",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 0.0f));

    // -----------------------------------------------------------------------
    // Chorus
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::chorusRate, "Chorus Rate",
        NormalisableRange<float>(0.1f, 5.f, 0.01f), 0.5f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::chorusDepth, "Chorus Depth",
        NormalisableRange<float>(0.001f, 0.015f, 0.0001f), 0.003f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::chorusMix, "Chorus Mix",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 0.f));

    // -----------------------------------------------------------------------
    // Reverb
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::reverbRoom, "Reverb Room",
        NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::reverbDamp, "Reverb Damp",
        NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::reverbMix, "Reverb Mix",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 0.f));

    // -----------------------------------------------------------------------
    // Drive
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParamIDs::driveType, "Drive Type",
        StringArray { "Soft", "Hard", "Fold" }, 0));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::driveAmount, "Drive Amount",
        NormalisableRange<float>(0.f, 24.f, 0.1f), 0.f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::driveMix, "Drive Mix",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 0.f));

    // -----------------------------------------------------------------------
    // Osc1 Unison
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::osc1UnisonVoices, "Osc1 Unison Voices", 1, 7, 1));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::osc1UnisonDetune, "Osc1 Unison Detune",
        NormalisableRange<float>(0.f, 100.f, 0.1f), 0.f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::osc1UnisonSpread, "Osc1 Unison Spread",
        NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));

    // -----------------------------------------------------------------------
    // Osc2 Unison
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::osc2UnisonVoices, "Osc2 Unison Voices", 1, 7, 1));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::osc2UnisonDetune, "Osc2 Unison Detune",
        NormalisableRange<float>(0.f, 100.f, 0.1f), 0.f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::osc2UnisonSpread, "Osc2 Unison Spread",
        NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));

    // -----------------------------------------------------------------------
    // Arp
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterBool>(
        ParamIDs::arpEnabled, "Arp On", false));
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParamIDs::arpMode, "Arp Mode",
        StringArray { "Up", "Down", "UpDown", "Random" }, 0));
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParamIDs::arpRate, "Arp Rate",
        StringArray { "1/16", "1/8", "1/4", "1/2" }, 0));
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::arpOctaveRange, "Arp Octaves", 1, 4, 1));

    // -----------------------------------------------------------------------
    // Modulation Matrix — 16 slots (UI shows 1..8; 9..16 reserved)
    // -----------------------------------------------------------------------
    {
        StringArray srcLabels;
        for (const char* s : SynthVibe::kModSources)
            srcLabels.add(s);

        StringArray dstLabels;
        for (const auto& d : SynthVibe::kDestinations)
            dstLabels.add(d.label);

        const StringArray curveLabels { "lin", "exp", "log", "s" };

        struct SlotIds { const char* src; const char* dst; const char* amount; const char* curve; };
        const SlotIds slots[16] = {
            { ParamIDs::mod1Src,  ParamIDs::mod1Dst,  ParamIDs::mod1Amount,  ParamIDs::mod1Curve  },
            { ParamIDs::mod2Src,  ParamIDs::mod2Dst,  ParamIDs::mod2Amount,  ParamIDs::mod2Curve  },
            { ParamIDs::mod3Src,  ParamIDs::mod3Dst,  ParamIDs::mod3Amount,  ParamIDs::mod3Curve  },
            { ParamIDs::mod4Src,  ParamIDs::mod4Dst,  ParamIDs::mod4Amount,  ParamIDs::mod4Curve  },
            { ParamIDs::mod5Src,  ParamIDs::mod5Dst,  ParamIDs::mod5Amount,  ParamIDs::mod5Curve  },
            { ParamIDs::mod6Src,  ParamIDs::mod6Dst,  ParamIDs::mod6Amount,  ParamIDs::mod6Curve  },
            { ParamIDs::mod7Src,  ParamIDs::mod7Dst,  ParamIDs::mod7Amount,  ParamIDs::mod7Curve  },
            { ParamIDs::mod8Src,  ParamIDs::mod8Dst,  ParamIDs::mod8Amount,  ParamIDs::mod8Curve  },
            { ParamIDs::mod9Src,  ParamIDs::mod9Dst,  ParamIDs::mod9Amount,  ParamIDs::mod9Curve  },
            { ParamIDs::mod10Src, ParamIDs::mod10Dst, ParamIDs::mod10Amount, ParamIDs::mod10Curve },
            { ParamIDs::mod11Src, ParamIDs::mod11Dst, ParamIDs::mod11Amount, ParamIDs::mod11Curve },
            { ParamIDs::mod12Src, ParamIDs::mod12Dst, ParamIDs::mod12Amount, ParamIDs::mod12Curve },
            { ParamIDs::mod13Src, ParamIDs::mod13Dst, ParamIDs::mod13Amount, ParamIDs::mod13Curve },
            { ParamIDs::mod14Src, ParamIDs::mod14Dst, ParamIDs::mod14Amount, ParamIDs::mod14Curve },
            { ParamIDs::mod15Src, ParamIDs::mod15Dst, ParamIDs::mod15Amount, ParamIDs::mod15Curve },
            { ParamIDs::mod16Src, ParamIDs::mod16Dst, ParamIDs::mod16Amount, ParamIDs::mod16Curve },
        };

        for (int i = 0; i < 16; ++i)
        {
            const auto& s = slots[i];
            const auto label = "Mod " + String(i + 1) + " ";
            params.push_back(std::make_unique<AudioParameterChoice>(
                s.src, label + "Src", srcLabels, 0));
            params.push_back(std::make_unique<AudioParameterChoice>(
                s.dst, label + "Dst", dstLabels, 0));
            params.push_back(std::make_unique<AudioParameterFloat>(
                s.amount, label + "Amount",
                NormalisableRange<float>(-1.f, 1.f, 0.001f), 0.f));
            params.push_back(std::make_unique<AudioParameterChoice>(
                s.curve, label + "Curve", curveLabels, 0));
        }
    }

    return { params.begin(), params.end() };
}
