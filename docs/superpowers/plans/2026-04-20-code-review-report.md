# SynthVibe Code Review Report — 2026-04-20

Full codebase review at HEAD `d2eace0` (pre bug-fix session).

---

## What Is Solid

- `ScopedNoDenormals` placed at top of `processBlock` — correct.
- `buildVoiceParams()` snapshots APVTS once per block — correct pattern for avoiding lock contention.
- `Delay` uses `std::tanh` on the feedback path — no runaway.
- `Drive::Fold` has upstream `jlimit(-1024, 1024)` clamp.
- `ArpEngine::prepare()` pre-reserves vectors — no allocations on the audio thread.
- `PolyBLEP` implementation correct for Saw and Square.
- `UnisonOscillator::recomputePan()` uses equal-power sine/cosine — correct.
- `ParameterIDs.h` is the single source of truth — no raw string literals.
- All UI colours defined in `SynthLookAndFeel` constants — no raw hex literals.

---

## Critical Issues

### ~~C-1: ArpEngine drops held notes on arp-disable mid-note~~ ✅ FIXED
**File:** `Source/Engine/ArpEngine.cpp`
When arp is disabled while user holds a note, `setParams(disabled)` cleared `heldNotes` before `process()` could re-emit them. SynthEngine never received a noteOn for the held key.

### ~~C-2: Voice steal grabs wrong voice + causes click~~ ✅ FIXED
**File:** `Source/Engine/SynthEngine.cpp:64`, `Source/Engine/Voice.cpp:56`
`findFreeVoice()` always returned `voices[0]` regardless of age. Stolen voice called `osc.reset()` causing a phase jump → audible click.

---

## Important Issues

### ~~I-1: Detune LFO modulates osc1 only~~ ✅ FIXED
**File:** `Source/Engine/Voice.cpp:104`
`osc2.setDetuneCents()` was missing from the detune LFO block.

### I-2: Per-sample filter coefficient recomputation
**File:** `Source/Engine/Voice.cpp:126`, `Source/Engine/Filter.cpp:49`
`setCutoff()` calls `std::tan()` every sample under any filter modulation. With 8 voices × 2 filters × 48000 Hz = ~768k transcendental calls/sec.
**Fix:** Cache last cutoff, threshold-gate updates, or use `juce::SmoothedValue`.

### I-3: Unison phase stagger lost when unison count changes mid-note
**File:** `Source/Engine/UnisonOscillator.cpp:28`
Newly activated oscillator slots start at phase=0 when unison count changes, causing a transient burst instead of proper spread.
**Fix:** Call phase-stagger logic inside `setUnison()` for newly activated slots.

### I-4: No parameter smoothing — zipper noise on knob moves
**Files:** Multiple (`Voice.cpp`, `PluginProcessor.cpp`, `FXChain.cpp`, `Delay.cpp`, `Drive.cpp`)
No parameter is smoothed before being applied to the audio graph. Knob moves produce zipper noise.
**Fix:** `juce::SmoothedValue<float>` on master volume, filter cutoff, FX mix at minimum.

### I-5: Permanent 1/8 gain at mono use
**File:** `Source/Engine/SynthEngine.cpp:38`
`gain = 1/NumVoices = 0.125` regardless of active voice count. Single monophonic note plays at −18 dBFS.
**Fix:** Use `1/sqrt(activeVoices)` RMS normalization, or a higher master volume default.

---

## Minor Issues

### M-1: Envelope no zero-time guard
**File:** `Source/Engine/Envelope.cpp:34,42,54`
`1.f / (params.attack * sr)` — if AI layer passes bare `VoiceParams` with attack=0, divide-by-zero.
**Fix:** `juce::jmax(0.0001f, params.attack)` guards inside `Envelope.cpp`.

### M-2: FX chain order unconventional
**File:** `Source/FX/FXChain.cpp:22`
Order is Delay → Chorus → Drive → Reverb. Standard is Drive → Chorus → Delay → Reverb (saturate dry signal, not echoes). Document intent or reorder.

