# SynthVibe Phase 1 — Moteur riche + UI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Upgrade the synth from 1-osc to dual-osc + 2 LFOs + Chorus + Arpeggiator, then update the UI with a horizontal classic layout.

**Architecture:** Each audio component (Voice, Chorus, ArpEngine) is modified or created in isolation and connected through VoiceParams / FXChain. Parameters are declared in ParameterIDs.h first, laid out in ParameterLayout.cpp second, and read in buildVoiceParams() third — this order avoids undefined symbol errors at each build step.

**Tech Stack:** C++17, JUCE 7.0.9, APVTS, Visual Studio 2022. Build command: `build.bat` from `AISynth/`. Output: `build\AISynth_artefacts\Release\VST3\AI Synth.vst3`.

---

## File Map

| File | Action | Responsibility |
|---|---|---|
| `Source/Parameters/ParameterIDs.h` | Modify | Rename osc_* → osc1_*, add osc2/lfo/chorus/arp IDs |
| `Source/Parameters/ParameterLayout.cpp` | Modify | Add ranges for all new parameters |
| `Source/Engine/Voice.h` | Modify | OscParams, LfoParams, LfoDest, updated VoiceParams, osc2 + lfo members |
| `Source/Engine/Voice.cpp` | Modify | Dual osc, LFO processing, updated prepare/setParams/noteOn |
| `Source/FX/Chorus.h` | Create | ChorusParams struct + Chorus class interface |
| `Source/FX/Chorus.cpp` | Create | Delay-line chorus implementation |
| `Source/FX/FXChain.h` | Modify | Complete: holds Delay + Chorus, exposes setParams/process |
| `Source/FX/FXChain.cpp` | Create | FXChain implementation |
| `Source/Engine/ArpEngine.h` | Create | ArpEngine class: mode/rate/octave, noteOn/noteOff/process |
| `Source/Engine/ArpEngine.cpp` | Create | Sequence building + sample-accurate MIDI generation |
| `Source/PluginProcessor.h` | Modify | Replace `Delay delay` with `FXChain fxChain`, add `ArpEngine arp`, add `buildChorusParams()` and `buildArpParams()` |
| `Source/PluginProcessor.cpp` | Modify | buildVoiceParams(), prepareToPlay, processBlock with arp + fxChain |
| `Source/UI/LookAndFeel.h` | Modify | Add section accent colors |
| `Source/PluginEditor.h` | Modify | Rewrite with new knobs/comboboxes for all new params |
| `Source/PluginEditor.cpp` | Modify | Rewrite resized() with horizontal 4-row layout |
| `CMakeLists.txt` | Modify | Add Chorus.cpp, FXChain.cpp, ArpEngine.cpp |

---

## Task 1: Parameters — rename and extend

**Files:**
- Modify: `Source/Parameters/ParameterIDs.h`
- Modify: `Source/Parameters/ParameterLayout.cpp`
- Modify: `Source/PluginProcessor.cpp` (only the buildVoiceParams references to old names)

- [ ] **Step 1: Replace old osc IDs and add all new IDs in ParameterIDs.h**

Replace the existing Oscillator block and add below it:

```cpp
namespace ParamIDs
{
    // Oscillator 1  (replaces the old osc_* block)
    inline constexpr const char* osc1Waveform  = "osc1_waveform";
    inline constexpr const char* osc1Octave    = "osc1_octave";
    inline constexpr const char* osc1Semitone  = "osc1_semitone";
    inline constexpr const char* osc1Detune    = "osc1_detune";
    inline constexpr const char* osc1Level     = "osc1_level";

    // Oscillator 2
    inline constexpr const char* osc2Waveform  = "osc2_waveform";
    inline constexpr const char* osc2Octave    = "osc2_octave";
    inline constexpr const char* osc2Semitone  = "osc2_semitone";
    inline constexpr const char* osc2Detune    = "osc2_detune";
    inline constexpr const char* osc2Level     = "osc2_level";

    // LFO 1
    inline constexpr const char* lfo1Shape     = "lfo1_shape";
    inline constexpr const char* lfo1Rate      = "lfo1_rate";
    inline constexpr const char* lfo1Depth     = "lfo1_depth";
    inline constexpr const char* lfo1Dest      = "lfo1_dest";

    // LFO 2
    inline constexpr const char* lfo2Shape     = "lfo2_shape";
    inline constexpr const char* lfo2Rate      = "lfo2_rate";
    inline constexpr const char* lfo2Depth     = "lfo2_depth";
    inline constexpr const char* lfo2Dest      = "lfo2_dest";

    // Filter, Envelopes, Master, Delay — unchanged
    inline constexpr const char* filterType      = "filter_type";
    inline constexpr const char* filterCutoff    = "filter_cutoff";
    inline constexpr const char* filterResonance = "filter_resonance";
    inline constexpr const char* filterEnvAmt    = "filter_env_amt";
    inline constexpr const char* ampAttack       = "amp_attack";
    inline constexpr const char* ampDecay        = "amp_decay";
    inline constexpr const char* ampSustain      = "amp_sustain";
    inline constexpr const char* ampRelease      = "amp_release";
    inline constexpr const char* fltAttack       = "flt_attack";
    inline constexpr const char* fltDecay        = "flt_decay";
    inline constexpr const char* fltSustain      = "flt_sustain";
    inline constexpr const char* fltRelease      = "flt_release";
    inline constexpr const char* masterVolume    = "master_volume";
    inline constexpr const char* delayTime       = "delay_time";
    inline constexpr const char* delayFeedback   = "delay_feedback";
    inline constexpr const char* delayMix        = "delay_mix";

    // Chorus
    inline constexpr const char* chorusRate      = "chorus_rate";
    inline constexpr const char* chorusDepth     = "chorus_depth";
    inline constexpr const char* chorusMix       = "chorus_mix";

    // Arp
    inline constexpr const char* arpEnabled      = "arp_enabled";
    inline constexpr const char* arpMode         = "arp_mode";
    inline constexpr const char* arpRate         = "arp_rate";
    inline constexpr const char* arpOctaveRange  = "arp_octave_range";
}
```

