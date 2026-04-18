#include "Voice.h"
#include <cmath>

void Voice::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    osc.setSampleRate(spec.sampleRate);
    ampEnv.setSampleRate(spec.sampleRate);
    fltEnv.setSampleRate(spec.sampleRate);

    // SVF needs a mono spec for per-sample processing
    juce::dsp::ProcessSpec monoSpec { spec.sampleRate, spec.maximumBlockSize, 1 };
    filter.prepare(monoSpec);
}

void Voice::setParams(const VoiceParams& p)
{
    params = p;
    osc.setWaveform(p.waveform);
    osc.setDetuneCents(p.detuneCents);
    filter.setType(p.filterType);
    filter.setCutoff(p.filterCutoff);
    filter.setResonance(p.filterResonance);
    ampEnv.setParams(p.ampEnv);
    fltEnv.setParams(p.fltEnv);

    if (currentNote >= 0)
        osc.setFrequency(midiNoteToHz(currentNote, p.octave));
}

void Voice::noteOn(int midiNote, float vel)
{
    currentNote = midiNote;
    velocity    = vel;
    osc.reset();
    filter.reset();
    osc.setFrequency(midiNoteToHz(midiNote, params.octave));
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

    // Modulate filter cutoff with filter envelope
    const float envMod    = fltEnv.getNextSample();
    const float modCutoff = params.filterCutoff
                          * (1.f + params.filterEnvAmt * envMod);
    filter.setCutoff(modCutoff);

    float sample = osc.getNextSample();
    sample       = filter.processSample(sample);
    sample      *= ampEnv.getNextSample() * velocity;

    return sample;
}

double Voice::midiNoteToHz(int note, int octaveOffset) noexcept
{
    return 440.0 * std::pow(2.0, ((note + octaveOffset * 12) - 69) / 12.0);
}
