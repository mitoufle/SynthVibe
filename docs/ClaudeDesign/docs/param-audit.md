# SynthVibe — Parameter Audit

Every UI control in the mockup mapped to an `AudioProcessorValueTreeState` parameter.
IDs are stable — don't rename after first public release or user presets break.

**Conventions**
- IDs use dot-separated lowercase: `section.name[.sub]`
- Normalized floats in [0, 1] unless marked otherwise; DAW automation uses these.
- Display/units are UI concerns (applied via `AudioParameterFloat::AttributedString` or formatter lambda).
- `skew` = JUCE `NormalisableRange::skew`. `0.5` = logarithmic (for freq), `1.0` = linear.
- `auto` = exposed to DAW automation.

---

## Global

| id | label | type | range | default | unit | skew | auto |
|---|---|---|---|---|---|---|---|
| `master.vol` | Master Volume | float | 0–1 | 0.8 | — | 1.0 | yes |
| `global.voices` | Max Voices | int | 1–8 | 8 | — | — | no |

## OSC 1 / OSC 2 (repeat prefix `osc1.` / `osc2.`)

| id suffix | label | type | range | default | unit | skew | auto |
|---|---|---|---|---|---|---|---|
| `wave` | Wave | choice | {saw,square,triangle,sine,noise} | saw (osc1) / square (osc2) | — | — | yes |
| `level` | Level | float | 0–1 | 0.8 | — | 1.0 | yes |
| `tune` | Tune | float | -24..+24 | 0 | semitones | 1.0 | yes |
| `fine` | Fine | float | -100..+100 | 0 | cents | 1.0 | yes |
| `phase` | Phase | float | 0..360 | 0 | deg | 1.0 | yes |
| `pwm` | Pulse Width | float | 0.02–0.98 | 0.5 | — | 1.0 | yes |
| `uni.voices` | Unison Voices | int | 1–8 | 1 | — | — | yes |
| `uni.detune` | Unison Detune | float | 0–1 | 0.2 | — | 1.0 | yes |
| `uni.spread` | Unison Spread | float | 0–1 | 0.5 | — | 1.0 | yes |

## Filter

| id | label | type | range | default | unit | skew | auto |
|---|---|---|---|---|---|---|---|
| `filt.type` | Filter Type | choice | {lp24,lp12,hp24,hp12,bp,notch} | lp24 | — | — | yes |
| `filt.cutoff` | Cutoff | float | 20–20000 | 1200 | Hz | 0.3 | yes |
| `filt.res` | Resonance | float | 0–1 | 0.2 | — | 1.0 | yes |
| `filt.drive` | Drive | float | 0–1 | 0 | — | 1.0 | yes |
| `filt.keytrack` | Keytrack | float | -1..+1 | 0 | — | 1.0 | yes |
| `filt.envamt` | Env Amount | float | -1..+1 | 0.4 | — | 1.0 | yes |

## Amp Envelope

| id | label | type | range | default | unit | skew | auto |
|---|---|---|---|---|---|---|---|
| `env.amp.attack` | Attack | float | 0.001–10 | 0.005 | s | 0.3 | yes |
| `env.amp.decay` | Decay | float | 0.001–10 | 0.2 | s | 0.3 | yes |
| `env.amp.sustain` | Sustain | float | 0–1 | 0.75 | — | 1.0 | yes |
| `env.amp.release` | Release | float | 0.001–15 | 0.4 | s | 0.3 | yes |

## Filter Envelope

| id | label | type | range | default | unit | skew | auto |
|---|---|---|---|---|---|---|---|
| `env.filt.attack` | Attack | float | 0.001–10 | 0.01 | s | 0.3 | yes |
| `env.filt.decay` | Decay | float | 0.001–10 | 0.3 | s | 0.3 | yes |
| `env.filt.sustain` | Sustain | float | 0–1 | 0.4 | — | 1.0 | yes |
| `env.filt.release` | Release | float | 0.001–15 | 0.5 | s | 0.3 | yes |

## LFO 1 / LFO 2 (prefix `lfo1.` / `lfo2.`)

| id suffix | label | type | range | default | unit | skew | auto |
|---|---|---|---|---|---|---|---|
| `wave` | Wave | choice | {sine,triangle,saw,square,s&h} | sine / s&h | — | — | yes |
| `rate` | Rate | float | 0.01–40 | 1.0 | Hz | 0.3 | yes |
| `rate.sync` | Sync Rate | choice | {1/1,1/2,1/4,1/8,1/16,1/32,1/4T,1/8T,1/16T} | 1/4 | — | — | yes |
| `sync.on` | Tempo Sync | bool | — | false | — | — | yes |
| `depth` | Depth | float | 0–1 | 0.5 | — | 1.0 | yes |
| `phase` | Phase | float | 0–360 | 0 | deg | 1.0 | yes |
| `fade` | Fade In | float | 0–5 | 0 | s | 0.4 | yes |
| `keytrigger` | Key Trigger | bool | — | false | — | — | yes |