- [ ] **Step 2: Update ParameterLayout.cpp — replace old osc block, add new params**

Replace the Oscillator section and append new sections at the end (before `return`):

```cpp
// Remove the three old oscWaveform/oscDetune/oscOctave push_backs, replace with:

// Osc 1
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

// Osc 2
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

// LFO 1
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

// LFO 2
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

// Chorus
params.push_back(std::make_unique<AudioParameterFloat>(
    ParamIDs::chorusRate, "Chorus Rate",
    NormalisableRange<float>(0.1f, 5.f, 0.01f), 0.5f));
params.push_back(std::make_unique<AudioParameterFloat>(
    ParamIDs::chorusDepth, "Chorus Depth",
    NormalisableRange<float>(0.001f, 0.015f, 0.0001f), 0.003f));
params.push_back(std::make_unique<AudioParameterFloat>(
    ParamIDs::chorusMix, "Chorus Mix",
    NormalisableRange<float>(0.f, 1.f, 0.001f), 0.f));

// Arp
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
```

- [ ] **Step 3: Fix the three references to old osc IDs in PluginProcessor.cpp buildVoiceParams()**

In `buildVoiceParams()`, change:
```cpp
// BEFORE
p.waveform    = static_cast<Waveform>(static_cast<int>(*apvts.getRawParameterValue(ParamIDs::oscWaveform)));
p.detuneCents = *apvts.getRawParameterValue(ParamIDs::oscDetune);
p.octave      = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::oscOctave));

// AFTER (temporary — reads osc1 params into old VoiceParams fields, compiles until Task 3)
p.waveform    = static_cast<Waveform>(static_cast<int>(*apvts.getRawParameterValue(ParamIDs::osc1Waveform)));
p.detuneCents = *apvts.getRawParameterValue(ParamIDs::osc1Detune);
p.octave      = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::osc1Octave));
```

- [ ] **Step 4: Build to verify no compile errors**

```bat
build.bat
```
Expected: Release build succeeds, no errors. Warnings about unused parameters are OK.

- [ ] **Step 5: Commit**

```bash
git add Source/Parameters/ParameterIDs.h Source/Parameters/ParameterLayout.cpp Source/PluginProcessor.cpp
git commit -m "feat: rename osc params to osc1, add osc2/lfo/chorus/arp parameter IDs"
```

---

## Task 2: Voice.h — OscParams, LfoParams, dual osc members

**Files:**
- Modify: `Source/Engine/Voice.h`

- [ ] **Step 1: Replace Voice.h entirely with the new version**

```cpp
#pragma once
#include "Oscillator.h"
#include "Envelope.h"
#include "Filter.h"
#include <juce_dsp/juce_dsp.h>

enum class LfoDest { Pitch = 0, Filter, Amp, Detune };

struct OscParams
{
    Waveform waveform = Waveform::Saw;
    int      octave   = 0;
    int      semitone = 0;
    float    detune   = 0.f;
    float    level    = 1.f;
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

    FilterType filterType      = FilterType::LowPass;
    float      filterCutoff    = 8000.f;
    float      filterResonance = 0.1f;
    float      filterEnvAmt    = 0.f;

    Envelope::Params ampEnv;
    Envelope::Params fltEnv;
};

class Voice
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void setParams(const VoiceParams& p);

    void noteOn (int midiNote, float velocity);
    void noteOff();

    float getNextSample();
    bool  isActive()    const { return ampEnv.isActive(); }
    int   getMidiNote() const { return currentNote; }

private:
    Oscillator osc1;
    Oscillator osc2;
    Oscillator lfo1Osc;
    Oscillator lfo2Osc;
    Envelope   ampEnv;
    Envelope   fltEnv;
    Filter     filter;

    VoiceParams params;
    int    currentNote = -1;
    float  velocity    = 1.f;
    double sampleRate  = 44100.0;

    static double midiNoteToHz(int note, int octaveOffset, int semitoneOffset = 0) noexcept;
};
```

- [ ] **Step 2: Build — expect errors in Voice.cpp (old member names), that's fine**

```bat
build.bat
```
Expected: errors referencing `osc.`, `params.waveform`, `params.detuneCents` etc. — that's correct, Voice.cpp still uses the old struct. Proceed to Task 3.

---

## Task 3: Voice.cpp — implement dual osc + LFO

**Files:**
- Modify: `Source/Engine/Voice.cpp`

- [ ] **Step 1: Replace Voice.cpp entirely**

```cpp
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

    // LFO outputs: -1..+1 scaled by depth
    const float l1 = lfo1Osc.getNextSample() * params.lfo1.depth;
    const float l2 = lfo2Osc.getNextSample() * params.lfo2.depth;

    // Pitch modulation: ±100 cents per unit depth
    const float pitchCents = (params.lfo1.dest == LfoDest::Pitch ? l1 * 100.f : 0.f)
                           + (params.lfo2.dest == LfoDest::Pitch ? l2 * 100.f : 0.f);
    const double pitchRatio = std::pow(2.0, pitchCents / 1200.0);

    if (currentNote >= 0)
    {
        osc1.setFrequency(midiNoteToHz(currentNote, params.osc1.octave, params.osc1.semitone) * pitchRatio);
        osc2.setFrequency(midiNoteToHz(currentNote, params.osc2.octave, params.osc2.semitone) * pitchRatio);
    }

    // Detune LFO: ±50 cents on osc1
    const float detuneMod = (params.lfo1.dest == LfoDest::Detune ? l1 * 50.f : 0.f)
                          + (params.lfo2.dest == LfoDest::Detune ? l2 * 50.f : 0.f);
    osc1.setDetuneCents(params.osc1.detune + detuneMod);

    float sample = osc1.getNextSample() * params.osc1.level
                 + osc2.getNextSample() * params.osc2.level;

    // Filter modulation: ±4000 Hz per unit depth
    const float envMod = fltEnv.getNextSample();
    float cutoff = params.filterCutoff * (1.f + params.filterEnvAmt * envMod);
    cutoff += (params.lfo1.dest == LfoDest::Filter ? l1 * 4000.f : 0.f);
    cutoff += (params.lfo2.dest == LfoDest::Filter ? l2 * 4000.f : 0.f);
    cutoff = juce::jlimit(20.f, 20000.f, cutoff);
    filter.setCutoff(cutoff);

    sample = filter.processSample(sample);

    float ampGain = ampEnv.getNextSample() * velocity;
    ampGain *= 1.f + (params.lfo1.dest == LfoDest::Amp ? l1 * 0.5f : 0.f);
    ampGain *= 1.f + (params.lfo2.dest == LfoDest::Amp ? l2 * 0.5f : 0.f);
    sample *= ampGain;

    return sample;
}

double Voice::midiNoteToHz(int note, int octaveOffset, int semitoneOffset) noexcept
{
    return 440.0 * std::pow(2.0, ((note + octaveOffset * 12 + semitoneOffset) - 69) / 12.0);
}
```

