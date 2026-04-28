# Phase B — Wavetable oscillator with curated bank

**Date**: 2026-04-28
**Branch (planned)**: `phase-b-wavetable`
**Driver**: After A.2 ship, manual smoke-test in Reaper showed prompts like "make a clean organ lead" producing unconvincing results. Root cause: the engine is purely subtractive (Sine/Saw/Square/Triangle → filter → ADSR), so several common timbres — organ, electric piano, bell, vocal — are physically out of reach regardless of how good the AI parameter mapping is. Claude can only describe what the engine can produce.

## Goal

Lift the most common ceiling by adding a **wavetable oscillator path** with a small curated bank, so Claude has authentic timbres to mix/filter/shape rather than approximating with a saw. Architecture must support growing the bank in future phases without breaking presets.

## Non-goals (V1)

- Multi-frame morphing wavetables (Serum-style). Defer to a later phase if user demand emerges from testing.
- Custom user wavetables (loading `.wav` from disk).
- Bonus tables beyond the strict 5 (brass, pluck, sub, pad).
- A new modulation destination for "wavetable position" (no morph param in V1).

## Design decisions

### Architecture choice

**Single-cycle bank with hierarchical selector.** The 4 native PolyBLEP waveforms stay strictly intact. One new `Waveform::Wavetable` value (index 4, appended) routes to a `WavetableBank` lookup. A new Choice param `oscN.table` selects which curated table to play.

Rejected alternatives:
- *Flat enum extension* (Sine/Saw/Square/Tri/Organ/EP/...): conceptually muddies synth shapes and instrument models in one dropdown, makes the `Oscillator` switch dispatch heterogeneous (PolyBLEP for some, lookup for others), and doesn't extend cleanly to a richer bank.
- *Multi-frame morphing wavetables*: 4 MB per table × bank size, multi-frame mipmaps, morph param plumbing through ModBus, evolving-table UI. Solves a different problem (evolving timbres) on top of the original one. Out of scope for the current driver.

### Bank V1 — strict 5

| Index | Name   | Source                  | Authoring                                                                  |
|-------|--------|-------------------------|----------------------------------------------------------------------------|
| 0     | Organ  | Generated additive      | Hammond drawbar 888000000 (9 partials, integer ratios)                     |
| 1     | EP     | AKWF (CC0)              | Cherry-picked from `AKWF_epiano_*` (Rhodes flavor)                         |
| 2     | Bell   | AKWF (CC0)              | Cherry-picked from `AKWF_bell_*` (tonal bell)                              |
| 3     | Vocal  | Generated additive      | Formant comb F1=730 Hz, F2=1090 Hz, F3=2440 Hz ("Aa" vowel)                |
| 4     | Noise  | Generated (fixed seed)  | 2048 random samples from a deterministic RNG                               |

All five tables are produced by `tools/wavetable/generate.py`, which writes the band-limited mip arrays into `Source/Engine/WavetableData.cpp`. The generated `.cpp` is committed to git so the C++ build never runs Python.

Append-only. Future tables (brass, pluck, sub, etc.) are added to the end of this list to preserve preset compatibility.

AKWF = Adventure Kid Waveforms by Kristoffer Ekstrand, CC0 / public domain. ~50 KB of source `.wav` committed under `tools/wavetable/akwf-sources/` for reproducibility. Attribution will be added to a new `THIRDPARTY.md` (courtesy, not required by license).

**AKWF cherry-pick criteria (EP and Bell):**
1. Pitch-detectable: a clear fundamental, no warble or detuning suggesting frequency drift in the source.
2. Tonal & musically usable: harmonic spectrum sparse enough to remain recognizable through a low-pass filter; reject entries dominated by inharmonic noise or distortion.
3. Symmetric or zero-crossing-aligned at the cycle boundary: avoids a click on phase wrap. Visible in waveform inspection — start and end samples should be near zero.
4. Distinct from existing tables: the EP candidate should sound like an electric piano in isolation (against a sustained note through a clean filter setting), the Bell should sound bell-like as a bright tonal pluck.

The chosen `.wav` filenames are recorded in a header comment of `WavetableData.cpp` so the choice is auditable and reversible.

### Storage format

- 5 tables × 10 mipmap levels per table.
- mip[0] = 2048 samples; mip[N] = 2048 / 2^N samples (mip[9] = 4 samples).
- Total: ~80 KB of `constexpr float` arrays embedded in the binary via a generated `Source/Engine/WavetableData.cpp`.
- Generation is performed once at design time by `tools/wavetable/generate.py` (FFT → zero high bins → IFFT for each mip level). The generated `.cpp` is committed to git. Python is **not** a build-time dependency — only a developer tool when the bank changes.

