#include "Voice.h"
#include <cmath>

void Voice::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    osc1.setSampleRate(spec.sampleRate);
    osc2.setSampleRate(spec.sampleRate);
    lfo1Osc.setSampleRate(spec.sampleRate);
    lfo2Osc.setSampleRate(spec.sampleRate);
    ampEnv.setSampleRate(spec.sampleRate);
    fltEnv.setSampleRate(spec.sampleRate);
    juce::dsp::ProcessSpec monoSpec { spec.sampleRate, spec.maximumBlockSize, 1 };
    filter.prepare(monoSpec);
    filterR.prepare(monoSpec);
    smoothCutoff.reset    (spec.sampleRate, 0.005);
    smoothResonance.reset (spec.sampleRate, 0.005);
    smoothOsc1Level.reset (spec.sampleRate, 0.005);
    smoothOsc2Level.reset (spec.sampleRate, 0.005);
    smoothCutoff.setCurrentAndTargetValue    (8000.f);
    smoothResonance.setCurrentAndTargetValue (0.1f);
    smoothOsc1Level.setCurrentAndTargetValue (1.f);
    smoothOsc2Level.setCurrentAndTargetValue (0.f);
    filterCoefCounter = 0;
}

void Voice::setParams(const VoiceParams& p)
{
    params = p;

    osc1.setWaveform(p.osc1.waveform);
    osc1.setDetuneCents(p.osc1.detune);
    osc2.setWaveform(p.osc2.waveform);
    osc2.setDetuneCents(p.osc2.detune);
    // setUnison MUST precede the setFrequency tail block below so that
    // newly-activated unison slots receive the correct base frequency.
    osc1.setUnison(p.unisonVoices, p.unisonDetuneCents);
    osc1.setStereoSpread(p.unisonStereoSpread);
    osc2.setUnison(p.osc2UnisonVoices, p.osc2UnisonDetuneCents);
    osc2.setStereoSpread(p.osc2UnisonStereoSpread);

    osc1.setStartingPhase(p.osc1.startingPhase);
    osc1.setPulseWidth   (p.osc1.pulseWidth);
    osc2.setStartingPhase(p.osc2.startingPhase);
    osc2.setPulseWidth   (p.osc2.pulseWidth);

    lfo1Osc.setWaveform(p.lfo1.shape);
    lfo1Osc.setFrequency(p.lfo1.rate);
    lfo1Osc.setDetuneCents(0.f);
    lfo2Osc.setWaveform(p.lfo2.shape);
    lfo2Osc.setFrequency(p.lfo2.rate);
    lfo2Osc.setDetuneCents(0.f);

    filter.setType(p.filterType);
    filter.setCutoff(p.filterCutoff);
    filter.setResonance(p.filterResonance);
    filter.setDrive(p.filterDrive);
    filterR.setType(p.filterType);
    filterR.setCutoff(p.filterCutoff);
    filterR.setResonance(p.filterResonance);
    filterR.setDrive(p.filterDrive);
    ampEnv.setParams(p.ampEnv);
    fltEnv.setParams(p.fltEnv);
    smoothCutoff.setTargetValue(p.filterCutoff);
    smoothResonance.setTargetValue(p.filterResonance);
    smoothOsc1Level.setTargetValue(p.osc1.level);
    smoothOsc2Level.setTargetValue(p.osc2.level);

    if (currentNote >= 0 && p.filterKeytrack != 0.f)
        keytrackMultiplier = std::pow(2.f, (currentNote - 60.f) / 12.f * p.filterKeytrack);
    else
        keytrackMultiplier = 1.f;

    if (currentNote >= 0)
    {
        osc1.setFrequency(midiNoteToHz(currentNote, p.osc1.octave, p.osc1.semitone));
        osc2.setFrequency(midiNoteToHz(currentNote, p.osc2.octave, p.osc2.semitone));
    }
}

void Voice::noteOn(int midiNote, float vel, bool stolen)
{
    currentNote = midiNote;
    keytrackMultiplier = params.filterKeytrack != 0.f
        ? std::pow(2.f, (currentNote - 60.f) / 12.f * params.filterKeytrack)
        : 1.f;
    velocity    = vel;
    if (!stolen)
    {
        osc1.resetAllPhasesToStart();
        osc2.resetAllPhasesToStart();
        filter.reset();
        filterR.reset();
    }
    osc1.setFrequency(midiNoteToHz(midiNote, params.osc1.octave, params.osc1.semitone));
    osc2.setFrequency(midiNoteToHz(midiNote, params.osc2.octave, params.osc2.semitone));
    ampEnv.noteOn();
    fltEnv.noteOn();
    filterCoefCounter = 0;
}

void Voice::noteOff()
{
    ampEnv.noteOff();
    fltEnv.noteOff();
}