- [ ] **Step 2: Update PluginProcessor.cpp buildVoiceParams() — read all new fields**

Replace the whole `buildVoiceParams()` function:

```cpp
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
```

- [ ] **Step 3: Build and verify**

```bat
build.bat
```
Expected: Release build succeeds. Load the Standalone — play notes, confirm sound. Osc2 is silent (level=0 default).

- [ ] **Step 4: Commit**

```bash
git add Source/Engine/Voice.h Source/Engine/Voice.cpp Source/PluginProcessor.cpp
git commit -m "feat: dual oscillator + 2 LFOs per voice (osc2 silent by default)"
```

---

## Task 4: Chorus FX

**Files:**
- Create: `Source/FX/Chorus.h`
- Create: `Source/FX/Chorus.cpp`

- [ ] **Step 1: Create Chorus.h**

```cpp
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

class Chorus
{
public:
    struct Params
    {
        float rate  = 0.5f;    // Hz,  0.1 – 5.0
        float depth = 0.003f;  // sec, 0.001 – 0.015
        float mix   = 0.f;     // 0 = dry, 1 = wet
    };

    void prepare(double sampleRate, int maxBlockSize);
    void setParams(const Params& p);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    static constexpr int MaxDelayMs = 25;

    std::vector<float> bufL, bufR;
    int    writePos   = 0;
    int    bufSize    = 0;
    double sampleRate = 44100.0;
    double lfoPhase   = 0.0;
    Params params;

    float readInterpolated(const std::vector<float>& buf, float delSamples) const noexcept;
};
```

- [ ] **Step 2: Create Chorus.cpp**

```cpp
#include "Chorus.h"
#include <cmath>

void Chorus::prepare(double sr, int /*maxBlockSize*/)
{
    sampleRate = sr;
    bufSize    = static_cast<int>(sr * MaxDelayMs / 1000.0) + 2;
    bufL.assign(bufSize, 0.f);
    bufR.assign(bufSize, 0.f);
    writePos  = 0;
    lfoPhase  = 0.0;
}

void Chorus::setParams(const Params& p)
{
    params = p;
}

void Chorus::reset()
{
    std::fill(bufL.begin(), bufL.end(), 0.f);
    std::fill(bufR.begin(), bufR.end(), 0.f);
    writePos = 0;
    lfoPhase = 0.0;
}

void Chorus::process(juce::AudioBuffer<float>& buffer)
{
    if (params.mix < 0.001f)
        return;

    const int   numSamples   = buffer.getNumSamples();
    float*      chL          = buffer.getWritePointer(0);
    float*      chR          = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : chL;
    const float lfoIncrement = static_cast<float>(params.rate / sampleRate);

    for (int i = 0; i < numSamples; ++i)
    {
        // LFO: sine, 0..1
        const float lfo = 0.5f * (1.f + std::sin(juce::MathConstants<float>::twoPi * static_cast<float>(lfoPhase)));
        lfoPhase += lfoIncrement;
        if (lfoPhase >= 1.0) lfoPhase -= 1.0;

        // Delay in samples: base 7ms ± depth
        const float delaySamples = static_cast<float>((0.007 + params.depth * lfo) * sampleRate);

        // Write dry signal to buffer
        bufL[writePos] = chL[i];
        bufR[writePos] = chR[i];

        // Read wet (delayed)
        const float wetL = readInterpolated(bufL, delaySamples);
        const float wetR = readInterpolated(bufR, delaySamples);

        // Mix
        chL[i] = chL[i] * (1.f - params.mix) + wetL * params.mix;
        chR[i] = chR[i] * (1.f - params.mix) + wetR * params.mix;

        writePos = (writePos + 1) % bufSize;
    }
}

float Chorus::readInterpolated(const std::vector<float>& buf, float delSamples) const noexcept
{
    const float readPosF = static_cast<float>(writePos) - delSamples;
    int   readPosI       = static_cast<int>(readPosF);
    const float frac     = readPosF - static_cast<float>(readPosI);
    while (readPosI < 0) readPosI += bufSize;
    readPosI %= bufSize;
    const int next = (readPosI + 1) % bufSize;
    return buf[readPosI] * (1.f - frac) + buf[next] * frac;
}
```

- [ ] **Step 3: Build — expect link errors because CMakeLists not updated yet**