### DSP path

```cpp
case Waveform::Wavetable:
    sample = bank->fetchSample(tableIdx, phase, dt);
    break;
```

`WavetableBank::fetchSample(tableIdx, phase, dt)`:
1. **Precondition:** `dt > 0`. Callers (only `Oscillator::getNextSample`) already guarantee this since `frequency > 0` post-`noteOn`. The bank does not validate; passing `dt == 0` would feed `log2(0) = -∞` into the formula. Documented as a precondition rather than guarded with a runtime check, to keep the inner loop branch-free.
2. `mipIdx = std::clamp(int(std::ceil(std::log2(dt * 2048.0))), 0, 9)` — selects the smallest mip whose band-limit sits at or above the playback Nyquist. Derivation: mip[N] holds harmonics 1..(1024/2^N); aliasing-free playback requires (1024/2^N) × f ≤ SR/2, equivalently `dt × 2048 ≤ 2^N`, so we want the smallest N with `2^N ≥ dt × 2048`. The implementer should verify this formula behaves as expected at boundaries (dt × 2048 = 1, 2, 4, …) before locking it in.
3. 2-tap linear interp inside that mip: `mip[i] + frac * (mip[i+1] - mip[i])`.

Linear interp is sufficient because each mip is already band-limited to ~Nyquist/2. The 10-mip density makes residual aliasing inaudible across the MIDI range. Cubic interp would be over-engineering.

CPU cost is comparable to the existing `Sine` case (1 cmp + 1 ln + 1 floor + 2 reads + 2 mul + 1 add ≈ `std::sin`). No expected regression.

### Voice / Unison integration

`UnisonOscillator` wraps N `Oscillator` instances. Each instance independently calls `bank->fetchSample` with its own (phase, detuned-freq). No changes inside `UnisonOscillator` other than forwarding `setTable` and the bank pointer to its sub-oscillators.

`Filter`, `Envelope`, `ModBus` — pipe-through unchanged. The wavetable osc plugs into the same point as the other waveforms.

### Behavior of orphan params under `wave==Wavetable`

| Param            | Behavior                                                                 |
|------------------|--------------------------------------------------------------------------|
| `oscN.pwm`       | Ignored (already the case for Sine/Saw/Triangle).                        |
| `oscN.phase`     | **Active** — phase ∈ [0,1) maps to read offset within the table.         |
| `oscN.fine`      | Active — frequency-driven, transparent through mip selection.            |
| `oscN.octave`/`semi` | Active.                                                              |
| `oscN.level`     | Active.                                                                  |
| Unison voices/detune/spread | Active.                                                       |

UI conditionally dims `oscN.pwm` when `wave != Square` (already implemented) and `oscN.table` when `wave != Wavetable` (new, mirrors PWM pattern).

### Param surface

New params:
- `osc1.table` — Choice, indices 0..4, default 0 (Organ)
- `osc2.table` — Choice, indices 0..4, default 0 (Organ)

Modified params:
- `osc1.wave` — Choice extended from `[Sine, Saw, Square, Triangle]` to `[Sine, Saw, Square, Triangle, Wavetable]`. `Wavetable` is **appended** at index 4. Existing presets retain values 0–3 unchanged.
- `osc2.wave` — same extension.

### AI integration

`ParamIdIndex` mirrors `ParameterLayout` exactly:
- `osc1.wave` and `osc2.wave` choice arrays grow to 5 entries.
- `osc1.table` and `osc2.table` are added as Choice entries with `[Organ, EP, Bell, Vocal, Noise]`.

