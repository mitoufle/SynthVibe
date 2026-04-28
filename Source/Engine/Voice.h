#pragma once
#include "UnisonOscillator.h"
#include "Envelope.h"
#include "Filter.h"
#include "ModEngine.h"
#include <juce_dsp/juce_dsp.h>
#include <utility>

enum class LfoDest { Pitch = 0, Filter, Amp, Detune };

struct OscParams
{
    Waveform waveform       = Waveform::Saw;
    int      octave         = 0;
    int      semitone       = 0;
    float    detune         = 0.f;
    float    level          = 1.f;
    float    startingPhase  = 0.f;  // degrees, 0–360
    float    pulseWidth     = 0.5f; // 0.01–0.99, only used by Square
    int      tableIdx       = 0;    // valid only when waveform == Waveform::Wavetable
};

struct LfoParams
{
    Waveform shape = Waveform::Sine;
    float    rate  = 1.f;
    float    depth = 0.f;
    LfoDest  dest  = LfoDest::Pitch;
};

struct VoiceParams
{
    OscParams osc1;
    OscParams osc2;   // osc2.level read from APVTS default 0.f — silent until turned up

    LfoParams lfo1;
    LfoParams lfo2;

    FilterType filterType      = FilterType::LP12;
    float      filterCutoff    = 8000.f;
    float      filterResonance = 0.1f;
    float      filterEnvAmt    = 0.f;
    float      filterDrive     = 0.f;   // 0–1
    float      filterKeytrack  = 0.f;   // −1..+1

    Envelope::Params ampEnv;
    Envelope::Params fltEnv;

    int   unisonVoices       = 1;    // osc1
    float unisonDetuneCents  = 0.f;  // osc1
    float unisonStereoSpread = 0.5f; // osc1

    int   osc2UnisonVoices       = 1;
    float osc2UnisonDetuneCents  = 0.f;
    float osc2UnisonStereoSpread = 0.5f;
};

class WavetableBank;

class Voice
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void setParams(const VoiceParams& p);
    void setBankPointer(const WavetableBank* bank) noexcept;

    void noteOn(int midiNote, float velocity, bool stolen = false);
    void noteOff();

    std::pair<float, float> getNextSample();
    bool  isActive()     const { return ampEnv.isActive(); }
    bool  isSustaining() const { return ampEnv.isSustaining(); }
    int   getMidiNote()  const { return currentNote; }
    uint64_t getNoteOnOrder() const noexcept { return noteOnOrder; }
    void     setNoteOnOrder(uint64_t order)  noexcept { noteOnOrder = order; }

    // Per-voice modulation source accessors. Cheap reads — no side effects.
    float getVelocity()         const noexcept { return velocity; }
    float getEnvAmpValue()      const noexcept;
    float getEnvFiltValue()     const noexcept;
    float getKeytrackOctaves()  const noexcept;

    // Modulation matrix integration. Snapshot pointer is non-owning; SynthEngine
    // populates it once per block. Voice consumes it at control rate.
    void setMatrixSnapshot(const SynthVibe::ModEngine::Snapshot* snap) noexcept { matrixSnapshot = snap; }
    float getCurrentEffectiveCutoff() const noexcept { return lastEffectiveCutoff; }

private:
    static constexpr int FilterCoefUpdateRate = 16;   // power of 2 → cheap bitmask; ~3 kHz control rate at 48 kHz
    int filterCoefCounter = 0;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothCutoff;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothResonance;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothOsc1Level;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothOsc2Level;

    UnisonOscillator osc1;
    UnisonOscillator osc2;
    Oscillator lfo1Osc;
    Oscillator lfo2Osc;
    Envelope   ampEnv;
    Envelope   fltEnv;
    Filter     filter;
    Filter     filterR;

    VoiceParams params;
    int      currentNote  = -1;
    float    velocity     = 1.f;
    double   sampleRate   = 44100.0;
    uint64_t noteOnOrder  = 0;
    float    keytrackMultiplier = 1.f;

    const SynthVibe::ModEngine::Snapshot* matrixSnapshot = nullptr;
    SynthVibe::ModBus                     modBus;
    float                                 lastEffectiveCutoff = 1000.f;
    float                                 lfo1Raw = 0.f;
    float                                 lfo2Raw = 0.f;

    static double midiNoteToHz(int note, int octaveOffset, int semitoneOffset = 0) noexcept;
};
