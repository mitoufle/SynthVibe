# Parameter Smoothing (I-4) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Eliminate zipper noise on knob moves by adding `juce::SmoothedValue<float, Linear>` (5 ms ramp) to Voice audio-rate params, master volume, and all four FX mix controls.

**Architecture:** Smoothers live where per-sample work already happens. Voice owns four smoothers (cutoff, resonance, osc1Level, osc2Level). PluginProcessor owns one (masterVol) and uses `skip()`+`applyGainRamp`. Each FX unit owns one smoother for its mix. Reverb requires a pre-allocated dry buffer because `juce::Reverb` processes the whole block at once; it is restructured to run full-wet internally and blend manually.

**Tech Stack:** C++17, JUCE 7.0.9, MSVC (VS2022). Build: `cmake.exe --build build --config Release --target <target>`. Tests: `build/AISynthTests_artefacts/Release/AISynthTests.exe`.

cmake path: `C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe`

---

## File Map

| File | Change |
|---|---|
| `Source/Engine/Voice.h` | Add 4 `SmoothedValue` members |
| `Source/Engine/Voice.cpp` | `prepare` / `setParams` / `getNextSample` |
| `Source/PluginProcessor.h` | Add `smoothMasterVol` member |
| `Source/PluginProcessor.cpp` | `prepareToPlay` + end of `processBlock` |
| `Source/FX/Delay.h` | Add `smoothMix` member |
| `Source/FX/Delay.cpp` | `prepare` / `setParams` / `process` |
| `Source/FX/Chorus.h` | Add `smoothMix` member |
| `Source/FX/Chorus.cpp` | `prepare` / `setParams` / `process` |
| `Source/FX/Drive.h` | Add `smoothMix` member |
| `Source/FX/Drive.cpp` | `prepare` / `setParams` / `process` (restructure loops) |
| `Source/FX/Reverb.h` | Add `smoothMix` + `dryBuf` members |
| `Source/FX/Reverb.cpp` | `prepare` / `setParams` / `process` |

---

## Task 1 — Voice: smooth filterCutoff, filterResonance, osc1Level, osc2Level

**Files:**
- Modify: `Source/Engine/Voice.h`
- Modify: `Source/Engine/Voice.cpp`

- [ ] **Step 1: Add four SmoothedValue members to Voice.h**

In `Voice.h`, after the `private:` line, add the four smoothers before the existing members:

```cpp
private:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothCutoff;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothResonance;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothOsc1Level;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothOsc2Level;

    UnisonOscillator osc1;
    // ... rest unchanged
```

- [ ] **Step 2: Initialise smoothers in Voice::prepare()**

In `Voice.cpp`, at the end of `prepare()`, add:

```cpp
    smoothCutoff.reset    (spec.sampleRate, 0.005);
    smoothResonance.reset (spec.sampleRate, 0.005);
    smoothOsc1Level.reset (spec.sampleRate, 0.005);
    smoothOsc2Level.reset (spec.sampleRate, 0.005);
    smoothCutoff.setCurrentAndTargetValue    (8000.f);
    smoothResonance.setCurrentAndTargetValue (0.1f);
    smoothOsc1Level.setCurrentAndTargetValue (1.f);
    smoothOsc2Level.setCurrentAndTargetValue (0.f);
```

- [ ] **Step 3: Set target values in Voice::setParams()**

In `Voice.cpp`, replace the two filter lines in `setParams()`:

```cpp
    filter.setType(p.filterType);
    filter.setCutoff(p.filterCutoff);
    filter.setResonance(p.filterResonance);
    filterR.setType(p.filterType);
    filterR.setCutoff(p.filterCutoff);
    filterR.setResonance(p.filterResonance);
```

with:

```cpp
    filter.setType(p.filterType);
    filter.setResonance(p.filterResonance);
    filter.setCutoff(p.filterCutoff);
    filterR.setType(p.filterType);
    filterR.setResonance(p.filterResonance);
    filterR.setCutoff(p.filterCutoff);
    smoothCutoff.setTargetValue(p.filterCutoff);
    smoothResonance.setTargetValue(p.filterResonance);
    smoothOsc1Level.setTargetValue(p.osc1.level);
    smoothOsc2Level.setTargetValue(p.osc2.level);
```

