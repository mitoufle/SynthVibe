# UI Redesign — Design Spec
Date: 2026-04-18

## Goal

Reorganize the SynthVibe plugin UI from the current 4-row flat layout to a tabbed layout with larger window, persistent top/bottom bars, and roomier sections. No AI prompt bar, no preset browser — this spec covers layout only.

## Window

- **Size:** 1200 × 750 px, fixed (non-resizable)
- **Current:** 900 × 560 px

## Global Structure

```
┌──────────────────────────────────────────────────────────────┐
│ TOP BAR  │ "AI Synth" logo  ◀ [Preset Name] ▶  [Save][Load]  Vol○ │  ~36px
├──────────────────────────────────────────────────────────────┤
│ TAB BAR  │ [SOUND]  MOD  FX  ARP                             │  ~28px
├──────────────────────────────────────────────────────────────┤
│                                                              │
│  TAB CONTENT AREA                          (~650px tall)     │
│                                                              │
├──────────────────────────────────────────────────────────────┤
│ BOTTOM BAR  │  Voices: ████░░░░ 4/8     CPU: 12%            │  ~24px
└──────────────────────────────────────────────────────────────┘
```

Total: 36 + 28 + 650 + 24 + margins ≈ 750px.

## Top Bar

Always visible, never changes between tabs.

| Element | Detail |
|---|---|
| Logo label | "AI Synth" in `colHighlight`, bold, left-aligned |
| Prev/Next arrows | ◀ ▶ buttons to cycle through presets |
| Preset name | Editable label or read-only text centred |
| Save / Load buttons | Two small buttons wired to `PresetManager` |
| Master volume knob | Small rotary, rightmost, param `master_volume` |

## Bottom Bar

Always visible. Read-only status strip.

| Element | Detail |
|---|---|
| Voice activity | Bar showing active voices out of 8 (e.g. `████░░░░ 4/8`) |
| CPU meter | Text label updated via `juce::Timer` |

## Tab Bar

Four tabs: **SOUND · MOD · FX · ARP**. Active tab highlighted in `colHighlight`. Tabs switch the content area; top and bottom bars stay put.

## Tab: SOUND

Three columns side by side, full content area height.

```
┌────────────────┬────────────────┬──────────────────────┐
│    OSC 1       │    OSC 2       │  FILTER              │
│  [Waveform▼]  │  [Waveform▼]  │  [Type▼]             │
│  Oct  Semi     │  Oct  Semi     │  Cutoff  Res  EnvAmt │
│  Detune Level  │  Detune Level  ├──────────────────────┤
│                │                │  AMP ENV             │
│  (col fills    │  (col fills    │  A  D  S  R          │
│   full height) │   full height) ├──────────────────────┤
│                │                │  FLT ENV             │
│                │                │  A  D  S  R          │
└────────────────┴────────────────┴──────────────────────┘
```

- Col 1 (OSC 1) and Col 2 (OSC 2): equal width, full height. Each contains waveform ComboBox + 4 knobs (Oct, Semi, Detune, Level).
- Col 3: Filter panel on top, AmpEnv panel in middle, FltEnv panel at bottom, stacked vertically.
- All panels use `SynthSection` (rounded rect, section-accent border, title label).

## Tab: MOD

LFO 1 and LFO 2 side by side, each taking half the content area width and full height.

Each LFO panel contains:
- Shape ComboBox (`lfo1_shape` / `lfo2_shape`)
- Destination ComboBox (`lfo1_dest` / `lfo2_dest`)
- Rate knob (`lfo1_rate` / `lfo2_rate`)
- Depth knob (`lfo1_depth` / `lfo2_depth`)

## Tab: FX

Delay and Chorus side by side, each half the content area width.

**Delay:** Time, Feedback, Mix knobs.  
**Chorus:** Rate, Depth, Mix knobs.

## Tab: ARP

Single panel, centred or full-width. Contains:
- On/Off ToggleButton (`arp_enabled`)
- Mode ComboBox (`arp_mode`: Up / Down / UpDown / Random)
- Rate ComboBox (`arp_rate`: 1/16 / 1/8 / 1/4 / 1/2)
- Octaves knob (`arp_octave_range`)

## Visual Style

| Aspect | Spec |
|---|---|
| Section panels | `SynthSection` component — rounded rect (6px radius), 1px accent border, uppercase title 11px bold |
| Knob size | ~48px (rotary diameter), up from ~38px current |
| Knob label font | 11px bold (up from 10px) |
| Colour palette | Unchanged — `LookAndFeel.h` constants (`colBackground`, `colPanel`, per-section accent colours) |
| Tab active state | Background fill `colHighlight`, text white; inactive tabs: text `colTextDim` |
| Top/bottom bars | Background `colAccent` (`0xFF0F3460`) |

## What Does NOT Change

- Audio engine, all JUCE parameters, all `ParamIDs` — untouched
- `KnobWithLabel` component — keep as-is, adjust size via `setBounds`
- `SynthLookAndFeel` colour constants — no new colours added
- `SynthSection` component — used as-is

## Implementation Constraints

- All param IDs stay in `ParameterIDs.h` — no hardcoding in editor
- `AISynthEditor` will grow; consider splitting tab content into sub-components (e.g. `SoundTab`, `ModTab`) for maintainability
- Preset save/load top-bar buttons wire into existing `PresetManager`
- Bottom bar CPU/voice meter requires a `juce::Timer` in the editor

## Out of Scope

- AI prompt bar / Claude API integration
- Preset browser (list/search)
- LFO visualizer waveform display
- Resizable window
