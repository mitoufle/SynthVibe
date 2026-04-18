#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters/ParameterIDs.h"

AISynthProcessor::AISynthProcessor()
    : AudioProcessor(BusesProperties()
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "AISynthState", ParameterLayout::create())
{
}

void AISynthProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec { sampleRate,
                                  static_cast<juce::uint32>(samplesPerBlock),
                                  2 };
    synth.prepare(spec);
    fxChain.prepare(sampleRate, samplesPerBlock);
    arp.prepare();
}

void AISynthProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Snapshot APVTS une seule fois par bloc (évite 26 getRawParameterValue × N events)
    const VoiceParams vp = buildVoiceParams();

    // Arpeggiator
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) bpm = *b;
    arp.setParams(buildArpParams());
    arp.process(midiMessages, buffer.getNumSamples(), bpm, getSampleRate());

    // Dispatch MIDI events à positions sample-accurate
    int currentSample = 0;
    for (const auto meta : midiMessages)
    {
        const int eventPos = meta.samplePosition;
        if (eventPos > currentSample)
        {
            synth.setParams(vp);
            synth.processBlock(buffer, currentSample, eventPos - currentSample);
            currentSample = eventPos;
        }
        synth.handleMidiMessage(meta.getMessage());
    }

    const int remaining = buffer.getNumSamples() - currentSample;
    if (remaining > 0) {
        synth.setParams(vp);
        synth.processBlock(buffer, currentSample, remaining);
    }

    // FX chain en premier, PUIS master volume (post-fader correct)
    fxChain.setParams(buildDelayParams(), buildChorusParams());
    fxChain.process(buffer);

    const float masterVol = *apvts.getRawParameterValue(ParamIDs::masterVolume);
    buffer.applyGain(masterVol);
}

VoiceParams AISynthProcessor::buildVoiceParams() const
{
    VoiceParams p;

    p.osc1.waveform = static_cast<Waveform>(static_cast<int>(*apvts.getRawParameterValue(ParamIDs::osc1Waveform)));
    p.osc1.octave   = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::osc1Octave));
    p.osc1.semitone = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::osc1Semitone));
    p.osc1.detune   = *apvts.getRawParameterValue(ParamIDs::osc1Detune);
    p.osc1.level    = *apvts.getRawParameterValue(ParamIDs::osc1Level);

    p.osc2.waveform = static_cast<Waveform>(static_cast<int>(*apvts.getRawParameterValue(ParamIDs::osc2Waveform)));
    p.osc2.octave   = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::osc2Octave));
    p.osc2.semitone = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::osc2Semitone));
    p.osc2.detune   = *apvts.getRawParameterValue(ParamIDs::osc2Detune);
    p.osc2.level    = *apvts.getRawParameterValue(ParamIDs::osc2Level);

    p.lfo1.shape = static_cast<Waveform>(static_cast<int>(*apvts.getRawParameterValue(ParamIDs::lfo1Shape)));
    p.lfo1.rate  = *apvts.getRawParameterValue(ParamIDs::lfo1Rate);
    p.lfo1.depth = *apvts.getRawParameterValue(ParamIDs::lfo1Depth);
    p.lfo1.dest  = static_cast<LfoDest>(static_cast<int>(*apvts.getRawParameterValue(ParamIDs::lfo1Dest)));

    p.lfo2.shape = static_cast<Waveform>(static_cast<int>(*apvts.getRawParameterValue(ParamIDs::lfo2Shape)));
    p.lfo2.rate  = *apvts.getRawParameterValue(ParamIDs::lfo2Rate);
    p.lfo2.depth = *apvts.getRawParameterValue(ParamIDs::lfo2Depth);
    p.lfo2.dest  = static_cast<LfoDest>(static_cast<int>(*apvts.getRawParameterValue(ParamIDs::lfo2Dest)));

    p.filterType      = static_cast<FilterType>(static_cast<int>(*apvts.getRawParameterValue(ParamIDs::filterType)));
    p.filterCutoff    = *apvts.getRawParameterValue(ParamIDs::filterCutoff);
    p.filterResonance = *apvts.getRawParameterValue(ParamIDs::filterResonance);
    p.filterEnvAmt    = *apvts.getRawParameterValue(ParamIDs::filterEnvAmt);

    p.ampEnv.attack  = *apvts.getRawParameterValue(ParamIDs::ampAttack);
    p.ampEnv.decay   = *apvts.getRawParameterValue(ParamIDs::ampDecay);
    p.ampEnv.sustain = *apvts.getRawParameterValue(ParamIDs::ampSustain);
    p.ampEnv.release = *apvts.getRawParameterValue(ParamIDs::ampRelease);

    p.fltEnv.attack  = *apvts.getRawParameterValue(ParamIDs::fltAttack);
    p.fltEnv.decay   = *apvts.getRawParameterValue(ParamIDs::fltDecay);
    p.fltEnv.sustain = *apvts.getRawParameterValue(ParamIDs::fltSustain);
    p.fltEnv.release = *apvts.getRawParameterValue(ParamIDs::fltRelease);

    return p;
}

ArpEngine::Params AISynthProcessor::buildArpParams() const
{
    ArpEngine::Params p;
    p.enabled     = *apvts.getRawParameterValue(ParamIDs::arpEnabled) > 0.5f;
    p.mode        = static_cast<ArpEngine::Mode>(static_cast<int>(*apvts.getRawParameterValue(ParamIDs::arpMode)));
    p.rateIndex   = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::arpRate));
    p.octaveRange = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::arpOctaveRange));
    return p;
}

Chorus::Params AISynthProcessor::buildChorusParams() const
{
    Chorus::Params p;
    p.rate  = *apvts.getRawParameterValue(ParamIDs::chorusRate);
    p.depth = *apvts.getRawParameterValue(ParamIDs::chorusDepth);
    p.mix   = *apvts.getRawParameterValue(ParamIDs::chorusMix);
    return p;
}

Delay::Params AISynthProcessor::buildDelayParams() const
{
    Delay::Params p;
    p.timeMs   = *apvts.getRawParameterValue(ParamIDs::delayTime);
    p.feedback = *apvts.getRawParameterValue(ParamIDs::delayFeedback);
    p.mix      = *apvts.getRawParameterValue(ParamIDs::delayMix);
    return p;
}

void AISynthProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, dest);
}

void AISynthProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* AISynthProcessor::createEditor()
{
    return new AISynthEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AISynthProcessor();
}
