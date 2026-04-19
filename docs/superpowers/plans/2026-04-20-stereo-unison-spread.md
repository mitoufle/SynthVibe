# Stereo Unison Spread Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Give each unison sub-voice its own stereo pan position so the synth produces true stereo width from the oscillator layer, controlled by a new independent `Spread` knob.

**Architecture:** `UnisonOscillator::getNextSample()` is replaced with `getNextSample(float& outL, float& outR)` that distributes sub-voice samples across the stereo field using constant-power panning. `Voice` gains a second `Filter` instance and changes its return type to `std::pair<float,float>`. `SynthEngine` accumulates L and R independently and writes them to separate buffer channels.

**Tech Stack:** C++17, JUCE 7.0.9, AudioProcessorValueTreeState, juce::dsp::StateVariableTPTFilter.

---

### Task 1: Rename `unisonSpread` → `unisonDetuneCents` and add `unisonStereoSpread` to VoiceParams

**Files:**
- Modify: `Source/Engine/Voice.h` (VoiceParams struct, line 43)
- Modify: `Source/Engine/Voice.cpp` (setParams, line 27–28)

- [ ] **Step 1: Update VoiceParams in Voice.h**

Replace lines 42–43 in `Source/Engine/Voice.h`:
```cpp
    int   unisonVoices = 1;
    float unisonSpread = 0.f;
```
With:
```cpp
    int   unisonVoices       = 1;
    float unisonDetuneCents  = 0.f;
    float unisonStereoSpread = 0.5f;
```

- [ ] **Step 2: Fix Voice.cpp setParams (line 27–28)**

Replace in `Source/Engine/Voice.cpp`:
```cpp
    osc1.setUnison(p.unisonVoices, p.unisonSpread);
    osc2.setUnison(p.unisonVoices, p.unisonSpread);
```
With:
```cpp
    osc1.setUnison(p.unisonVoices, p.unisonDetuneCents);
    osc2.setUnison(p.unisonVoices, p.unisonDetuneCents);
```

- [ ] **Step 3: Commit**

```bash
git add Source/Engine/Voice.h Source/Engine/Voice.cpp
git commit -m "refactor: rename unisonSpread→unisonDetuneCents, add unisonStereoSpread to VoiceParams"
```

---

### Task 2: Add `unisonStereoSpread` parameter ID and register APVTS parameter

**Files:**
- Modify: `Source/Parameters/ParameterIDs.h` (Unison section, line 67–68)
- Modify: `Source/Parameters/ParameterLayout.cpp` (Unison section, lines 197–201)

- [ ] **Step 1: Add ID constant to ParameterIDs.h**

In `Source/Parameters/ParameterIDs.h`, replace the Unison section:
```cpp
    // Unison
    inline constexpr const char* unisonVoices    = "unison_voices";
    inline constexpr const char* unisonDetune    = "unison_detune";
```
With:
```cpp
    // Unison
    inline constexpr const char* unisonVoices        = "unison_voices";
    inline constexpr const char* unisonDetune        = "unison_detune";
    inline constexpr const char* unisonStereoSpread  = "unison_spread";
```

- [ ] **Step 2: Register the APVTS parameter in ParameterLayout.cpp**

In `Source/Parameters/ParameterLayout.cpp`, replace the Unison section:
```cpp
    // -----------------------------------------------------------------------
    // Unison
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::unisonVoices, "Unison Voices", 1, 7, 1));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::unisonDetune, "Unison Spread",
        NormalisableRange<float>(0.f, 100.f, 0.1f), 0.f));
```
With:
```cpp
    // -----------------------------------------------------------------------
    // Unison
    // -----------------------------------------------------------------------
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::unisonVoices, "Unison Voices", 1, 7, 1));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::unisonDetune, "Unison Detune",
        NormalisableRange<float>(0.f, 100.f, 0.1f), 0.f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::unisonStereoSpread, "Unison Stereo Spread",
        NormalisableRange<float>(0.f, 1.f, 0.01f), 0.5f));
```

- [ ] **Step 3: Commit**

```bash
git add Source/Parameters/ParameterIDs.h Source/Parameters/ParameterLayout.cpp
git commit -m "feat: register unison_spread APVTS parameter"
```

---

### Task 3: Read `unisonStereoSpread` in PluginProcessor

**Files:**
- Modify: `Source/PluginProcessor.cpp` (`buildVoiceParams()`, lines 108–109)

- [ ] **Step 1: Update buildVoiceParams in PluginProcessor.cpp**

