# SynthVibe — Responsive Strategy

DAW hosts can resize plugin windows freely. We support 3 named breakpoints + continuous scaling within each range. The design target in the mockup is **Default**.

---

## Breakpoints

| Name | Window size | Purpose |
|---|---|---|
| **Compact** | 960 × 560 | Laptop + narrow DAW layouts |
| **Default** | 1280 × 720 | Primary design target |
| **Wide** | 1680 × 900 | Large monitors / dual-screen DAW |

Window has a minimum of **900 × 520** (below this, some panels clip gracefully rather than reflowing further). Maximum is `2400 × 1400` with proportional scaling beyond Wide.

### Switching rules

- `< 1150px` wide → Compact layout
- `>= 1150px and < 1550px` → Default
- `>= 1550px` → Wide

Height scaling is proportional; layouts maintain ~16:9.

---

## Layout differences

### Compact (960 × 560)

- OSC 1 / OSC 2 stack vertically (as in mockup).
- Filter / Amp Env / Filter Env stack vertically on the right.
- Knob size **38px** (down from 46px).
- Panel padding **10px** (down from 14px).
- Top bar collapses: brand + preset name + primary actions only; master vol hidden behind a context menu on the master section.
- Tab bar: icon-only with tooltips (no text labels).
- Bottom keyboard: **3 octaves** visible (down from 4).
- Mod Matrix: 5 rows visible, scrollable.
- FX Chain: slots shrink to 2 columns × N rows wrap.

### Default (1280 × 720) — mockup matches this exactly

- Current mockup layout.
- Knob 46px, panel padding 14px.
- 4 octaves of keys.

### Wide (1680 × 900)

- OSC 1 and OSC 2 side-by-side (not stacked).
- Filter + Amp Env + Flt Env in a 3-column row on the right.
- Knob size **52px**.
- Envelope viz gets more width.
- 5 octaves of keys.
- Mod Matrix: 10 rows visible.
- FX Chain: 5 or 6 slots in a row, additional slots in a second row.
- AI modal: variations shown as 4-wide instead of 2×2.

---

## Implementation notes

- Use `juce::Component::resized()` with a breakpoint check at the top:
  ```cpp
  enum class BP { Compact, Default, Wide };
  BP currentBP = width < 1150 ? BP::Compact : width < 1550 ? BP::Default : BP::Wide;
  ```
- Each major panel gets `layoutFor(BP)` so the breakpoint only lives in the root editor.
- Use `juce::FlexBox` for row/column primitives, `juce::Grid` for the knob arrays.
- Font sizes scale via a single `getScale()` helper (1.0 at Default, 0.85 at Compact, 1.15 at Wide).
- Knob size is driven by a shared `constexpr int kKnobSize[3]` array.
- Keyboard octave count changes re-layout entirely; don't try to mid-resize.

## What NOT to reflow

- **The tab structure** (Sound / Mod / FX / Arp) is always identical across breakpoints.
- **The preset name text** never moves; it anchors the brand area.
- **Envelope node positions** are relative to the envelope viz bounds — nodes don't have breakpoint-specific positions.

## Default plugin window size

- First launch: **1280 × 720** (Default).
- Persisted in `PropertiesFile` under `window.width` / `window.height`.
- Restored on next open.

## Print / screenshot target (for docs, etc.)

Use **Default (1280 × 720)** — match the HTML mockup.
