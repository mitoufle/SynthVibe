# SynthVibe

## What
C++17 JUCE VST3 synthesizer plugin. Polyphonic synth engine with an AI-assisted sound design layer planned (Claude API + MCP). The AI layer is not yet built — current work is on the synth engine and UI.

Repo: https://github.com/mitoufle/SynthVibe

## Why
Let musicians describe sounds in plain language instead of tweaking knobs. The synth translates natural language prompts into synthesizer parameters via the Claude API. `VoiceParams` (Source/Engine/Voice.h) is the bridge struct between the AI layer and the audio engine.

## How

### Build
```bat
build.bat       # CMake configure + compile (run from AISynth/)
deploy.bat      # copy VST3 to C:\Program Files\Common Files\VST3\
```
JUCE 7.0.9 is fetched automatically by CMake on first build. Generator: Visual Studio 18 2026.

### Extend
- **New parameter**: `ParameterIDs.h` → `ParameterLayout.cpp` → read in `PluginProcessor::buildVoiceParams()` → add `KnobWithLabel` in editor
- **New waveform**: extend `Waveform` enum in `Engine/Oscillator.h`, add case in `getNextSample()`
- **New FX**: follow `Source/FX/Delay` pattern (prepare / setParams / process / reset), call after delay in `processBlock`

### Key files
- `Source/Parameters/ParameterIDs.h` — all param ID strings, never hardcode elsewhere
- `Source/Engine/Voice.h` — `VoiceParams` struct, the APVTS→engine contract
- `Source/UI/LookAndFeel.h` — all UI colours as constants, never use raw literals
- `Source/API/APIServer.h` — stub for the future HTTP REST layer (MCP integration point)
