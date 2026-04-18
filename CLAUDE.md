# SynthVibe

## What
C++17 JUCE VST3 synthesizer plugin. Polyphonic synth engine with an AI-assisted sound design layer planned (Claude API + MCP). The AI layer is not yet built — current work is on the synth engine and UI.

Repo: https://github.com/mitoufle/SynthVibe

## Why
Let musicians describe sounds in plain language instead of tweaking knobs. The synth translates natural language prompts into synthesizer parameters via the Claude API. `VoiceParams` (Source/Engine/Voice.h) is the bridge struct between the AI layer and the audio engine. `PresetManager` (Source/Presets/PresetManager.h) will persist and recall AI-generated sounds.

## Implementation status

| Module | Status |
|---|---|
| Oscillator (Sine/Saw/Square/Triangle + PolyBLEP) | Done |
| Envelope (amp + filter, ADSR) | Done |
| Filter (LP/HP/BP) + filter envelope | Done |
| Voice (monophonic unit) | Done |
| SynthEngine (8-voice polyphony, MIDI) | Done |
| Delay FX | Done |
| PresetManager (save/load to disk) | Done |
| PluginProcessor + APVTS | Done |
| PluginEditor / UI | In progress |
| FXChain (Reverb, Chorus…) | Stub — `Source/FX/FXChain.h`, CMakeLists entry commented out |
| APIServer (HTTP/MCP layer) | Stub — `Source/API/APIServer.h`, CMakeLists entry commented out |

## Audio signal flow
```
PluginProcessor::processBlock
  → SynthEngine::processBlock
      → Voice[0..7]::getNextSample   (Oscillator → Filter → AmpEnvelope)
  → Delay::process
```
- 8 voices max (`SynthEngine::NumVoices`). Change to a power of 2.
- `apvts` is public on `AISynthProcessor` — editor and AI layer both read/write it.
- `buildVoiceParams()` in PluginProcessor.cpp is the APVTS → VoiceParams conversion point.

## Parameters (all IDs in Source/Parameters/ParameterIDs.h)

| Group | IDs |
|---|---|
| Oscillator | `osc_waveform`, `osc_detune`, `osc_octave` |
| Filter | `filter_type`, `filter_cutoff`, `filter_resonance`, `filter_env_amt` |
| Amp envelope | `amp_attack`, `amp_decay`, `amp_sustain`, `amp_release` |
| Filter envelope | `flt_attack`, `flt_decay`, `flt_sustain`, `flt_release` |
| Master | `master_volume` |
| Delay | `delay_time`, `delay_feedback`, `delay_mix` |
| Future (commented out) | `lfo_rate`, `lfo_depth`, `reverb_mix` |

## How

### Build
```bat
build.bat       # CMake configure + compile (run from AISynth/)
deploy.bat      # copy VST3 to C:\Program Files\Common Files\VST3\
```
JUCE 7.0.9 is fetched automatically by CMake on first build. Generator: Visual Studio 18 2022.

### Output path
```
build\AISynth_artefacts\Release\VST3\AI Synth.vst3
```

### Slash commands
- `/deploy-vst` — runs `deploy.bat` and confirms the file is in place

### Extend
- **New parameter**: `ParameterIDs.h` → `ParameterLayout.cpp` → read in `PluginProcessor::buildVoiceParams()` → add `KnobWithLabel` in editor
- **New waveform**: extend `Waveform` enum in `Engine/Oscillator.h`, add case in `getNextSample()`
- **New FX**: follow `Source/FX/Delay` pattern (prepare / setParams / process / reset), call after delay in `processBlock`, then wire into `FXChain`
- **Activate FXChain**: uncomment `Source/API/FXChain.cpp` in CMakeLists, implement `FXChain::process`
- **Activate APIServer**: uncomment `Source/API/APIServer.cpp` in CMakeLists, implement HTTP layer

### Key files
- `Source/Parameters/ParameterIDs.h` — all param ID strings, never hardcode elsewhere
- `Source/Engine/Voice.h` — `VoiceParams` struct, the APVTS→engine contract (and future AI→engine contract)
- `Source/UI/LookAndFeel.h` — all UI colours as constants, never use raw literals
- `Source/API/APIServer.h` — stub for the future HTTP REST layer (MCP integration point)
- `Source/Presets/PresetManager.h` — save/load presets; AI layer will call this to persist generated sounds