std::pair<float, float> Voice::getNextSample()
{
    if (!ampEnv.isActive())
        return { 0.f, 0.f };

    // LFO outputs: always advance phase, scaled by depth
    const float l1 = lfo1Osc.getNextSample() * params.lfo1.depth;
    const float l2 = lfo2Osc.getNextSample() * params.lfo2.depth;

    // Pitch modulation
    const bool hasPitchLfo = (params.lfo1.dest == LfoDest::Pitch && params.lfo1.depth != 0.f)
                           || (params.lfo2.dest == LfoDest::Pitch && params.lfo2.depth != 0.f);
    if (hasPitchLfo && currentNote >= 0)
    {
        const float pitchCents = (params.lfo1.dest == LfoDest::Pitch ? l1 * 100.f : 0.f)
                               + (params.lfo2.dest == LfoDest::Pitch ? l2 * 100.f : 0.f);
        const double pitchRatio = std::pow(2.0, pitchCents / 1200.0);
        osc1.setFrequency(midiNoteToHz(currentNote, params.osc1.octave, params.osc1.semitone) * pitchRatio);
        osc2.setFrequency(midiNoteToHz(currentNote, params.osc2.octave, params.osc2.semitone) * pitchRatio);
    }

    // Detune modulation
    const bool hasDetuneLfo = (params.lfo1.dest == LfoDest::Detune && params.lfo1.depth != 0.f)
                            || (params.lfo2.dest == LfoDest::Detune && params.lfo2.depth != 0.f);
    if (hasDetuneLfo)
    {
        const float detuneMod = (params.lfo1.dest == LfoDest::Detune ? l1 * 50.f : 0.f)
                              + (params.lfo2.dest == LfoDest::Detune ? l2 * 50.f : 0.f);
        osc1.setDetuneCents(params.osc1.detune + detuneMod);
        osc2.setDetuneCents(params.osc2.detune + detuneMod);
    }

    // Stereo oscillator samples
    float osc1L = 0.f, osc1R = 0.f, osc2L = 0.f, osc2R = 0.f;
    osc1.getNextSample(osc1L, osc1R);
    osc2.getNextSample(osc2L, osc2R);

    const float sOsc1Level = smoothOsc1Level.getNextValue();
    const float sOsc2Level = smoothOsc2Level.getNextValue();
    float mixL = osc1L * sOsc1Level + osc2L * sOsc2Level;
    float mixR = osc1R * sOsc1Level + osc2R * sOsc2Level;

    // Filter modulation — update both filter instances identically
    const float sCutoff = smoothCutoff.getNextValue();
    const float sRes    = smoothResonance.getNextValue();
    if (smoothResonance.isSmoothing())
    {
        filter.setResonance(sRes);
        filterR.setResonance(sRes);
    }
    const float envMod = fltEnv.getNextSample();
    const bool hasFilterMod = (params.filterEnvAmt != 0.f)
                            || (params.lfo1.dest == LfoDest::Filter && params.lfo1.depth != 0.f)
                            || (params.lfo2.dest == LfoDest::Filter && params.lfo2.depth != 0.f)
                            || (params.filterKeytrack != 0.f);
    if ((hasFilterMod || smoothCutoff.isSmoothing()) && filterCoefCounter == 0)
    {
        float cutoff = sCutoff * keytrackMultiplier * (1.f + params.filterEnvAmt * envMod);
        cutoff += (params.lfo1.dest == LfoDest::Filter ? l1 * 4000.f : 0.f);
        cutoff += (params.lfo2.dest == LfoDest::Filter ? l2 * 4000.f : 0.f);
        cutoff = juce::jlimit(20.f, 20000.f, cutoff);
        filter.setCutoff(cutoff);
        filterR.setCutoff(cutoff);
    }
    filterCoefCounter = (filterCoefCounter + 1) & (FilterCoefUpdateRate - 1);

    mixL = filter.processSample(mixL);
    mixR = filterR.processSample(mixR);

    float ampGain = ampEnv.getNextSample() * velocity;
    const bool hasAmpLfo = (params.lfo1.dest == LfoDest::Amp && params.lfo1.depth != 0.f)
                         || (params.lfo2.dest == LfoDest::Amp && params.lfo2.depth != 0.f);
    if (hasAmpLfo)
    {
        ampGain *= 1.f + (params.lfo1.dest == LfoDest::Amp ? l1 * 0.5f : 0.f);
        ampGain *= 1.f + (params.lfo2.dest == LfoDest::Amp ? l2 * 0.5f : 0.f);
    }

    return { mixL * ampGain, mixR * ampGain };
}

double Voice::midiNoteToHz(int note, int octaveOffset, int semitoneOffset) noexcept
{
    return 440.0 * std::pow(2.0, ((note + octaveOffset * 12 + semitoneOffset) - 69) / 12.0);
}
