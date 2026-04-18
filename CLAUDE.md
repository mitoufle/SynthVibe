# SynthVibe — AI-Assisted Synthesizer VST

## Project Goal
A polyphonic synthesizer VST plugin built with JUCE/C++. The long-term goal is AI-assisted sound design: the user describes a sound in plain language and the synth configures itself accordingly. The AI layer (Claude API + MCP server) is not yet implemented — the current focus is building a high-quality, extensible synth engine first.

## Stack
- **Language**: C++17
- **Framework**: JUCE 7.0.9 (fetched via CMake FetchContent)
- **Build**: CMake + Visual Studio 18 2026 (Windows)
- **Formats**: VST3 + Standalone
- **Repo**: https://github.com/mitoufle/SynthVibe

## Build & Deploy
```bat
build.bat       # configure + compile (run from AISynth/)
deploy.bat      # copy VST3 to C:\Program Files\Common Files\VST3\
```
`build/` is gitignored. JUCE is downloaded automatically on first build.

## Architecture

```
Source/
├── Parameters/
│   ├── ParameterIDs.h        # all APVTS param ID constants — add new ones here
│   └── ParameterLayout.cpp   # declares all parameter ranges/defaults
├── Engine/
│   ├── Oscillator            # PolyBLEP antialiased: Sine, Saw, Square, Triangle
│   ├── Envelope              # ADSR — used for both amp and filter modulation
│   ├── Filter                # JUCE StateVariableTPTFilter: LP, HP, BP
│   ├── Voice                 # 1 voice = osc + filter + 2 envelopes
│   └── SynthEngine           # 8-voice polyphony, MIDI dispatch, voice stealing
├── FX/
│   ├── Delay                 # stereo delay, tanh feedback, wet/dry mix
│   └── FXChain.h             # stub for future effects (reverb, chorus, etc.)
├── Presets/
│   └── PresetManager         # save/load named presets to ~/Documents/AISynth/Presets/
├── UI/
│   ├── LookAndFeel.h         # dark theme — all colours defined here as constants
│   └── SynthSection.h        # base class for future UI panels
├── API/
│   └── APIServer.h           # stub: local HTTP server for MCP/AI integration
├── PluginProcessor           # AudioProcessor, APVTS, sample-accurate MIDI, state persistence
└── PluginEditor              # UI: knobs with live value display, section layout
```

## Current Features
- 4 waveforms with PolyBLEP antialiasing
- State-variable filter (LP/HP/BP) with resonance and filter envelope
- Amp ADSR + Filter ADSR
- 8-voice polyphony with voice stealing
- Stereo delay (time, feedback, mix)
- All knobs show live values; double-click to type exact values
- Preset save/load to disk
- Full DAW state persistence

## Extending the Synth

### Add a new parameter
1. Add ID to `Source/Parameters/ParameterIDs.h`
2. Add range/default to `Source/Parameters/ParameterLayout.cpp`
3. Read it in `PluginProcessor::processBlock` and pass to the relevant engine class
4. Add a `KnobWithLabel` in `PluginEditor`

### Add a new oscillator waveform
- Extend `enum class Waveform` in `Engine/Oscillator.h`
- Add a case in `Oscillator::getNextSample()`
- Add the option to `oscWaveformBox` in `PluginEditor.cpp`

### Add a new FX
- Implement in `Source/FX/` following the `Delay` pattern (prepare / setParams / process / reset)
- Add params to `ParameterIDs.h` + `ParameterLayout.cpp`
- Call from `PluginProcessor::processBlock` after the existing delay call
- Add knobs in the editor delay row or a new row

## Planned: AI Layer
- `APIServer.h` will become a local HTTP REST server (cpp-httplib)
- Endpoints: `GET /params`, `POST /params`, `GET /presets`, `POST /presets/load`
- A Python MCP server will expose these as Claude tools
- A prompt text box in the UI will call the Claude API directly
- `VoiceParams` is the bridge struct — the AI translates natural language → `VoiceParams`

## Conventions
- All parameter string IDs live only in `ParameterIDs.h` — never hardcode them elsewhere
- `VoiceParams` is the single struct that flows from APVTS → engine → voices
- `KnobWithLabel` takes a suffix string and decimal count — always pass these for readable values
- UI colours are all in `SynthLookAndFeel` constants — never use raw colour literals in layout code