(The direct filter calls keep the filter correct if setParams is called before any notes play; the smoothers handle the per-sample ramping during playback.)

- [ ] **Step 4: Use smoothed values in Voice::getNextSample()**

In `Voice.cpp`, in `getNextSample()`, replace the osc mix lines:

```cpp
    float mixL = osc1L * params.osc1.level + osc2L * params.osc2.level;
    float mixR = osc1R * params.osc1.level + osc2R * params.osc2.level;
```

with:

```cpp
    const float sOsc1Level = smoothOsc1Level.getNextValue();
    const float sOsc2Level = smoothOsc2Level.getNextValue();
    float mixL = osc1L * sOsc1Level + osc2L * sOsc2Level;
    float mixR = osc1R * sOsc1Level + osc2R * sOsc2Level;
```

Then replace the filter modulation block:

```cpp
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
```

with:

```cpp
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
                            || (params.lfo2.dest == LfoDest::Filter && params.lfo2.depth != 0.f);
    if (hasFilterMod || smoothCutoff.isSmoothing())
    {
        float cutoff = sCutoff * (1.f + params.filterEnvAmt * envMod);
        cutoff += (params.lfo1.dest == LfoDest::Filter ? l1 * 4000.f : 0.f);
        cutoff += (params.lfo2.dest == LfoDest::Filter ? l2 * 4000.f : 0.f);
        cutoff = juce::jlimit(20.f, 20000.f, cutoff);
        filter.setCutoff(cutoff);
        filterR.setCutoff(cutoff);
    }
```

- [ ] **Step 5: Build and run tests**

```bat
"C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" --build build --config Release --target AISynthTests
./build/AISynthTests_artefacts/Release/AISynthTests.exe
```

Expected: all tests pass, exit code 0.

- [ ] **Step 6: Commit**

```bash
git add Source/Engine/Voice.h Source/Engine/Voice.cpp
git commit -m "feat: smooth filterCutoff, filterResonance, osc1/2Level in Voice (I-4)"
```

---

## Task 2 — PluginProcessor: smooth master volume

**Files:**
- Modify: `Source/PluginProcessor.h`
- Modify: `Source/PluginProcessor.cpp`

- [ ] **Step 1: Add smoother member to PluginProcessor.h**

In `PluginProcessor.h`, in the `private:` section, add after `ArpEngine arp;`:

```cpp
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMasterVol;
```

- [ ] **Step 2: Initialise in prepareToPlay()**

In `PluginProcessor.cpp`, at the end of `prepareToPlay()`, add:

```cpp
    smoothMasterVol.reset(sampleRate, 0.005);
    smoothMasterVol.setCurrentAndTargetValue(
        *apvts.getRawParameterValue(ParamIDs::masterVolume));
```

- [ ] **Step 3: Replace applyGain with smoothed ramp in processBlock()**

In `PluginProcessor.cpp`, replace the last two lines of `processBlock()`:

```cpp
    const float masterVol = *apvts.getRawParameterValue(ParamIDs::masterVolume);
    buffer.applyGain(masterVol);
```

with:

```cpp
    smoothMasterVol.setTargetValue(*apvts.getRawParameterValue(ParamIDs::masterVolume));
    const float gainStart = smoothMasterVol.getCurrentValue();
    smoothMasterVol.skip(buffer.getNumSamples());
    const float gainEnd   = smoothMasterVol.getCurrentValue();
    buffer.applyGainRamp(0, buffer.getNumSamples(), gainStart, gainEnd);
```

- [ ] **Step 4: Build and run tests**

```bat
"C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" --build build --config Release --target AISynthTests
./build/AISynthTests_artefacts/Release/AISynthTests.exe
```

Expected: all tests pass, exit code 0.

- [ ] **Step 5: Commit**

```bash
git add Source/PluginProcessor.h Source/PluginProcessor.cpp
git commit -m "feat: smooth master volume in PluginProcessor (I-4)"
```

---

## Task 3 — Delay: smooth mix

**Files:**
- Modify: `Source/FX/Delay.h`
- Modify: `Source/FX/Delay.cpp`

- [ ] **Step 1: Add smoothMix to Delay.h**

In `Delay.h`, in the `private:` section, add after `Params params;`:

```cpp
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMix;
```

- [ ] **Step 2: Initialise in Delay::prepare()**

In `Delay.cpp`, at the end of `prepare()`, add:

```cpp
    smoothMix.reset(sr, 0.005);
    smoothMix.setCurrentAndTargetValue(0.f);
```

- [ ] **Step 3: Set target in Delay::setParams()**

In `Delay.cpp`, at the end of `setParams()`, add:

```cpp
    smoothMix.setTargetValue(p.mix);
```

- [ ] **Step 4: Advance smoother in Delay::process()**

In `Delay.cpp`, replace the entire `process()` function:

```cpp
void Delay::process(juce::AudioBuffer<float>& buffer)
{
    if (!smoothMix.isSmoothing() && smoothMix.getTargetValue() < 0.0001f)
    {
        smoothMix.skip(buffer.getNumSamples());
        return;
    }

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    float* chL = buffer.getWritePointer(0);
    float* chR = numChannels > 1 ? buffer.getWritePointer(1) : chL;

    const float fb = params.feedback;

    for (int i = 0; i < numSamples; ++i)
    {
        const float wet = smoothMix.getNextValue();
        const float dry = 1.f - wet;

        const float dryL = chL[i];
        const float dryR = chR[i];

        const float wetL = readInterpolated(bufL, delaySamples);
        const float wetR = readInterpolated(bufR, delaySamples);

        bufL[writePos] = std::tanh(dryL + wetL * fb);
        bufR[writePos] = std::tanh(dryR + wetR * fb);

        chL[i] = dry * dryL + wet * wetL;
        if (chR != chL)
            chR[i] = dry * dryR + wet * wetR;

        writePos = (writePos + 1) % bufSize;
    }
}
```

- [ ] **Step 5: Build and run tests**

```bat
"C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" --build build --config Release --target AISynthTests
./build/AISynthTests_artefacts/Release/AISynthTests.exe
```

Expected: exit code 0.

- [ ] **Step 6: Commit**

```bash
git add Source/FX/Delay.h Source/FX/Delay.cpp
git commit -m "feat: smooth Delay mix (I-4)"
```

---

## Task 4 — Chorus: smooth mix

**Files:**
- Modify: `Source/FX/Chorus.h`
- Modify: `Source/FX/Chorus.cpp`

- [ ] **Step 1: Add smoothMix to Chorus.h**

In `Chorus.h`, in the `private:` section, add after `Params params;`:

```cpp
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMix;
```

- [ ] **Step 2: Initialise in Chorus::prepare()**

In `Chorus.cpp`, at the end of `prepare()`, add:

```cpp
    smoothMix.reset(sr, 0.005);
    smoothMix.setCurrentAndTargetValue(0.f);
```

- [ ] **Step 3: Set target in Chorus::setParams()**

In `Chorus.cpp`, add after `params = p;`:

```cpp
    smoothMix.setTargetValue(p.mix);
```

- [ ] **Step 4: Advance smoother in Chorus::process()**

In `Chorus.cpp`, replace the entire `process()` function:

```cpp
void Chorus::process(juce::AudioBuffer<float>& buffer)
{
    if (!smoothMix.isSmoothing() && smoothMix.getTargetValue() < 0.001f)
    {
        smoothMix.skip(buffer.getNumSamples());
        return;
    }

    const int   numSamples   = buffer.getNumSamples();
    float*      chL          = buffer.getWritePointer(0);
    float*      chR          = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : chL;
    const float lfoIncrement = static_cast<float>(params.rate / sampleRate);

    for (int i = 0; i < numSamples; ++i)
    {
        const float mix = smoothMix.getNextValue();

        const float lfoL = 0.5f * (1.f + std::sin(juce::MathConstants<float>::twoPi * static_cast<float>(lfoPhase)));
        lfoPhase += lfoIncrement;
        if (lfoPhase >= 1.0) lfoPhase -= 1.0;

        const float lfoR = 0.5f * (1.f + std::sin(juce::MathConstants<float>::twoPi * static_cast<float>(lfoPhaseR)));
        lfoPhaseR += lfoIncrement;
        if (lfoPhaseR >= 1.0) lfoPhaseR -= 1.0;

        const float delaySamplesL = static_cast<float>((0.007 + params.depth * lfoL) * sampleRate);
        const float delaySamplesR = static_cast<float>((0.007 + params.depth * lfoR) * sampleRate);

        bufL[writePos] = chL[i];
        bufR[writePos] = chR[i];

        const float wetL = readInterpolated(bufL, delaySamplesL);
        const float wetR = readInterpolated(bufR, delaySamplesR);

        chL[i] = chL[i] * (1.f - mix) + wetL * mix;
        chR[i] = chR[i] * (1.f - mix) + wetR * mix;

        writePos = (writePos + 1) % bufSize;
    }
}
```