Replace lines 108–109 in `Source/PluginProcessor.cpp`:
```cpp
    p.unisonVoices = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::unisonVoices));
    p.unisonSpread = *apvts.getRawParameterValue(ParamIDs::unisonDetune);
```
With:
```cpp
    p.unisonVoices       = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::unisonVoices));
    p.unisonDetuneCents  = *apvts.getRawParameterValue(ParamIDs::unisonDetune);
    p.unisonStereoSpread = *apvts.getRawParameterValue(ParamIDs::unisonStereoSpread);
```

- [ ] **Step 2: Commit**

```bash
git add Source/PluginProcessor.cpp
git commit -m "feat: read unisonStereoSpread from APVTS in buildVoiceParams"
```

---

### Task 4: Update UnisonOscillator to produce stereo output

**Files:**
- Modify: `Source/Engine/UnisonOscillator.h`
- Modify: `Source/Engine/UnisonOscillator.cpp`

- [ ] **Step 1: Update UnisonOscillator.h**

Replace the entire content of `Source/Engine/UnisonOscillator.h` with:
```cpp
#pragma once
#include "Oscillator.h"
#include <algorithm>
#include <cmath>

class UnisonOscillator
{
public:
    static constexpr int MaxUnison = 7;

    void setSampleRate(double sr);
    void setFrequency(double hz);
    void setWaveform(Waveform wf);
    void setDetuneCents(float baseCents);
    void setUnison(int voices, float spreadCents);
    void setStereoSpread(float spread);   // 0 = centre, 1 = full width
    void reset();
    void getNextSample(float& outL, float& outR);

private:
    Oscillator oscs[MaxUnison];
    int   unisonVoices = 1;
    float spreadCents  = 0.f;
    float baseCents    = 0.f;
    float invNormGain  = 1.f;
    float stereoSpread = 0.5f;
    float lGains[MaxUnison] = {};
    float rGains[MaxUnison] = {};

    void recomputeDetune();
    void recomputePan();
};
```

- [ ] **Step 2: Update UnisonOscillator.cpp**

Replace the entire content of `Source/Engine/UnisonOscillator.cpp` with:
```cpp
#include "UnisonOscillator.h"
#include <cmath>

void UnisonOscillator::setSampleRate(double sr)
{
    for (auto& o : oscs)
        o.setSampleRate(sr);
}

void UnisonOscillator::setFrequency(double hz)
{
    for (int i = 0; i < unisonVoices; ++i)
        oscs[i].setFrequency(hz);
}

void UnisonOscillator::setWaveform(Waveform wf)
{
    for (auto& o : oscs)
        o.setWaveform(wf);
}

void UnisonOscillator::setDetuneCents(float base)
{
    baseCents = base;
    recomputeDetune();
}

void UnisonOscillator::setUnison(int voices, float spread)
{
    unisonVoices = std::clamp(voices, 1, MaxUnison);
    spreadCents  = spread;
    invNormGain  = 1.f / std::sqrt(static_cast<float>(unisonVoices));
    recomputeDetune();
    recomputePan();
}

void UnisonOscillator::setStereoSpread(float spread)
{
    stereoSpread = spread;
    recomputePan();
}

void UnisonOscillator::reset()
{
    for (int i = 0; i < MaxUnison; ++i)
        oscs[i].reset();

    if (unisonVoices > 1)
        for (int i = 0; i < unisonVoices; ++i)
            oscs[i].setPhase(static_cast<double>(i) / unisonVoices);
}

void UnisonOscillator::getNextSample(float& outL, float& outR)
{
    outL = 0.f;
    outR = 0.f;
    for (int i = 0; i < unisonVoices; ++i)
    {
        const float s = oscs[i].getNextSample() * invNormGain;
        outL += s * lGains[i];
        outR += s * rGains[i];
    }
}

void UnisonOscillator::recomputeDetune()
{
    for (int i = 0; i < unisonVoices; ++i)
    {
        const float offset = (unisonVoices > 1)
            ? spreadCents * (2.0f * i / static_cast<float>(unisonVoices - 1) - 1.0f)
            : 0.f;
        oscs[i].setDetuneCents(baseCents + offset);
    }
}

void UnisonOscillator::recomputePan()
{
    constexpr float pi = 3.14159265358979f;
    for (int i = 0; i < unisonVoices; ++i)
    {
        // pan ∈ [-stereoSpread, +stereoSpread]; 0 = centre
        const float pan = (unisonVoices > 1)
            ? stereoSpread * (2.f * i / static_cast<float>(unisonVoices - 1) - 1.f)
            : 0.f;
        // constant-power pan: angle ∈ [0, π/2]
        const float angle = (pan + 1.f) * pi / 4.f;
        lGains[i] = std::cos(angle);
        rGains[i] = std::sin(angle);
    }
}
```