```bat
build.bat
```
Expected: compile succeeds if Chorus.cpp is not yet in CMakeLists (it won't be compiled yet, so no errors). Proceed.

---

## Task 5: FXChain + CMakeLists + PluginProcessor

**Files:**
- Modify: `Source/FX/FXChain.h`
- Create: `Source/FX/FXChain.cpp`
- Modify: `CMakeLists.txt`
- Modify: `Source/PluginProcessor.h`
- Modify: `Source/PluginProcessor.cpp`

- [ ] **Step 1: Rewrite FXChain.h**

```cpp
#pragma once
#include "Delay.h"
#include "Chorus.h"

class FXChain
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void setParams(const Delay::Params& dp, const Chorus::Params& cp);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    Delay  delay;
    Chorus chorus;
};
```

- [ ] **Step 2: Create FXChain.cpp**

```cpp
#include "FXChain.h"

void FXChain::prepare(double sr, int maxBlockSize)
{
    delay.prepare(sr, maxBlockSize);
    chorus.prepare(sr, maxBlockSize);
}

void FXChain::setParams(const Delay::Params& dp, const Chorus::Params& cp)
{
    delay.setParams(dp);
    chorus.setParams(cp);
}

void FXChain::process(juce::AudioBuffer<float>& buffer)
{
    delay.process(buffer);
    chorus.process(buffer);
}

void FXChain::reset()
{
    delay.reset();
    chorus.reset();
}
```

- [ ] **Step 3: Add Chorus.cpp and FXChain.cpp to CMakeLists.txt**

In the `target_sources` block, add:
```cmake
    Source/FX/Chorus.cpp
    Source/FX/FXChain.cpp
```
(Replace the commented-out `# Source/FX/FXChain.cpp` line)

- [ ] **Step 4: Update PluginProcessor.h — replace Delay with FXChain, add ArpEngine forward**

```cpp
// Replace:
//   #include "FX/Delay.h"
//   ...
//   Delay delay;
// With:
#include "FX/FXChain.h"
#include "Parameters/ParameterLayout.h"

// In the private section:
SynthEngine synth;
FXChain     fxChain;

// Add these private methods:
Delay::Params   buildDelayParams()  const;
Chorus::Params  buildChorusParams() const;
```

- [ ] **Step 5: Update PluginProcessor.cpp**

In `prepareToPlay`, replace `delay.prepare(...)` with:
```cpp
fxChain.prepare(sampleRate, samplesPerBlock);
```

In `processBlock`, replace:
```cpp
delay.setParams(buildDelayParams());
delay.process(buffer);
```
with:
```cpp
fxChain.setParams(buildDelayParams(), buildChorusParams());
fxChain.process(buffer);
```

Add `buildChorusParams()` after `buildDelayParams()`:
```cpp
Chorus::Params AISynthProcessor::buildChorusParams() const
{
    Chorus::Params p;
    p.rate  = *apvts.getRawParameterValue(ParamIDs::chorusRate);
    p.depth = *apvts.getRawParameterValue(ParamIDs::chorusDepth);
    p.mix   = *apvts.getRawParameterValue(ParamIDs::chorusMix);
    return p;
}
```

- [ ] **Step 6: Build and verify**

```bat
build.bat
```
Expected: full Release build succeeds. Standalone loads, sound works. Chorus mix at 0 = no audible change yet.

- [ ] **Step 7: Commit**

```bash
git add Source/FX/Chorus.h Source/FX/Chorus.cpp Source/FX/FXChain.h Source/FX/FXChain.cpp CMakeLists.txt Source/PluginProcessor.h Source/PluginProcessor.cpp
git commit -m "feat: add Chorus FX, activate FXChain (Delay + Chorus)"
```

---

## Task 6: ArpEngine

**Files:**
- Create: `Source/Engine/ArpEngine.h`
- Create: `Source/Engine/ArpEngine.cpp`
- Modify: `CMakeLists.txt`
- Modify: `Source/PluginProcessor.h`
- Modify: `Source/PluginProcessor.cpp`

- [ ] **Step 1: Create ArpEngine.h**

```cpp
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <utility>

class ArpEngine
{
public:
    enum class Mode { Up = 0, Down, UpDown, Random };

    struct Params
    {
        bool  enabled     = false;
        Mode  mode        = Mode::Up;
        int   rateIndex   = 0;    // 0=1/16, 1=1/8, 2=1/4, 3=1/2
        int   octaveRange = 1;    // 1..4
    };

    void setParams(const Params& p);
    void noteOn (int midiNote, float velocity);
    void noteOff(int midiNote);
    void reset();

    // Rewrites midiBuffer in-place. Call before SynthEngine::handleMidiMessage.
    void process(juce::MidiBuffer& midi, int numSamples, double bpm, double sr);

private:
    Params params;

    struct HeldNote { int note; float velocity; };
    std::vector<HeldNote> heldNotes;
    std::vector<HeldNote> sequence;

    int   stepIndex     = 0;
    int   sampleCounter = 0;
    int   pingDir       = 1;   // for UpDown mode
    int   lastNote      = -1;
    bool  noteIsOn      = false;

    void  buildSequence();
    int   samplesPerStep(double bpm, double sr) const noexcept;
};
```

- [ ] **Step 2: Create ArpEngine.cpp**

```cpp
#include "ArpEngine.h"
#include <algorithm>
#include <random>

void ArpEngine::setParams(const Params& p)
{
    params = p;
    if (!p.enabled) reset();
}

void ArpEngine::noteOn(int midiNote, float velocity)
{
    // Avoid duplicates
    for (auto& h : heldNotes)
        if (h.note == midiNote) return;
    heldNotes.push_back({ midiNote, velocity });
    buildSequence();
}

void ArpEngine::noteOff(int midiNote)
{
    heldNotes.erase(std::remove_if(heldNotes.begin(), heldNotes.end(),
        [midiNote](const HeldNote& h) { return h.note == midiNote; }),
        heldNotes.end());
    buildSequence();
    if (heldNotes.empty()) { stepIndex = 0; sampleCounter = 0; }
}

void ArpEngine::reset()
{
    heldNotes.clear();
    sequence.clear();
    stepIndex     = 0;
    sampleCounter = 0;
    pingDir       = 1;
    lastNote      = -1;
    noteIsOn      = false;
}

void ArpEngine::buildSequence()
{
    sequence.clear();
    if (heldNotes.empty()) return;

    // Sort by pitch
    auto sorted = heldNotes;
    std::sort(sorted.begin(), sorted.end(),
              [](const HeldNote& a, const HeldNote& b) { return a.note < b.note; });

    // Expand to octave range
    for (int oct = 0; oct < params.octaveRange; ++oct)
        for (auto& h : sorted)
            sequence.push_back({ h.note + oct * 12, h.velocity });

    if (params.mode == Mode::Down)
        std::reverse(sequence.begin(), sequence.end());

    if (params.mode == Mode::Random)
    {
        static std::mt19937 rng { std::random_device{}() };
        std::shuffle(sequence.begin(), sequence.end(), rng);
    }

    if (stepIndex >= static_cast<int>(sequence.size()))
        stepIndex = 0;
}

int ArpEngine::samplesPerStep(double bpm, double sr) const noexcept
{
    // rateIndex: 0=1/16, 1=1/8, 2=1/4, 3=1/2
    static constexpr double rateFractions[] = { 0.25, 0.5, 1.0, 2.0 };
    const double beats = rateFractions[juce::jlimit(0, 3, params.rateIndex)];
    return static_cast<int>((60.0 / bpm) * beats * sr);
}

void ArpEngine::process(juce::MidiBuffer& midi, int numSamples, double bpm, double sr)
{
    if (!params.enabled || sequence.empty())
        return;

    juce::MidiBuffer output;
    const int stepLen = samplesPerStep(bpm, sr);

    // Pass through note-off for any active arp note that gets a real note-off
    for (auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOff())
            noteOff(msg.getNoteNumber());
        else if (msg.isNoteOn())
            noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
    }

    for (int i = 0; i < numSamples; ++i)
    {
        if (sampleCounter == 0 && !sequence.empty())
        {
            // Note off previous
            if (noteIsOn && lastNote >= 0)
            {
                output.addEvent(juce::MidiMessage::noteOff(1, lastNote), i);
                noteIsOn = false;
            }

            // Note on next step
            const auto& step = sequence[stepIndex];
            output.addEvent(juce::MidiMessage::noteOn(1, step.note, step.velocity), i);
            lastNote = step.note;
            noteIsOn = true;

            // Advance step index
            if (params.mode == Mode::UpDown)
            {
                stepIndex += pingDir;
                if (stepIndex >= static_cast<int>(sequence.size()) - 1) pingDir = -1;
                if (stepIndex <= 0) pingDir = 1;
                stepIndex = juce::jlimit(0, static_cast<int>(sequence.size()) - 1, stepIndex);
            }
            else
            {
                stepIndex = (stepIndex + 1) % static_cast<int>(sequence.size());
            }
        }
        ++sampleCounter;
        if (sampleCounter >= stepLen) sampleCounter = 0;
    }

    midi = output;
}
```

- [ ] **Step 3: Add ArpEngine.cpp to CMakeLists.txt**

In `target_sources`, add:
```cmake
    Source/Engine/ArpEngine.cpp
```

- [ ] **Step 4: Update PluginProcessor.h**

Add includes and member:
```cpp
#include "Engine/ArpEngine.h"

// In private section:
ArpEngine arp;

// Add private method:
ArpEngine::Params buildArpParams() const;
```

- [ ] **Step 5: Update PluginProcessor.cpp**

Add `buildArpParams()`:
```cpp
ArpEngine::Params AISynthProcessor::buildArpParams() const
{
    ArpEngine::Params p;
    p.enabled     = *apvts.getRawParameterValue(ParamIDs::arpEnabled) > 0.5f;
    p.mode        = static_cast<ArpEngine::Mode>(static_cast<int>(*apvts.getRawParameterValue(ParamIDs::arpMode)));
    p.rateIndex   = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::arpRate));
    p.octaveRange = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::arpOctaveRange));
    return p;
}
```

In `processBlock`, before the MIDI event loop, add:
```cpp
double bpm = 120.0;
if (auto* ph = getPlayHead())
    if (auto pos = ph->getPosition())
        if (auto b = pos->getBpm()) bpm = *b;

arp.setParams(buildArpParams());
arp.process(midiMessages, buffer.getNumSamples(), bpm, getSampleRate());
```

- [ ] **Step 6: Build and verify**

```bat
build.bat
```
Expected: Release build succeeds. Load standalone, hold a chord, enable arp in DAW host — notes arpeggiate up.

- [ ] **Step 7: Commit**

```bash
git add Source/Engine/ArpEngine.h Source/Engine/ArpEngine.cpp CMakeLists.txt Source/PluginProcessor.h Source/PluginProcessor.cpp
git commit -m "feat: add ArpEngine (Up/Down/UpDown/Random, rate, octave range)"
```

---

## Task 7: UI — New horizontal layout

**Files:**
- Modify: `Source/UI/LookAndFeel.h`
- Modify: `Source/PluginEditor.h`
- Modify: `Source/PluginEditor.cpp`

- [ ] **Step 1: Add section colors to LookAndFeel.h**

Add these constants after `colTextDim`:
```cpp
// Section accent colors (used by SynthSection title)
static constexpr auto colOscAccent    = 0xFF2255AA;
static constexpr auto colFilterAccent = 0xFF7722AA;
static constexpr auto colEnvAccent    = 0xFF226622;
static constexpr auto colLfoAccent    = 0xFFAA2266;
static constexpr auto colFxAccent     = 0xFF227799;
static constexpr auto colArpAccent    = 0xFFAA7722;
```

- [ ] **Step 2: Rewrite PluginEditor.h**

Replace the file entirely:

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/LookAndFeel.h"

struct KnobWithLabel : public juce::Component
{
    juce::Slider slider { juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Label  nameLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    KnobWithLabel(const juce::String& name,
                  juce::AudioProcessorValueTreeState& apvts,
                  const juce::String& paramID,
                  const juce::String& valueSuffix   = "",
                  int                 decimalPlaces = 2)
    {
        nameLabel.setText(name, juce::dontSendNotification);
        nameLabel.setJustificationType(juce::Justification::centred);
        nameLabel.setFont(juce::Font(10.f, juce::Font::bold));
        nameLabel.setColour(juce::Label::textColourId, juce::Colour(SynthLookAndFeel::colTextDim));
        addAndMakeVisible(nameLabel);
        slider.setTextValueSuffix(valueSuffix);
        slider.setNumDecimalPlacesToDisplay(decimalPlaces);
        slider.setTooltip("Double-click or Ctrl+click to enter a value");
        addAndMakeVisible(slider);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramID, slider);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        nameLabel.setBounds(b.removeFromTop(14));
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, b.getWidth(), 16);
        slider.setBounds(b);
    }
};

