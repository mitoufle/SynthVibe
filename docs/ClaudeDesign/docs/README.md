# SynthVibe — Design Handoff

Bundle of specs for implementing the HTML mockup (`AISynth V1 Hi-Fi.html`) in the JUCE C++ plugin.

## Read in this order

1. **`param-audit.md`** — every parameter, its ID, type, range, default. Start of all wiring.
2. **`ai-contract.md`** — how Claude talks to the plugin (system prompt + JSON schema).
3. **`preset-format.md`** — on-disk layout of `.synthvibe` and the JSON sidecar.
4. **`responsive.md`** — 3 breakpoints and reflow rules.
5. **`components.md`** — every custom component, its states, and suggested file paths.
6. **`../Source/UI/DesignTokens.h`** — colors, spacing, radii, motion timings as C++ constants.

## Pointing Claude Code at this

When you hand this to Claude Code, ask it to:
1. Read `AISynth V1 Hi-Fi.html` for the visual spec.
2. Read all files in `docs/` for the textual spec.
3. Read `Source/UI/DesignTokens.h` for the color/spacing palette.
4. Start with **Phase 1** in the implementation roadmap (LookAndFeel + top bar + one knob).

## Phases

1. **Foundation** — LookAndFeel, tokens, top bar, first ArcKnob.
2. **Sound tab** — OSC panels, filter, envelopes, oscilloscope wiring.
3. **Mod / FX / Arp** — matrix, chain, sequencer.
4. **AI layer** — modal, Claude integration, apply/tween, history.
5. **Polish** — micro-interactions, responsiveness, MIDI learn.

See the main chat summary for time estimates per phase.