### M-3: LFO not retriggered on noteOn
**File:** `Source/Engine/Voice.cpp` `noteOn()`
`lfo1Osc` and `lfo2Osc` are free-running. Phase position at note start is unpredictable.

### M-4: CPU meter stub
**File:** `Source/UI/BottomBar.h:57`
`cpuLabel.setText("CPU: --", ...)` is permanently a placeholder.

### M-5: TopBar raw `new AlertWindow`
**File:** `Source/UI/TopBar.h:96`
Use `juce::AlertWindow::launchAsync` (JUCE 7+) instead of raw `new`.

### M-6: No filter keytracking
No MIDI note → filter cutoff tracking. Timbral brightness changes across keyboard range.
**Fix:** Add `filterKeytrack` parameter, scale cutoff by `midiNoteToHz(note) / 261.63f`.

### M-7: APVTS pointer caching
**File:** `Source/PluginProcessor.cpp:67`
`getRawParameterValue` does a string-hash lookup every block. Cache `float*` pointers once in `prepareToPlay`.

### M-8: UnisonOscillator::reset() stagger math uses wrong count
**File:** `Source/Engine/UnisonOscillator.cpp:43`
`reset()` iterates `MaxUnison` (7) slots but stagger math uses `unisonVoices`. Phases for inactive slots are set relative to old voice count; when count increases, new slots have wrong initial phase.

---

## Remediation Priority

| Priority | Issue | Effort |
|---|---|---|
| ~~Critical~~ | ~~C-1: Arp MIDI drop~~ | ✅ Done |
| ~~Critical~~ | ~~C-2: Voice steal wrong + click~~ | ✅ Done |
| ~~Important~~ | ~~I-1: osc2 detune LFO~~ | ✅ Done |
| Important | I-2: Per-sample filter recompute | Medium |
| Important | I-3: Unison phase stagger on count change | Small |
| Important | I-4: No parameter smoothing | Medium-Large |
| Important | I-5: Permanent 1/8 gain at mono | Small |
| Minor | M-1: Envelope zero-time guard | Small |
| Minor | M-7: APVTS pointer caching | Medium |
| Minor | M-8: UnisonOscillator reset stagger | Small |
| Minor | M-2/3/4/5/6 | Various |

---

## Post-Fix Review Findings (2026-04-20)

From the final integration review after C-1, C-2, I-1 were fixed.

### Important: Missing ArpEngine → SynthEngine integration test
No test wires `ArpEngine::process()` output directly into `SynthEngine::handleMidiMessage()`. The C-1/C-2 seam was verified by reasoning (correct), but a combined integration test would make it machine-verifiable.
**Suggested test:** enable arp → press note → `arp.process()` → feed result into `engine.handleMidiMessage()` → disable arp → repeat → assert `engine.hasActiveNote(heldNote)`.
**File:** `Tests/ArpEngineTests.cpp` or new `Tests/IntegrationTests.cpp`

### Suggestion: Stale `pendingNoteOff` on rapid re-enable
**File:** `Source/Engine/ArpEngine.cpp:18`
Re-enable guard clears `pendingNoteOns` but not `pendingNoteOff`. A disable→re-enable sequence before `process()` runs leaves a stale `pendingNoteOff=true` that fires on the next block. Add `pendingNoteOff = false;` alongside `pendingNoteOns.clear()`, or add a comment explaining the intentional asymmetry.

### Suggestion: Envelope-from-zero on stolen voices
**File:** `Source/Engine/Voice.cpp:69`
`ampEnv.noteOn()` is called unconditionally even when `stolen=true`. With a short attack (0.001s = 44 samples), the amplitude ramps from its current sustain level upward — a subtle transient. Document as a known limitation; future "smooth steal" work should ramp from current envelope level rather than restarting Attack.

### Suggestion: `hasActiveNote()` thread-safety
**File:** `Source/Engine/SynthEngine.h:22`
`hasActiveNote()` reads voice state without synchronization. Safe from audio thread; unsafe from GUI/AI threads. Add an "audio-thread only" comment to prevent misuse when the AI layer is wired up.