class AISynthEditor : public juce::AudioProcessorEditor
{
public:
    explicit AISynthEditor(AISynthProcessor&);
    ~AISynthEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    AISynthProcessor& processor;
    SynthLookAndFeel  laf;
    juce::TooltipWindow tooltipWindow { this, 500 };

    // --- OSC 1 ---
    juce::ComboBox osc1WaveBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osc1WaveAttach;
    KnobWithLabel knobOsc1Oct     { "Oct",    processor.apvts, "osc1_octave",   "",     0 };
    KnobWithLabel knobOsc1Semi    { "Semi",   processor.apvts, "osc1_semitone", "",     0 };
    KnobWithLabel knobOsc1Detune  { "Detune", processor.apvts, "osc1_detune",   " ct",  1 };
    KnobWithLabel knobOsc1Level   { "Level",  processor.apvts, "osc1_level",    "",     2 };

    // --- OSC 2 ---
    juce::ComboBox osc2WaveBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osc2WaveAttach;
    KnobWithLabel knobOsc2Oct     { "Oct",    processor.apvts, "osc2_octave",   "",     0 };
    KnobWithLabel knobOsc2Semi    { "Semi",   processor.apvts, "osc2_semitone", "",     0 };
    KnobWithLabel knobOsc2Detune  { "Detune", processor.apvts, "osc2_detune",   " ct",  1 };
    KnobWithLabel knobOsc2Level   { "Level",  processor.apvts, "osc2_level",    "",     2 };

