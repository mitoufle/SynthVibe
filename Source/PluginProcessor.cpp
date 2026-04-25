#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters/ParameterIDs.h"
#include "Engine/ModEngine.h"

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
    fxRunner.prepare(sampleRate, samplesPerBlock);
    arp.prepare();
    smoothMasterVol.reset(sampleRate, 0.005);
    smoothMasterVol.setCurrentAndTargetValue(
        *apvts.getRawParameterValue(ParamIDs::masterVolume));
}

void AISynthProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Merge UI keyboard events with host-sent MIDI so both paths reach the
    // synth through the same sample-accurate dispatch loop below.
    keyboardState.processNextMidiBuffer(midiMessages, 0,
                                        buffer.getNumSamples(), true);

    // Snapshot APVTS une seule fois par bloc (évite 26 getRawParameterValue × N events)
    const VoiceParams vp = buildVoiceParams();

    SynthVibe::ModEngine::Snapshot snap;
    SynthVibe::ModEngine::readSnapshot(apvts, snap);
    synth.setMatrixSnapshot(snap);

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

    // FX runner first, THEN master volume (post-fader)
    fxRunner.setSnapshot(buildFxSnapshot());
    fxRunner.process(buffer);

    smoothMasterVol.setTargetValue(*apvts.getRawParameterValue(ParamIDs::masterVolume));
    const float gainStart = smoothMasterVol.getCurrentValue();
    smoothMasterVol.skip(buffer.getNumSamples());
    const float gainEnd   = smoothMasterVol.getCurrentValue();
    buffer.applyGainRamp(0, buffer.getNumSamples(), gainStart, gainEnd);
}

VoiceParams AISynthProcessor::buildVoiceParams() const
{
    VoiceParams p;

    p.osc1.waveform = static_cast<Waveform>(static_cast<int>(*apvts.getRawParameterValue(ParamIDs::osc1Waveform)));
    p.osc1.octave   = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::osc1Octave));
    p.osc1.semitone = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::osc1Semitone));
    p.osc1.detune   = *apvts.getRawParameterValue(ParamIDs::osc1Detune);
    p.osc1.level    = *apvts.getRawParameterValue(ParamIDs::osc1Level);
    p.osc1.startingPhase = *apvts.getRawParameterValue(ParamIDs::osc1Phase);
    p.osc1.pulseWidth    = *apvts.getRawParameterValue(ParamIDs::osc1Pwm);

    p.osc2.waveform = static_cast<Waveform>(static_cast<int>(*apvts.getRawParameterValue(ParamIDs::osc2Waveform)));
    p.osc2.octave   = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::osc2Octave));
    p.osc2.semitone = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::osc2Semitone));
    p.osc2.detune   = *apvts.getRawParameterValue(ParamIDs::osc2Detune);
    p.osc2.level    = *apvts.getRawParameterValue(ParamIDs::osc2Level);
    p.osc2.startingPhase = *apvts.getRawParameterValue(ParamIDs::osc2Phase);
    p.osc2.pulseWidth    = *apvts.getRawParameterValue(ParamIDs::osc2Pwm);

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
    p.filterDrive     = *apvts.getRawParameterValue(ParamIDs::filterDrive);
    p.filterKeytrack  = *apvts.getRawParameterValue(ParamIDs::filterKeytrack);

    p.ampEnv.attack  = *apvts.getRawParameterValue(ParamIDs::ampAttack);
    p.ampEnv.decay   = *apvts.getRawParameterValue(ParamIDs::ampDecay);
    p.ampEnv.sustain = *apvts.getRawParameterValue(ParamIDs::ampSustain);
    p.ampEnv.release = *apvts.getRawParameterValue(ParamIDs::ampRelease);

    p.fltEnv.attack  = *apvts.getRawParameterValue(ParamIDs::fltAttack);
    p.fltEnv.decay   = *apvts.getRawParameterValue(ParamIDs::fltDecay);
    p.fltEnv.sustain = *apvts.getRawParameterValue(ParamIDs::fltSustain);
    p.fltEnv.release = *apvts.getRawParameterValue(ParamIDs::fltRelease);

    p.unisonVoices       = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::osc1UnisonVoices));
    p.unisonDetuneCents  = *apvts.getRawParameterValue(ParamIDs::osc1UnisonDetune);
    p.unisonStereoSpread = *apvts.getRawParameterValue(ParamIDs::osc1UnisonSpread);

    p.osc2UnisonVoices       = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::osc2UnisonVoices));
    p.osc2UnisonDetuneCents  = *apvts.getRawParameterValue(ParamIDs::osc2UnisonDetune);
    p.osc2UnisonStereoSpread = *apvts.getRawParameterValue(ParamIDs::osc2UnisonSpread);

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