## Modulation Matrix — 8 slots (`mod.N.*`, N = 1..8)

| id suffix | label | type | range | default | unit | auto |
|---|---|---|---|---|---|---|
| `src` | Source | choice | {none,lfo1,lfo2,envAmp,envFilt,velocity,modwheel,aftertouch,keytrack,random} | none | — | yes |
| `dst` | Destination | choice | *(curated list of ~30 targets; see paramId index below)* | none | — | yes |
| `amount` | Amount | float | -1..+1 | 0 | — | yes |
| `curve` | Curve | choice | {lin,exp,log,s} | lin | — | no |

> **Scrollable panel:** UI prepared to show 8 visible; architecture supports growing the slot count later by changing a single constant. Parameter IDs are allocated statically up to `mod.16.*` to avoid breaking presets when we expand.

## FX Chain — up to 10 serial slots (`fx.N.*`, N = 1..10)

| id suffix | label | type | range | default | auto |
|---|---|---|---|---|---|
| `type` | Effect Type | choice | {none,drive,chorus,delay,reverb,eq3,comp,phaser,flanger,bitcrush,filter} | none | yes |
| `bypass` | Bypass | bool | — | false | yes |
| `mix` | Mix | float | 0–1 | 1.0 | yes |
| `p1` | Param 1 | float | 0–1 | 0.5 | yes |
| `p2` | Param 2 | float | 0–1 | 0.5 | yes |
| `p3` | Param 3 | float | 0–1 | 0.5 | yes |
| `p4` | Param 4 | float | 0–1 | 0.5 | yes |

> **Generic slot model:** each FX slot has 4 generic param knobs. The *meaning* of p1..p4 depends on `type` (drive: amt/tone/mix/unused; delay: time/fback/width/mix; etc). Labels + skew come from a lookup table in code — DAW still sees stable `fx.3.p1` IDs, but the UI relabels them. This avoids needing 10 × 10 × 4 = 400 distinct parameter IDs.

## Arp + Sequencer (`arp.*`)

| id | label | type | range | default | unit | auto |
|---|---|---|---|---|---|---|
| `arp.on` | Arp On | bool | — | false | — | yes |
| `arp.pattern` | Pattern | choice | {up,down,updn,dnup,rand,asplayed,chord} | updn | — | yes |
| `arp.rate` | Rate | choice | {1/4,1/8,1/16,1/16T,1/32} | 1/16 | — | yes |
| `arp.octaves` | Octaves | int | 1–4 | 2 | — | yes |
| `arp.gate` | Gate | float | 0.05–1 | 0.58 | — | yes |
| `arp.swing` | Swing | float | 0–1 | 0.22 | — | yes |
| `arp.humanize` | Humanize | float | 0–1 | 0.4 | — | yes |
| `arp.latch` | Latch | bool | — | false | — | yes |

### Sequencer — 16 steps (`seq.N.*`, N = 1..16)

| id suffix | label | type | range | default | auto |
|---|---|---|---|---|---|
| `gate` | Gate On | bool | — | per pattern | yes |
| `pitch` | Pitch Offset | int | 0–24 | 0 | yes (semitones) |
| `vel` | Velocity | float | 0–1 | 0.8 | yes |
| `tie` | Tie | bool | — | false | yes |

---

## Summary counts

| Section | Automatable params |
|---|---|
| Global | 1 |
| OSC 1 + OSC 2 | 18 |
| Filter | 6 |
| Envelopes (Amp + Filt) | 8 |
| LFO 1 + LFO 2 | 14 |
| Mod Matrix (8 × 4, reserving 16 slots for future) | 32–64 |
| FX Chain (10 × 7) | 70 |
| Arp + Seq (16 × 4) | 72 |
| **Total (usable now)** | **~221** |

This is well within JUCE's recommended upper bound (~500) and acceptable to most DAWs.

## Notes for implementation

1. Use `juce::AudioProcessorValueTreeState::ParameterLayout` factory — declare all params in one place.
2. Group related params with `AudioProcessorParameterGroup` for host param tree browsing ("OSC 1" → Level, Tune, etc).
3. Mod matrix destination is a `juce::AudioParameterChoice` — maintain a single `kDestinations[]` table that pairs choice index → paramId string. The mod engine looks up the target by ID at runtime.
4. Reserve IDs for future-proofing: declare `mod.9..16.*` now even though the UI shows 8, so adding 8 more slots later doesn't require preset migration.