    // --- FILTER ---
    juce::ComboBox filterTypeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterTypeAttach;
    KnobWithLabel knobCutoff    { "Cutoff",  processor.apvts, "filter_cutoff",    " Hz", 0 };
    KnobWithLabel knobResonance { "Res",     processor.apvts, "filter_resonance", "",    2 };
    KnobWithLabel knobFilterEnv { "Env Amt", processor.apvts, "filter_env_amt",   "",    2 };

    // --- AMP ENV ---
    KnobWithLabel knobAmpA { "A", processor.apvts, "amp_attack",  " s", 3 };
    KnobWithLabel knobAmpD { "D", processor.apvts, "amp_decay",   " s", 3 };
    KnobWithLabel knobAmpS { "S", processor.apvts, "amp_sustain", "",   2 };
    KnobWithLabel knobAmpR { "R", processor.apvts, "amp_release", " s", 3 };

    // --- FILTER ENV ---
    KnobWithLabel knobFltA { "A", processor.apvts, "flt_attack",  " s", 3 };
    KnobWithLabel knobFltD { "D", processor.apvts, "flt_decay",   " s", 3 };
    KnobWithLabel knobFltS { "S", processor.apvts, "flt_sustain", "",   2 };
    KnobWithLabel knobFltR { "R", processor.apvts, "flt_release", " s", 3 };

    // --- LFO 1 ---
    juce::ComboBox lfo1ShapeBox, lfo1DestBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfo1ShapeAttach, lfo1DestAttach;
    KnobWithLabel knobLfo1Rate  { "Rate",  processor.apvts, "lfo1_rate",  " Hz", 2 };
    KnobWithLabel knobLfo1Depth { "Depth", processor.apvts, "lfo1_depth", "",    2 };

    // --- LFO 2 ---
    juce::ComboBox lfo2ShapeBox, lfo2DestBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfo2ShapeAttach, lfo2DestAttach;
    KnobWithLabel knobLfo2Rate  { "Rate",  processor.apvts, "lfo2_rate",  " Hz", 2 };
    KnobWithLabel knobLfo2Depth { "Depth", processor.apvts, "lfo2_depth", "",    2 };

    // --- DELAY ---
    KnobWithLabel knobDelayTime     { "Time",     processor.apvts, "delay_time",     " ms", 0 };
    KnobWithLabel knobDelayFeedback { "Feedback", processor.apvts, "delay_feedback", "",    2 };
    KnobWithLabel knobDelayMix      { "Mix",      processor.apvts, "delay_mix",      "",    2 };

    // --- CHORUS ---
    KnobWithLabel knobChorusRate  { "Rate",  processor.apvts, "chorus_rate",  " Hz", 2 };
    KnobWithLabel knobChorusDepth { "Depth", processor.apvts, "chorus_depth", "",    3 };
    KnobWithLabel knobChorusMix   { "Mix",   processor.apvts, "chorus_mix",   "",    2 };

    // --- ARP ---
    juce::ToggleButton arpOnButton { "On" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> arpOnAttach;
    juce::ComboBox arpModeBox, arpRateBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> arpModeAttach, arpRateAttach;
    KnobWithLabel knobArpOct { "Octaves", processor.apvts, "arp_octave_range", "", 0 };

    // --- MASTER ---
    KnobWithLabel knobMaster { "Volume", processor.apvts, "master_volume", "", 2 };

    // Section labels
    juce::Label lblOsc1, lblOsc2, lblFilter, lblAmpEnv, lblFltEnv;
    juce::Label lblLfo1, lblLfo2, lblDelay, lblChorus, lblArp, lblMaster;

    void layoutKnobs(juce::Rectangle<int> bounds, std::initializer_list<juce::Component*> knobs);
    void styleLabel(juce::Label& l, const juce::String& text, juce::uint32 colour);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AISynthEditor)
};
```

- [ ] **Step 3: Rewrite PluginEditor.cpp**

```cpp
#include "PluginEditor.h"