- [ ] **Step 5: Build and run tests**

```bat
"C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" --build build --config Release --target AISynthTests
./build/AISynthTests_artefacts/Release/AISynthTests.exe
```

Expected: exit code 0.

- [ ] **Step 6: Commit**

```bash
git add Source/FX/Chorus.h Source/FX/Chorus.cpp
git commit -m "feat: smooth Chorus mix (I-4)"
```

---

## Task 5 — Drive: smooth mix

Drive's loop is currently channel-outer / sample-inner. The smoother must advance once per sample, not once per (channel × sample). Restructure to sample-outer / channel-inner.

**Files:**
- Modify: `Source/FX/Drive.h`
- Modify: `Source/FX/Drive.cpp`

- [ ] **Step 1: Add smoothMix to Drive.h**

In `Drive.h`, in the `private:` section, add after `float cachedPostGain = 1.f;`:

```cpp
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMix;
```

- [ ] **Step 2: Initialise in Drive::prepare()**

Replace the current `Drive::prepare()` body (currently empty):

```cpp
void Drive::prepare(double sampleRate, int /*maxBlockSize*/)
{
    smoothMix.reset(sampleRate, 0.005);
    smoothMix.setCurrentAndTargetValue(0.f);
}
```

- [ ] **Step 3: Set target in Drive::setParams()**

In `Drive.cpp`, at the end of `setParams()`, add:

```cpp
    smoothMix.setTargetValue(p.mix);
```

- [ ] **Step 4: Replace process() with sample-outer loop**

In `Drive.cpp`, replace the entire `process()` function:

```cpp
void Drive::process(juce::AudioBuffer<float>& buffer)
{
    if (!smoothMix.isSmoothing() && smoothMix.getTargetValue() < 0.001f)
    {
        smoothMix.skip(buffer.getNumSamples());
        return;
    }

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        const float mix = smoothMix.getNextValue();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data      = buffer.getWritePointer(ch);
            const float dry  = data[i];
            const float wet  = processSample(dry, params.type, cachedGain) * cachedPostGain;
            data[i] = dry * (1.f - mix) + wet * mix;
        }
    }
}
```

- [ ] **Step 5: Build and run tests**

```bat
"C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" --build build --config Release --target AISynthTests
./build/AISynthTests_artefacts/Release/AISynthTests.exe
```

Expected: exit code 0.

- [ ] **Step 6: Commit**

```bash
git add Source/FX/Drive.h Source/FX/Drive.cpp
git commit -m "feat: smooth Drive mix, restructure to sample-outer loop (I-4)"
```

---

## Task 6 — Reverb: smooth mix

`juce::Reverb::processStereo()` takes the whole block — per-sample blending requires a dry copy. Pre-allocate a dry buffer in `prepare()`. Change `setParams()` to always pass `wetLevel=1, dryLevel=0` so juce::Reverb outputs 100% wet; blend manually in `process()`.

**Files:**
- Modify: `Source/FX/Reverb.h`
- Modify: `Source/FX/Reverb.cpp`

- [ ] **Step 1: Add smoothMix and dryBuf to Reverb.h**

In `Reverb.h`, in the `private:` section, add after `Params params;`:

```cpp
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMix;
    juce::AudioBuffer<float> dryBuf;  // pre-allocated dry copy, avoids audio-thread alloc
```

- [ ] **Step 2: Initialise in Reverb::prepare()**

Replace the entire `Reverb::prepare()` in `Reverb.cpp`:

