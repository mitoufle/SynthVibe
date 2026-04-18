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
}

void Voice::setParams(const VoiceParams& p)
{
    params = p;

    osc1.setWaveform(p.osc1.waveform);
    osc1.setDetuneCents(p.osc1.detune);
    osc2.setWaveform(p.osc2.waveform);
    osc2.setDetuneCents(p.osc2.detune);

    lfo1Osc.setWaveform(p.lfo1.shape);
    lfo1Osc.setFrequency(p.lfo1.rate);
    lfo1Osc.setDetuneCents(0.f);
    lfo2Osc.setWaveform(p.lfo2.shape);
    lfo2Osc.setFrequency(p.lfo2.rate);
    lfo2Osc.setDetuneCents(0.f);

    filter.setType(p.filterType);
    filter.setCutoff(p.filterCutoff);
    filter.setResonance(p.filterResonance);
    ampEnv.setParams(p.ampEnv);
    fltEnv.setParams(p.fltEnv);

    if (currentNote >= 0)
    {
        osc1.setFrequency(midiNoteToHz(currentNote, p.osc1.octave, p.osc1.semitone));
        osc2.setFrequency(midiNoteToHz(currentNote, p.osc2.octave, p.osc2.semitone));
    }
}

void Voice::noteOn(int midiNote, float vel)
{
    currentNote = midiNote;
    velocity    = vel;
    osc1.reset();
    osc2.reset();
    filter.reset();
    osc1.setFrequency(midiNoteToHz(midiNote, params.osc1.octave, params.osc1.semitone));
    osc2.setFrequency(midiNoteToHz(midiNote, params.osc2.octave, params.osc2.semitone));
    ampEnv.noteOn();
    fltEnv.noteOn();
}

void Voice::noteOff()
{
    ampEnv.noteOff();
    fltEnv.noteOff();
}

float Voice::getNextSample()
{
    if (!ampEnv.isActive())
        return 0.f;

    // LFO outputs: always advance phase, scaled by depth
    const float l1 = lfo1Osc.getNextSample() * params.lfo1.depth;
    const float l2 = lfo2Osc.getNextSample() * params.lfo2.depth;

    // Pitch modulation: only compute std::pow when an LFO is actually targeting Pitch
    const bool hasPitchLfo = (params.lfo1.dest == LfoDest::Pitch && params.lfo1.depth > 0.f)
                           || (params.lfo2.dest == LfoDest::Pitch && params.lfo2.depth > 0.f);
    if (hasPitchLfo && currentNote >= 0)
    {
        const float pitchCents = (params.lfo1.dest == LfoDest::Pitch ? l1 * 100.f : 0.f)
                               + (params.lfo2.dest == LfoDest::Pitch ? l2 * 100.f : 0.f);
        const double pitchRatio = std::pow(2.0, pitchCents / 1200.0);
        osc1.setFrequency(midiNoteToHz(currentNote, params.osc1.octave, params.osc1.semitone) * pitchRatio);
        osc2.setFrequency(midiNoteToHz(currentNote, params.osc2.octave, params.osc2.semitone) * pitchRatio);
    }

    // Detune modulation: only apply LFO mod when an LFO targets Detune
    // Static detune is already set by setParams() / noteOn()
    const bool hasDetuneLfo = (params.lfo1.dest == LfoDest::Detune && params.lfo1.depth > 0.f)
                            || (params.lfo2.dest == LfoDest::Detune && params.lfo2.depth > 0.f);
    if (hasDetuneLfo)
    {
        const float detuneMod = (params.lfo1.dest == LfoDest::Detune ? l1 * 50.f : 0.f)
                              + (params.lfo2.dest == LfoDest::Detune ? l2 * 50.f : 0.f);
        osc1.setDetuneCents(params.osc1.detune + detuneMod);
    }

    float sample = osc1.getNextSample() * params.osc1.level
                 + osc2.getNextSample() * params.osc2.level;

    // Filter modulation: only update cutoff when env or LFO is actually modulating it
    const float envMod = fltEnv.getNextSample();
    const bool hasFilterMod = (params.filterEnvAmt != 0.f)
                            || (params.lfo1.dest == LfoDest::Filter && params.lfo1.depth > 0.f)
                            || (params.lfo2.dest == LfoDest::Filter && params.lfo2.depth > 0.f);
    if (hasFilterMod)
    {
        float cutoff = params.filterCutoff * (1.f + params.filterEnvAmt * envMod);
        cutoff += (params.lfo1.dest == LfoDest::Filter ? l1 * 4000.f : 0.f);
        cutoff += (params.lfo2.dest == LfoDest::Filter ? l2 * 4000.f : 0.f);
        cutoff = juce::jlimit(20.f, 20000.f, cutoff);
        filter.setCutoff(cutoff);
    }

    sample = filter.processSample(sample);

    float ampGain = ampEnv.getNextSample() * velocity;
    const bool hasAmpLfo = (params.lfo1.dest == LfoDest::Amp && params.lfo1.depth > 0.f)
                         || (params.lfo2.dest == LfoDest::Amp && params.lfo2.depth > 0.f);
    if (hasAmpLfo)
    {
        ampGain *= 1.f + (params.lfo1.dest == LfoDest::Amp ? l1 * 0.5f : 0.f);
        ampGain *= 1.f + (params.lfo2.dest == LfoDest::Amp ? l2 * 0.5f : 0.f);
    }
    sample *= ampGain;

    return sample;
}

double Voice::midiNoteToHz(int note, int octaveOffset, int semitoneOffset) noexcept
{
    return 440.0 * std::pow(2.0, ((note + octaveOffset * 12 + semitoneOffset) - 69) / 12.0);
}