`SystemPrompt` gains an explanatory paragraph (manages Claude's expectations to prevent the same overpromise that drove Phase B):
> *"To use sampled instrument timbres, set oscN.wave to 4 (Wavetable) AND oscN.table to the desired index. The table param is ignored when wave is Sine/Saw/Square/Triangle. V1 timbre limitations: 'Bell' is a tonal harmonic snapshot — works as a bright pluck through filter+envelope, not as a full evolving bell decay. 'Vocal' is a single 'Aa' vowel — does not synthesize 'Ooh', 'Eh', or other phonemes. Choose accordingly when matching prompts."*

This conditional-pertinence pattern is already in use for PWM (only meaningful when wave==Square). Claude reads param names well; risk of mis-coupling is low. The drift test (`Tests/ParamIdIndexDriftTests.cpp`) catches any divergence between APVTS and `ParamIdIndex`.

### UI exposure

- `WaveTypeSelect` accepts a 5th entry "Wavetable".
- New component `Source/UI/components/TableSelect.h` — `juce::ComboBox` adossé à `oscN.table`, populated with `[Organ, EP, Bell, Vocal, Noise]`. Mirrors `WaveTypeSelect`'s pattern.
- Placement: under the wave selector in each OSC panel of `SoundTab`.
- Conditional dimming: `TableSelect` is rendered but disabled (greyed) when `oscN.wave != 4`. Achieved via a `juce::ParameterAttachment` on `oscN.wave`, identical pattern to the existing `osc1WaveAttach` that toggles `knobOsc1Pwm.setEnabled` based on `wave==Square`.
- `OscilloscopeView` extended: when `wave==4`, it reads the table cycle from `WavetableBank::getCycle(tableIdx)` and renders that. Otherwise, current behavior. The existing `sample()` switch gains a 5th case.

`OscilloscopeView`'s constructor signature gains a `WavetableBank*` parameter (nullable for existing call sites that don't show wavetables). The bank pointer is propagated from `AISynthProcessor` through `SoundTab`'s constructor chain.

## File-level impact

### New files

| File                                          | Purpose                                                                |
|-----------------------------------------------|------------------------------------------------------------------------|
| `Source/Engine/WavetableBank.h` / `.cpp`      | Runtime API: `fetchSample(idx, phase, dt)`, `getCycle(idx)` for scope  |
| `Source/Engine/WavetableData.cpp`             | Generated, committed; arrays + entry table                             |
| `Source/UI/components/TableSelect.h`          | Combo box for `oscN.table`                                             |
| `Tests/WavetableBankTests.cpp`                | Sanity: load, normalize ±0.95, 10 mips, no NaN, energy below Nyquist   |
| `tools/wavetable/generate.py`                 | Build-time tool, hors build C++                                        |
| `tools/wavetable/akwf-sources/*.wav`          | Source wavs for EP/Bell, ~50 KB, CC0                                   |
| `THIRDPARTY.md`                               | AKWF attribution                                                       |

### Modified files

| File                                          | Change                                                                                  |
|-----------------------------------------------|------------------------------------------------------------------------------------------|
| `Source/Engine/Oscillator.h`                  | `Waveform::Wavetable`; `setTable(int)`; const `WavetableBank*` member; `setBank(...)`    |
| `Source/Engine/Oscillator.cpp`                | Dispatch case `Wavetable` → `bank->fetchSample(...)`                                     |
| `Source/Engine/UnisonOscillator.{h,cpp}`      | Forward `setTable` and bank pointer to sub-oscillators                                   |
| `Source/Engine/Voice.h`                       | `OscParams.tableIdx` (int)                                                               |
| `Source/Engine/Voice.cpp`                     | Apply `tableIdx` in `setParams`                                                          |
| `Source/Engine/SynthEngine.{h,cpp}`           | Hold `WavetableBank` member; `prepare()` injects bank pointer into each voice            |
| `Source/Parameters/ParameterIDs.h`            | `osc1Table = "osc1.table"`, `osc2Table = "osc2.table"`                                   |
| `Source/Parameters/ParameterLayout.cpp`       | Append `"Wavetable"` to wave choices; add 2 new Choice params                            |
| `Source/AI/ParamIdIndex.cpp`                  | Sync wave choices to 5 entries; add `osc1.table` / `osc2.table` entries                  |
| `Source/AI/SystemPrompt.cpp`                  | Add the wavetable coupling sentence                                                      |
| `Source/PluginProcessor.cpp`                  | `buildVoiceParams` reads `osc1.table` / `osc2.table` into `OscParams.tableIdx`           |
| `Source/UI/SoundTab.h`                        | Add `TableSelect` instances; conditional-dim `ParameterAttachment` on wave param         |
| `Source/UI/components/OscilloscopeView.h`     | Constructor gains `WavetableBank*`; `sample()` gains wavetable case                      |
| `Tests/ParamIdIndexDriftTests.cpp`            | Adapt expected counts if hardcoded; verify by running                                    |
| `CMakeLists.txt`                              | Add new `.cpp` files to `AISynth_VST3` and `AISynthTests` targets                        |

### Control flow at init

```
SynthEngine::prepare(spec)
  ├─ for each Voice: voice.setBankPointer(&wavetableBank)
  └─ ...
```

`WavetableBank` is a member of `SynthEngine`. Its constructor sets up the entry table pointing at the `constexpr` arrays in `WavetableData.cpp`; no separate `initialize()` is needed. Voices hold a `const WavetableBank*` set once at `prepare`, never rewritten.

### Real-time safety