```cpp
void Reverb::prepare(double sampleRate, int maxBlockSize)
{
    reverb.setSampleRate(sampleRate);
    reverb.reset();
    smoothMix.reset(sampleRate, 0.005);
    smoothMix.setCurrentAndTargetValue(0.f);
    dryBuf.setSize(2, maxBlockSize, false, true, false);
}
```

- [ ] **Step 3: Update setParams() to run reverb full-wet**

Replace the entire `Reverb::setParams()` in `Reverb.cpp`:

```cpp
void Reverb::setParams(const Params& p)
{
    params = p;
    smoothMix.setTargetValue(p.mix);
    juce::Reverb::Parameters rp;
    rp.roomSize   = p.room;
    rp.damping    = p.damp;
    rp.wetLevel   = 1.f;   // dry/wet blended manually in process() for smooth mix
    rp.dryLevel   = 0.f;
    rp.width      = 1.f;
    rp.freezeMode = 0.f;
    reverb.setParameters(rp);
}
```

- [ ] **Step 4: Replace process() with manual dry/wet blend**

Replace the entire `Reverb::process()` in `Reverb.cpp`:

```cpp
void Reverb::process(juce::AudioBuffer<float>& buffer)
{
    if (!smoothMix.isSmoothing() && smoothMix.getTargetValue() < 0.001f)
    {
        smoothMix.skip(buffer.getNumSamples());
        return;
    }

    const int numSamples = buffer.getNumSamples();

    // Copy dry signal before reverb overwrites the buffer
    dryBuf.copyFrom(0, 0, buffer, 0, 0, numSamples);
    if (buffer.getNumChannels() >= 2)
        dryBuf.copyFrom(1, 0, buffer, 1, 0, numSamples);

    // Process at full wet
    if (buffer.getNumChannels() >= 2)
        reverb.processStereo(buffer.getWritePointer(0),
                             buffer.getWritePointer(1),
                             numSamples);
    else
        reverb.processMono(buffer.getWritePointer(0), numSamples);

    // Blend dry + wet per-sample using smoothed mix
    float* outL       = buffer.getWritePointer(0);
    const float* dryL = dryBuf.getReadPointer(0);
    float* outR       = buffer.getNumChannels() >= 2 ? buffer.getWritePointer(1) : nullptr;
    const float* dryR = buffer.getNumChannels() >= 2 ? dryBuf.getReadPointer(1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        const float mix = smoothMix.getNextValue();
        outL[i] = dryL[i] * (1.f - mix) + outL[i] * mix;
        if (outR) outR[i] = dryR[i] * (1.f - mix) + outR[i] * mix;
    }
}
```

- [ ] **Step 5: Build VST3 and run tests**

```bat
"C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" --build build --config Release --target AISynth_VST3
"C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" --build build --config Release --target AISynthTests
./build/AISynthTests_artefacts/Release/AISynthTests.exe
```

Expected: VST3 built at `build/AISynthTests_artefacts/Release/VST3/AI Synth.vst3`, tests exit code 0.

- [ ] **Step 6: Commit**

```bash
git add Source/FX/Reverb.h Source/FX/Reverb.cpp
git commit -m "feat: smooth Reverb mix with pre-allocated dry buffer (I-4)"
```

---

## Self-Review

**Spec coverage:**
- Voice: filterCutoff, filterResonance, osc1Level, osc2Level → Task 1 ✓
- PluginProcessor: masterVol → Task 2 ✓
- Delay mix → Task 3 ✓
- Chorus mix → Task 4 ✓
- Drive mix → Task 5 ✓
- Reverb mix → Task 6 ✓

**Placeholder scan:** None found — all steps contain full code.

**Type consistency:**
- `smoothMix` used consistently across all FX tasks.
- `smoothCutoff`, `smoothResonance`, `smoothOsc1Level`, `smoothOsc2Level` declared in Task 1 Step 1 and used in Steps 2–4.
- `dryBuf` declared in Task 6 Step 1, sized in Step 2, read in Step 4.
- `Drive::prepare()` was an empty stub — Task 5 Step 2 gives it a body; the signature `void Drive::prepare(double sampleRate, int maxBlockSize)` matches `Drive.h`.

**Architecture note:** Reverb is the only unit where the juce::Reverb wet/dry API is bypassed. The change is intentional and documented in the commit message. Room and damp are still passed through correctly.