std::array<SynthVibe::FxSlotParams, SynthVibe::FxRunner::kNumSlots>
AISynthProcessor::buildFxSnapshot() const
{
    using SynthVibe::FxSlotParams;
    using SynthVibe::FxType;

    struct SlotIds { const char* type; const char* bypass; const char* mix;
                     const char* p1; const char* p2; const char* p3; const char* p4; };
    static constexpr SlotIds slots[10] = {
        { ParamIDs::fx1Type,  ParamIDs::fx1Bypass,  ParamIDs::fx1Mix,
          ParamIDs::fx1P1,    ParamIDs::fx1P2,      ParamIDs::fx1P3,    ParamIDs::fx1P4  },
        { ParamIDs::fx2Type,  ParamIDs::fx2Bypass,  ParamIDs::fx2Mix,
          ParamIDs::fx2P1,    ParamIDs::fx2P2,      ParamIDs::fx2P3,    ParamIDs::fx2P4  },
        { ParamIDs::fx3Type,  ParamIDs::fx3Bypass,  ParamIDs::fx3Mix,
          ParamIDs::fx3P1,    ParamIDs::fx3P2,      ParamIDs::fx3P3,    ParamIDs::fx3P4  },
        { ParamIDs::fx4Type,  ParamIDs::fx4Bypass,  ParamIDs::fx4Mix,
          ParamIDs::fx4P1,    ParamIDs::fx4P2,      ParamIDs::fx4P3,    ParamIDs::fx4P4  },
        { ParamIDs::fx5Type,  ParamIDs::fx5Bypass,  ParamIDs::fx5Mix,
          ParamIDs::fx5P1,    ParamIDs::fx5P2,      ParamIDs::fx5P3,    ParamIDs::fx5P4  },
        { ParamIDs::fx6Type,  ParamIDs::fx6Bypass,  ParamIDs::fx6Mix,
          ParamIDs::fx6P1,    ParamIDs::fx6P2,      ParamIDs::fx6P3,    ParamIDs::fx6P4  },
        { ParamIDs::fx7Type,  ParamIDs::fx7Bypass,  ParamIDs::fx7Mix,
          ParamIDs::fx7P1,    ParamIDs::fx7P2,      ParamIDs::fx7P3,    ParamIDs::fx7P4  },
        { ParamIDs::fx8Type,  ParamIDs::fx8Bypass,  ParamIDs::fx8Mix,
          ParamIDs::fx8P1,    ParamIDs::fx8P2,      ParamIDs::fx8P3,    ParamIDs::fx8P4  },
        { ParamIDs::fx9Type,  ParamIDs::fx9Bypass,  ParamIDs::fx9Mix,
          ParamIDs::fx9P1,    ParamIDs::fx9P2,      ParamIDs::fx9P3,    ParamIDs::fx9P4  },
        { ParamIDs::fx10Type, ParamIDs::fx10Bypass, ParamIDs::fx10Mix,
          ParamIDs::fx10P1,   ParamIDs::fx10P2,     ParamIDs::fx10P3,   ParamIDs::fx10P4 },
    };

    std::array<FxSlotParams, SynthVibe::FxRunner::kNumSlots> out;
    for (int i = 0; i < SynthVibe::FxRunner::kNumSlots; ++i)
    {
        const auto& s = slots[i];
        FxSlotParams& p = out[i];
        p.type   = static_cast<FxType>(static_cast<int>(*apvts.getRawParameterValue(s.type)));
        p.bypass = *apvts.getRawParameterValue(s.bypass) > 0.5f;
        p.mix    = *apvts.getRawParameterValue(s.mix);
        p.p1     = *apvts.getRawParameterValue(s.p1);
        p.p2     = *apvts.getRawParameterValue(s.p2);
        p.p3     = *apvts.getRawParameterValue(s.p3);
        p.p4     = *apvts.getRawParameterValue(s.p4);
    }
    return out;
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