`WavetableBank` construction happens once when `SynthEngine` is constructed (off the audio thread). It only stores const pointers into static `constexpr` arrays. No allocation, no file I/O, no locks. `fetchSample` on the audio thread is purely read-only.

## Testing strategy

| Test target                            | Coverage                                                                                |
|----------------------------------------|------------------------------------------------------------------------------------------|
| `WavetableBankTests` (new)             | Each table is reachable; mip count = 10 per table; all samples ∈ [-1, +1]; no NaN; for each mip, FFT energy in bins above its band-limit < -60 dB (proves band-limiting) |
| `OscillatorTests` (extended)           | `Waveform::Wavetable` case renders non-zero across MIDI range; no DC offset > 0.05; aliasing check at high notes (C7, C8) |
| `ParamIdIndexDriftTests` (auto)        | Catches APVTS ↔ `ParamIdIndex` mismatch on the new params                                |
| `VoiceTests` (extended)                | Voice with `tableIdx=0..4` renders without crash; tableIdx out-of-range is clamped       |
| `PatchApplierTests` (extended)         | Claude setting `osc1.wave=4` + `osc1.table=2` lands correctly in `OscParams`             |

Manual smoke test post-build (per A.2 pattern):
- Load AI Synth in Reaper.
- Cycle through `osc1.wave = Wavetable` × `osc1.table = 0..4` ; each table audibly distinct.
- Send the prompt "make a clean organ lead" and verify the modal returns variations that use the Wavetable shape and produce a recognizable organ.
- Load a pre-V1 preset and confirm `osc1.table` defaults to Organ silently (since wave is 0..3, table is ignored).

## Migration

- `osc1.wave` Choice extended by appending — existing preset values 0..3 remain valid and meaningful.
- `osc1.table` is new with default 0 (Organ); ignored on pre-V1 presets since their wave is 0..3.
- No migration code path required. No version bump in `PresetManager`.

## Risks & open questions

1. **Mipmap selection cost in inner loop.** `log2 + floor` per sample × up to 112 oscillators concurrently (8 voices × 2 osc × 7 unison) is theoretically fine but worth measuring. If we see CPU pressure, **cache `mipIdx` at `Voice::setParams`** (frequency only changes per block, not per sample). Implement this cache only if profiling shows pressure; do not pre-optimize.

2. **`OscilloscopeView` constructor signature.** Currently `(apvts, paramID)`. Will become `(apvts, paramID, bankPtr)`. The only production call site is `Source/UI/SoundTab.h` (osc1 + osc2 scopes); test usage in `Tests/UIConstructionTests.cpp` must also be updated. `bankPtr` is non-nullable in this design — if a future caller doesn't need wavetable rendering, an overload without the param can be added then.

3. **Drift test format.** `ParamIdIndexDriftTests.cpp` may hardcode counts; needs verification. If the test enumerates the wave choices explicitly, that enumeration must be updated to match.

4. **AKWF curation effort.** Cherry-picking the right EP and Bell from ~4000 AKWF files requires listening time. Budget: half a day of audition with Reaper, picking 1 strong candidate per category. Backup plan: if no AKWF EP convinces, fall back to additive (bell-component × ratio 2.76 + tine partial decay frozen) — adds 2–3 hours.

5. **Bank initialization order.** `WavetableBank::initialize` must run before any voice renders. Today, `SynthEngine::prepare` is called once before `processBlock`, so this is automatic. Tests must exercise the full prepare path, not call voice methods directly with an uninitialized bank.

6. **Polyphonic table-switching during a held note.** If the user changes `osc1.table` while a note is held, the mid-note table swap will produce a click. Acceptable for V1 — same as a mid-note waveform change today. No ramp/crossfade needed.

## Out of scope (deferred to future phases)

- Multi-frame morphing wavetables and a `morph` modulation destination.
- Loading custom user wavetables from disk.
- Bonus tables (brass, pluck, sub, pad).
- Wavetable-specific UI affordances (waterfall view, frame scrubber).
- Sound-design presets that ship with the plugin showcasing wavetable timbres (separate effort, after engine ships).

## Acceptance criteria

- All existing tests pass (≥ 127/127 baseline maintained).
- New tests added per the testing strategy table; all green.
- Plugin builds and deploys via existing `deploy.ps1` flow.
- Manual smoke test in Reaper passes the 4 checks listed under "Testing strategy".
- `make a clean organ lead` prompt produces a variation that uses `wave=Wavetable, table=Organ` and sounds recognizably organ-like.
- No CPU regression in 8-voice + 7-unison stress test (measured via Reaper's plugin CPU meter).
- No preset compatibility regression: a pre-V1 preset loaded post-merge sounds identical to before.