AISynthEditor::AISynthEditor(AISynthProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&laf);
    setSize(900, 560);

    // OSC 1 waveform
    osc1WaveBox.addItemList({ "Sine", "Saw", "Square", "Triangle" }, 1);
    osc1WaveAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "osc1_waveform", osc1WaveBox);

    // OSC 2 waveform
    osc2WaveBox.addItemList({ "Sine", "Saw", "Square", "Triangle" }, 1);
    osc2WaveAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "osc2_waveform", osc2WaveBox);

    // Filter type
    filterTypeBox.addItemList({ "Low Pass", "High Pass", "Band Pass" }, 1);
    filterTypeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "filter_type", filterTypeBox);

    // LFO 1
    lfo1ShapeBox.addItemList({ "Sine", "Saw", "Square", "Triangle" }, 1);
    lfo1ShapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "lfo1_shape", lfo1ShapeBox);
    lfo1DestBox.addItemList({ "Pitch", "Filter", "Amp", "Detune" }, 1);
    lfo1DestAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "lfo1_dest", lfo1DestBox);

    // LFO 2
    lfo2ShapeBox.addItemList({ "Sine", "Saw", "Square", "Triangle" }, 1);
    lfo2ShapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "lfo2_shape", lfo2ShapeBox);
    lfo2DestBox.addItemList({ "Pitch", "Filter", "Amp", "Detune" }, 1);
    lfo2DestAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "lfo2_dest", lfo2DestBox);

    // Arp
    arpOnAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.apvts, "arp_enabled", arpOnButton);
    arpModeBox.addItemList({ "Up", "Down", "UpDown", "Random" }, 1);
    arpModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "arp_mode", arpModeBox);
    arpRateBox.addItemList({ "1/16", "1/8", "1/4", "1/2" }, 1);
    arpRateAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "arp_rate", arpRateBox);

    // Section labels
    styleLabel(lblOsc1,   "OSC 1",   SynthLookAndFeel::colOscAccent);
    styleLabel(lblOsc2,   "OSC 2",   SynthLookAndFeel::colOscAccent);
    styleLabel(lblFilter, "FILTER",  SynthLookAndFeel::colFilterAccent);
    styleLabel(lblAmpEnv, "AMP ENV", SynthLookAndFeel::colEnvAccent);
    styleLabel(lblFltEnv, "FLT ENV", SynthLookAndFeel::colEnvAccent);
    styleLabel(lblLfo1,   "LFO 1",   SynthLookAndFeel::colLfoAccent);
    styleLabel(lblLfo2,   "LFO 2",   SynthLookAndFeel::colLfoAccent);
    styleLabel(lblDelay,  "DELAY",   SynthLookAndFeel::colFxAccent);
    styleLabel(lblChorus, "CHORUS",  SynthLookAndFeel::colFxAccent);
    styleLabel(lblArp,    "ARP",     SynthLookAndFeel::colArpAccent);
    styleLabel(lblMaster, "MASTER",  SynthLookAndFeel::colAccent);

    for (auto* c : { &osc1WaveBox, &osc2WaveBox, &filterTypeBox,
                     &lfo1ShapeBox, &lfo1DestBox, &lfo2ShapeBox, &lfo2DestBox,
                     &arpModeBox, &arpRateBox })
        addAndMakeVisible(c);

    addAndMakeVisible(arpOnButton);

    for (auto* c : { (juce::Component*)&knobOsc1Oct, &knobOsc1Semi, &knobOsc1Detune, &knobOsc1Level,
                     &knobOsc2Oct, &knobOsc2Semi, &knobOsc2Detune, &knobOsc2Level,
                     &knobCutoff, &knobResonance, &knobFilterEnv,
                     &knobAmpA, &knobAmpD, &knobAmpS, &knobAmpR,
                     &knobFltA, &knobFltD, &knobFltS, &knobFltR,
                     &knobLfo1Rate, &knobLfo1Depth, &knobLfo2Rate, &knobLfo2Depth,
                     &knobDelayTime, &knobDelayFeedback, &knobDelayMix,
                     &knobChorusRate, &knobChorusDepth, &knobChorusMix,
                     &knobArpOct, &knobMaster })
        addAndMakeVisible(c);
}

AISynthEditor::~AISynthEditor()
{
    setLookAndFeel(nullptr);
}

void AISynthEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(SynthLookAndFeel::colBackground));
}