- [ ] **Step 3: Commit**

```bash
git add Source/Engine/UnisonOscillator.h Source/Engine/UnisonOscillator.cpp
git commit -m "feat: UnisonOscillator outputs stereo via per-voice constant-power panning"
```

---

### Task 5: Update Voice — stereo output and second filter

**Files:**
- Modify: `Source/Engine/Voice.h`
- Modify: `Source/Engine/Voice.cpp`

- [ ] **Step 1: Update Voice.h**

Replace the entire content of `Source/Engine/Voice.h` with:
```cpp
#pragma once
#include "UnisonOscillator.h"
#include "Envelope.h"
#include "Filter.h"
#include <juce_dsp/juce_dsp.h>
#include <utility>

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

    int   unisonVoices       = 1;
    float unisonDetuneCents  = 0.f;
    float unisonStereoSpread = 0.5f;
};

class Voice
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void setParams(const VoiceParams& p);

    void noteOn (int midiNote, float velocity);
    void noteOff();

    std::pair<float, float> getNextSample();
    bool  isActive()    const { return ampEnv.isActive(); }
    int   getMidiNote() const { return currentNote; }

private:
    UnisonOscillator osc1;
    UnisonOscillator osc2;
    Oscillator lfo1Osc;
    Oscillator lfo2Osc;
    Envelope   ampEnv;
    Envelope   fltEnv;
    Filter     filter;
    Filter     filterR;

    VoiceParams params;
    int    currentNote = -1;
    float  velocity    = 1.f;
    double sampleRate  = 44100.0;

    static double midiNoteToHz(int note, int octaveOffset, int semitoneOffset = 0) noexcept;
};
```

- [ ] **Step 2: Update Voice.cpp**

Replace the entire content of `Source/Engine/Voice.cpp` with:
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
    filterR.prepare(monoSpec);
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
    osc2.setUnison(p.unisonVoices, p.unisonDetuneCents);
    osc1.setStereoSpread(p.unisonStereoSpread);
    osc2.setStereoSpread(p.unisonStereoSpread);

    lfo1Osc.setWaveform(p.lfo1.shape);
    lfo1Osc.setFrequency(p.lfo1.rate);
    lfo1Osc.setDetuneCents(0.f);
    lfo2Osc.setWaveform(p.lfo2.shape);
    lfo2Osc.setFrequency(p.lfo2.rate);
    lfo2Osc.setDetuneCents(0.f);

    filter.setType(p.filterType);
    filter.setCutoff(p.filterCutoff);
    filter.setResonance(p.filterResonance);
    filterR.setType(p.filterType);
    filterR.setCutoff(p.filterCutoff);
    filterR.setResonance(p.filterResonance);
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
    filterR.reset();
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
    }

    // Stereo oscillator samples
    float osc1L = 0.f, osc1R = 0.f, osc2L = 0.f, osc2R = 0.f;
    osc1.getNextSample(osc1L, osc1R);
    osc2.getNextSample(osc2L, osc2R);

    float mixL = osc1L * params.osc1.level + osc2L * params.osc2.level;
    float mixR = osc1R * params.osc1.level + osc2R * params.osc2.level;

    // Filter modulation — update both filter instances identically
    const float envMod = fltEnv.getNextSample();
    const bool hasFilterMod = (params.filterEnvAmt != 0.f)
                            || (params.lfo1.dest == LfoDest::Filter && params.lfo1.depth != 0.f)
                            || (params.lfo2.dest == LfoDest::Filter && params.lfo2.depth != 0.f);
    if (hasFilterMod)
    {
        float cutoff = params.filterCutoff * (1.f + params.filterEnvAmt * envMod);
        cutoff += (params.lfo1.dest == LfoDest::Filter ? l1 * 4000.f : 0.f);
        cutoff += (params.lfo2.dest == LfoDest::Filter ? l2 * 4000.f : 0.f);
        cutoff = juce::jlimit(20.f, 20000.f, cutoff);
        filter.setCutoff(cutoff);
        filterR.setCutoff(cutoff);
    }

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
```

- [ ] **Step 3: Commit**

```bash
git add Source/Engine/Voice.h Source/Engine/Voice.cpp
git commit -m "feat: Voice outputs stereo pair, adds filterR for right channel"
```

---

### Task 6: Update SynthEngine to accumulate stereo

**Files:**
- Modify: `Source/Engine/SynthEngine.cpp` (`processBlock`, lines 36–56)

- [ ] **Step 1: Replace mono accumulation with stereo in SynthEngine.cpp**

Replace the `processBlock` function in `Source/Engine/SynthEngine.cpp`:
```cpp
void SynthEngine::processBlock(juce::AudioBuffer<float>& buffer,
                                int startSample, int numSamples)
{
    const float gain = 1.f / static_cast<float>(NumVoices);

    for (int i = startSample; i < startSample + numSamples; ++i)
    {
        float sumL = 0.f, sumR = 0.f;
        for (auto& v : voices)
        {
            if (v.isActive())
            {
                auto [l, r] = v.getNextSample();
                sumL += l;
                sumR += r;
            }
        }

        buffer.getWritePointer(0)[i] += sumL * gain;
        buffer.getWritePointer(1)[i] += sumR * gain;
    }

    // Update active voice count for the UI (message-thread read via atomic)
    int count = 0;
    for (const auto& v : voices)
        if (v.isActive()) ++count;
    activeVoiceCount.store(count, std::memory_order_relaxed);
}
```

- [ ] **Step 2: Commit**

```bash
git add Source/Engine/SynthEngine.cpp
git commit -m "feat: SynthEngine accumulates stereo L/R from Voice pairs"
```

---

### Task 7: Add Spread knob to SoundTab UI

**Files:**
- Modify: `Source/UI/SoundTab.h`

- [ ] **Step 1: Add member declaration**

In `Source/UI/SoundTab.h`, after line 118:
```cpp
    KnobWithLabel knobUnisonSpread { "Spread", apvts, ParamIDs::unisonDetune,  " ct", 1 };
