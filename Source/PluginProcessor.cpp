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
    delay.prepare(sampleRate, samplesPerBlock);
}

void AISynthProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Dispatch MIDI events at sample-accurate positions
    int currentSample = 0;
    for (const auto meta : midiMessages)
    {
        const int eventPos = meta.samplePosition;
        if (eventPos > currentSample)
        {
            synth.setParams(buildVoiceParams());
            synth.processBlock(buffer, currentSample, eventPos - currentSample);
            currentSample = eventPos;
        }
        synth.handleMidiMessage(meta.getMessage());
    }

    // Process remaining samples after last MIDI event
    const int remaining = buffer.getNumSamples() - currentSample;
    if (remaining > 0) {
        synth.setParams(buildVoiceParams());
        synth.processBlock(buffer, currentSample, remaining);
    }

    // Apply master volume
    const float masterVol = *apvts.getRawParameterValue(ParamIDs::masterVolume);
    buffer.applyGain(masterVol);

    // Delay (post-gain)
    delay.setParams(buildDelayParams());
    delay.process(buffer);
}

VoiceParams AISynthProcessor::buildVoiceParams() const
{
    VoiceParams p;

    p.waveform     = static_cast<Waveform>(
                         static_cast<int>(*apvts.getRawParameterValue(ParamIDs::oscWaveform)));
    p.detuneCents  = *apvts.getRawParameterValue(ParamIDs::oscDetune);
    p.octave       = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::oscOctave));

    p.filterType   = static_cast<FilterType>(
                         static_cast<int>(*apvts.getRawParameterValue(ParamIDs::filterType)));
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