void AISynthEditor::resized()
{
    auto area  = getLocalBounds().reduced(8);
    const int  pad   = 6;
    const int  comboH = 22;
    const int  labelH = 16;

    // Row heights
    const int row1H = 110;  // OSC row
    const int row2H = 120;  // Filter + Env row
    const int row3H = 100;  // LFO row
    const int row4H = 90;   // FX + Arp + Master row

    // ── ROW 1: OSC 1 | OSC 2 ──────────────────────────────────────────────
    auto row1 = area.removeFromTop(row1H).reduced(0, pad / 2);
    auto osc1Area = row1.removeFromLeft(row1.getWidth() / 2).reduced(pad, 0);
    auto osc2Area = row1.reduced(pad, 0);

    // OSC 1
    lblOsc1.setBounds(osc1Area.removeFromTop(labelH));
    osc1WaveBox.setBounds(osc1Area.removeFromLeft(90).removeFromTop(comboH));
    layoutKnobs(osc1Area, { &knobOsc1Oct, &knobOsc1Semi, &knobOsc1Detune, &knobOsc1Level });

    // OSC 2
    lblOsc2.setBounds(osc2Area.removeFromTop(labelH));
    osc2WaveBox.setBounds(osc2Area.removeFromLeft(90).removeFromTop(comboH));
    layoutKnobs(osc2Area, { &knobOsc2Oct, &knobOsc2Semi, &knobOsc2Detune, &knobOsc2Level });

    // ── ROW 2: Filter | AmpEnv | FltEnv ───────────────────────────────────
    auto row2 = area.removeFromTop(row2H).reduced(0, pad / 2);
    auto filterArea = row2.removeFromLeft(260).reduced(pad, 0);
    auto ampEnvArea = row2.removeFromLeft(row2.getWidth() / 2).reduced(pad, 0);
    auto fltEnvArea = row2.reduced(pad, 0);

    lblFilter.setBounds(filterArea.removeFromTop(labelH));
    filterTypeBox.setBounds(filterArea.removeFromTop(comboH));
    layoutKnobs(filterArea, { &knobCutoff, &knobResonance, &knobFilterEnv });

    lblAmpEnv.setBounds(ampEnvArea.removeFromTop(labelH));
    layoutKnobs(ampEnvArea, { &knobAmpA, &knobAmpD, &knobAmpS, &knobAmpR });

    lblFltEnv.setBounds(fltEnvArea.removeFromTop(labelH));
    layoutKnobs(fltEnvArea, { &knobFltA, &knobFltD, &knobFltS, &knobFltR });

    // ── ROW 3: LFO 1 | LFO 2 ──────────────────────────────────────────────
    auto row3 = area.removeFromTop(row3H).reduced(0, pad / 2);
    auto lfo1Area = row3.removeFromLeft(row3.getWidth() / 2).reduced(pad, 0);
    auto lfo2Area = row3.reduced(pad, 0);

    lblLfo1.setBounds(lfo1Area.removeFromTop(labelH));
    lfo1ShapeBox.setBounds(lfo1Area.removeFromLeft(80).withHeight(comboH));
    lfo1DestBox.setBounds (lfo1Area.removeFromLeft(80).withHeight(comboH));
    layoutKnobs(lfo1Area, { &knobLfo1Rate, &knobLfo1Depth });

    lblLfo2.setBounds(lfo2Area.removeFromTop(labelH));
    lfo2ShapeBox.setBounds(lfo2Area.removeFromLeft(80).withHeight(comboH));
    lfo2DestBox.setBounds (lfo2Area.removeFromLeft(80).withHeight(comboH));
    layoutKnobs(lfo2Area, { &knobLfo2Rate, &knobLfo2Depth });

    // ── ROW 4: Delay | Chorus | Arp | Master ──────────────────────────────
    auto row4 = area.removeFromTop(row4H).reduced(0, pad / 2);
    auto delayArea  = row4.removeFromLeft(180).reduced(pad, 0);
    auto chorusArea = row4.removeFromLeft(180).reduced(pad, 0);
    auto arpArea    = row4.removeFromLeft(200).reduced(pad, 0);
    auto masterArea = row4.reduced(pad, 0);

    lblDelay.setBounds(delayArea.removeFromTop(labelH));
    layoutKnobs(delayArea, { &knobDelayTime, &knobDelayFeedback, &knobDelayMix });

    lblChorus.setBounds(chorusArea.removeFromTop(labelH));
    layoutKnobs(chorusArea, { &knobChorusRate, &knobChorusDepth, &knobChorusMix });

    lblArp.setBounds(arpArea.removeFromTop(labelH));
    arpOnButton.setBounds(arpArea.removeFromLeft(30).removeFromTop(22));
    arpModeBox.setBounds(arpArea.removeFromTop(comboH).removeFromLeft(90));
    arpRateBox.setBounds(arpArea.removeFromTop(comboH).removeFromLeft(70));
    knobArpOct.setBounds(arpArea);

    lblMaster.setBounds(masterArea.removeFromTop(labelH));
    knobMaster.setBounds(masterArea);
}

void AISynthEditor::layoutKnobs(juce::Rectangle<int> bounds,
                                 std::initializer_list<juce::Component*> knobs)
{
    if (knobs.size() == 0) return;
    const int w = bounds.getWidth() / static_cast<int>(knobs.size());
    for (auto* k : knobs)
    {
        k->setBounds(bounds.removeFromLeft(w));
    }
}

void AISynthEditor::styleLabel(juce::Label& l, const juce::String& text, juce::uint32 colour)
{
    l.setText(text, juce::dontSendNotification);
    l.setFont(juce::Font(10.f, juce::Font::bold));
    l.setColour(juce::Label::textColourId, juce::Colour(colour));
    addAndMakeVisible(l);
}
```

- [ ] **Step 4: Build and verify**

```bat
build.bat
```
Expected: Release build succeeds. Open standalone — all knobs and comboboxes visible in 4-row layout. Play MIDI, confirm sound, twist knobs, confirm parameters respond.

- [ ] **Step 5: Commit**

```bash
git add Source/UI/LookAndFeel.h Source/PluginEditor.h Source/PluginEditor.cpp
git commit -m "feat: new horizontal 4-row UI layout with dual osc, LFOs, chorus, arp"
```

---

## Self-review checklist

- [x] All spec requirements have a task: dual osc ✓, 2 LFOs ✓, Chorus ✓, FXChain ✓, ArpEngine ✓, params ✓, UI ✓
- [x] No TBD or TODO placeholders
- [x] Type consistency: `OscParams`, `LfoParams`, `LfoDest` defined in Task 2, used consistently in Tasks 3, 4, 5
- [x] `Chorus::Params` defined in Task 4, used in Tasks 5 and 7
- [x] `ArpEngine::Params` defined in Task 6, `buildArpParams()` uses same field names
- [x] `arpRate` APVTS type is `AudioParameterChoice` (index), read as `static_cast<int>` in `buildArpParams()` ✓
- [x] `arpEnabled` is `AudioParameterBool`, read as `> 0.5f` ✓
- [x] Old `oscWaveform`/`oscDetune`/`oscOctave` IDs fully removed in Task 1