```
Add:
```cpp
    KnobWithLabel knobUnisonStereo { "Stereo", apvts, ParamIDs::unisonStereoSpread, "", 2 };
```

- [ ] **Step 2: Add addAndMakeVisible call**

In the constructor, after:
```cpp
        addAndMakeVisible(knobUnisonSpread);
```
Add:
```cpp
        addAndMakeVisible(knobUnisonStereo);
```

- [ ] **Step 3: Update layoutKnobs call in resized()**

Replace:
```cpp
        layoutKnobs(c1, { &knobUnisonVoices, &knobUnisonSpread });
```
With:
```cpp
        layoutKnobs(c1, { &knobUnisonVoices, &knobUnisonSpread, &knobUnisonStereo });
```

- [ ] **Step 4: Commit**

```bash
git add Source/UI/SoundTab.h
git commit -m "feat: add Stereo spread knob to Unison panel in SoundTab"
```

---

### Task 8: Build and verify

- [ ] **Step 1: Build**

Run `build.bat` (or the cmake build command used in this project). Expected: zero errors, warnings only for the pre-existing C4100 `slider` warning in `LookAndFeel.h`.

```
cmake --build build --config Release
```

Expected last lines:
```
AISynth.vcxproj -> ...\build\AISynth_artefacts\Release\AI Synth_SharedCode.lib
AISynth_VST3.vcxproj -> ...\build\AISynth_artefacts\Release\VST3\AI Synth.vst3
```

- [ ] **Step 2: Manual verification checklist**

Load the plugin in a DAW or VST host. Check:

1. **1 unison voice, Stereo knob at 0** — play a note. Sound should be centred (equal L and R). Confirm with a stereo meter.
2. **1 unison voice, Stereo knob at 1** — sound should still be centred (N=1 always pans to centre regardless of spread).
3. **3 unison voices, Stereo knob at 0** — all voices centre, sound is mono.
4. **3 unison voices, Stereo knob at 0.5** — three voices: slight left, centre, slight right. Stereo meter shows width.
5. **3 unison voices, Stereo knob at 1** — voices hard left, centre, hard right. Maximum width.
6. **7 unison voices, Stereo knob at 1** — maximum richness and width. Each voice at a different pan position.
7. **LFO → Filter with stereo active** — filter sweep should affect both channels consistently.
8. **Existing presets** — load any saved preset. The Stereo knob defaults to 0.5 (from APVTS default), so existing presets will automatically get moderate stereo width.

- [ ] **Step 3: Final commit if any last-minute fixes were needed**

```bash
git add -A
git commit -m "fix: <describe any build fix>"
```
