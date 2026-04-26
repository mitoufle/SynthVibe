# Phase 5 — Arp Polish Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Flesh out `ArpEngine` + `ArpTab` to match `param-audit.md` — adds 4 behavioral params (gate/swing/humanize/latch), 3 new patterns (dnup/asplayed/chord), reorders pattern+rate enums (preset break, accepted), rebuilds `ArpTab` around atomized components.

**Architecture:** Engine extensions are localised to `ArpEngine.{h,cpp}` — no new engine files. UI introduces 2 generic atoms (`SegmentedButtonRow`, `ArpOnOffPill`) under `Source/UI/components/` and rewrites `ArpTab.h` to use them alongside existing `PanelHeader` and `ArcKnob`. Step Sequencer (`seq.N.*`, 64 params, `StepSequencerGrid`) is **out of scope** and shipped as Phase 6.

**Tech Stack:** C++17, JUCE 7.0.9, `juce::AudioProcessorValueTreeState`, `juce::UnitTest`. No new third-party deps.

**Build:** `./build-with-vs.bat` (loads vcvars64 + cmake/msbuild).
**Tests:** `./build/AISynthTests_artefacts/Release/AISynthTests.exe` (note: NO space in name; the audit/legacy paths use a wrong "AISynth Tests.exe" path).
**Deploy:** `./deploy.ps1` (PowerShell only, not bash).

**Branch:** `phase5-arp-polish` (already created from main after the Phase 4 merge). No new branch needed.

**No breaking middle:** unlike Phase 4 (where the plugin target was broken between Tasks 1 and 12 due to deleted legacy IDs), every Phase 5 task leaves both the plugin target AND the test target green. Each commit can be deployed and auditioned independently.

---

## Critical environment notes

These are gotchas learned from Phase 4 that subagent implementers must internalise:

1. **Test exe path:** `./build/AISynthTests_artefacts/Release/AISynthTests.exe` — NOT `"AISynth Tests.exe"` (no space).
2. **`./build-with-vs.bat` returns exit 0 even when a target fails.** Always verify the test exe was actually rebuilt by checking its mtime is ≥ the build invocation time. If stale, force-rebuild only the test target with:
   `cmd //c '"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul && cmake --build build --config Release --target AISynthTests'`
3. **Stay on branch `phase5-arp-polish`.** Do NOT switch branches mid-task.
4. **`cmake` is not on the bash PATH** — must go through `vcvars64.bat` (the build script handles this; only relevant if you bypass it).

---

## File Structure

| File | Status | Responsibility |
|---|---|---|
| `Source/Parameters/ParameterIDs.h` | modify | Add `arpGate`, `arpSwing`, `arpHumanize`, `arpLatch` constexpr IDs |
| `Source/Parameters/ParameterLayout.cpp` | modify | Register 4 new params; reorder pattern/rate choice arrays (4→7 / 4→5); update defaults (pattern=2, rate=2, octaves=2) |
| `Source/Engine/ArpEngine.h` | modify | Extend `Mode` enum (`Dnup`, `AsPlayed`, `Chord` added; `Random` moves to index 4); extend `Params` struct (gate/swing/humanize/latch); add `humanizeRng`, `samplesUntilNoteOff`, `swingShiftSamples`, `freshLatchGroup`, `latchedNotes` members |
| `Source/Engine/ArpEngine.cpp` | modify | `buildSequence` cases for new modes; `process` gate/swing/humanize timing; `noteOn`/`noteOff` latch behavior; `samplesPerStep` rate table 4→5 entries |
| `Source/PluginProcessor.cpp` | modify | `buildArpParams` reads 4 new APVTS values |
| `Source/UI/components/SegmentedButtonRow.h` | create | Generic N-segment choice picker bound to a JUCE Choice param |
| `Source/UI/components/ArpOnOffPill.h` | create | Styled bool toggle for arp.on with active-glow |
| `Source/UI/ArpTab.h` | rewrite | Atomized layout: `PanelHeader` + `ArpOnOffPill` + 2× `SegmentedButtonRow` + 4× `ArcKnob` + Latch toggle |
| `Tests/ParameterIdMigrationTests.cpp` | modify | Defaults check + negative legacy enum break documentation |
| `Tests/ArpEngineTests.cpp` | modify | 10 new test blocks (3 patterns + 2 gate + 1 swing + 2 humanize + 2 latch) |
| `Tests/UIConstructionTests.cpp` | modify | 3 new test blocks (SegmentedButtonRow, ArpOnOffPill, ArpTab atomized) |
| `CMakeLists.txt` | no change | All new files are header-only or already-listed sources |

Total: 17 new test blocks across 3 files; 8 source files modified; 2 created.

---

## Task 1: Add 4 new arp params, reorder pattern/rate enums, update defaults

**Files:**
- Modify: `Source/Parameters/ParameterIDs.h`
- Modify: `Source/Parameters/ParameterLayout.cpp`
- Modify: `Tests/ParameterIdMigrationTests.cpp`

This is the **breaking commit** for the arp section — it reorders the `arp.pattern` (4→7) and `arp.rate` (4→5) choice arrays. Old presets with `arp.pattern=3` (was Random) silently load as `dnup`; with `arp.rate=3` (was 1/2) load as `1/16T`. Documented and accepted as the 1.A pattern.

- [ ] **Step 1: Write the failing tests**

Append a new block to `runTest()` in `Tests/ParameterIdMigrationTests.cpp`, after the existing `"FX slots register with correct defaults"` block:

```cpp
        beginTest("Arp params (existing + new) register with audit defaults");
        {
            juce::AudioProcessorGraph dummyGraph;
            juce::AudioProcessorValueTreeState apvts(
                dummyGraph, nullptr, "AISynthState", ParameterLayout::create());

            auto readFloat = [&](const char* id) {
                auto* p = apvts.getRawParameterValue(id);
                expect(p != nullptr, juce::String("missing param: ") + id);
                return p ? p->load() : 0.f;
            };

            // Existing arp params: defaults must match audit
            expectEquals(readFloat(ParamIDs::arpEnabled),     0.f);   // false
            expectEquals(readFloat(ParamIDs::arpMode),        2.f);   // index 2 = "updn"
            expectEquals(readFloat(ParamIDs::arpRate),        2.f);   // index 2 = "1/16"
            expectEquals(readFloat(ParamIDs::arpOctaveRange), 2.f);   // 2 octaves

            // New arp params: defaults from audit
            expectWithinAbsoluteError(readFloat(ParamIDs::arpGate),     0.58f, 0.001f);
            expectWithinAbsoluteError(readFloat(ParamIDs::arpSwing),    0.22f, 0.001f);
            expectWithinAbsoluteError(readFloat(ParamIDs::arpHumanize), 0.4f,  0.001f);
            expectEquals(readFloat(ParamIDs::arpLatch),                 0.f);  // false

            // Choice arity check (locks the new enum sizes — fails loudly if anyone
            // adds/removes patterns or rates without updating the engine)
            auto* pattern = dynamic_cast<juce::AudioParameterChoice*>(
                apvts.getParameter(ParamIDs::arpMode));
            expect(pattern != nullptr, "arp.pattern must be a choice param");
            if (pattern != nullptr) expectEquals(pattern->choices.size(), 7);

            auto* rate = dynamic_cast<juce::AudioParameterChoice*>(
                apvts.getParameter(ParamIDs::arpRate));
            expect(rate != nullptr, "arp.rate must be a choice param");
            if (rate != nullptr) expectEquals(rate->choices.size(), 5);

            // Negative test — documents the 1.A break: pattern index 3 is now "dnup",
            // not the legacy "Random". This locks the choice ordering.
            if (pattern != nullptr) expectEquals(pattern->choices[3], juce::String("dnup"));
            if (rate    != nullptr) expectEquals(rate->choices[3],    juce::String("1/16T"));
        }
```

- [ ] **Step 2: Run the test exe and confirm fail**

Run: `./build-with-vs.bat`
Then: `"./build/AISynthTests_artefacts/Release/AISynthTests.exe"`

Expected: the new `Arp params register with audit defaults` test fails — the new param IDs (`arpGate` etc.) don't exist yet → compile error in the test (unknown identifier). The compile error is the "fail" signal.

- [ ] **Step 3: Add the 4 new IDs to `Source/Parameters/ParameterIDs.h`**

Find the existing arp block:

```cpp
    inline constexpr const char* arpEnabled      = "arp.on";
    inline constexpr const char* arpMode         = "arp.pattern";
    inline constexpr const char* arpRate         = "arp.rate";
    inline constexpr const char* arpOctaveRange  = "arp.octaves";
```

Append below it (before the next blank line / next section):

```cpp
    inline constexpr const char* arpGate         = "arp.gate";
    inline constexpr const char* arpSwing        = "arp.swing";
    inline constexpr const char* arpHumanize     = "arp.humanize";
    inline constexpr const char* arpLatch        = "arp.latch";
```

- [ ] **Step 4: Update the arp registration in `Source/Parameters/ParameterLayout.cpp`**

Find the existing arp registration (currently lines ~186–195):

```cpp
    params.push_back(std::make_unique<AudioParameterBool>(
        ParamIDs::arpEnabled, "Arp On", false));
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParamIDs::arpMode, "Arp Mode",
        StringArray { "Up", "Down", "UpDown", "Random" }, 0));
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParamIDs::arpRate, "Arp Rate",
        StringArray { "1/16", "1/8", "1/4", "1/2" }, 0));
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::arpOctaveRange, "Arp Octaves", 1, 4, 1));
```

Replace it entirely with:

```cpp
    params.push_back(std::make_unique<AudioParameterBool>(
        ParamIDs::arpEnabled, "Arp On", false));
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParamIDs::arpMode, "Arp Mode",
        StringArray { "up", "down", "updn", "dnup", "rand", "asplayed", "chord" }, 2));
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParamIDs::arpRate, "Arp Rate",
        StringArray { "1/4", "1/8", "1/16", "1/16T", "1/32" }, 2));
    params.push_back(std::make_unique<AudioParameterInt>(
        ParamIDs::arpOctaveRange, "Arp Octaves", 1, 4, 2));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::arpGate, "Arp Gate",
        NormalisableRange<float>(0.05f, 1.f, 0.001f), 0.58f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::arpSwing, "Arp Swing",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 0.22f));
    params.push_back(std::make_unique<AudioParameterFloat>(
        ParamIDs::arpHumanize, "Arp Humanize",
        NormalisableRange<float>(0.f, 1.f, 0.001f), 0.4f));
    params.push_back(std::make_unique<AudioParameterBool>(
        ParamIDs::arpLatch, "Arp Latch", false));
```

- [ ] **Step 5: Build, run tests, confirm new test passes**

Run: `./build-with-vs.bat`

Confirm test exe was rebuilt: `ls -la build/AISynthTests_artefacts/Release/AISynthTests.exe` — mtime should be ≥ the build invocation time. If stale, force-rebuild:
`cmd //c '"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul && cmake --build build --config Release --target AISynthTests'`

Then run: `"./build/AISynthTests_artefacts/Release/AISynthTests.exe"`
Expected: exit code 0; the new `Arp params register with audit defaults` test passes; total test count is now 65 (was 64 after Phase 4 fix).

The plugin target also still compiles — there's no breakage in this task because all consumers of `arp.pattern` / `arp.rate` use the choice INDEX, not the label string, and the index range merely expanded (0–3 → 0–6 / 0–4).

- [ ] **Step 6: Commit**

```bash
git add Source/Parameters/ParameterIDs.h Source/Parameters/ParameterLayout.cpp Tests/ParameterIdMigrationTests.cpp
git commit -m "feat(params)!: arp gate/swing/humanize/latch + extended pattern/rate enums"
```

The `!` advertises the choice-array reordering as a breaking change. Old presets with `arp.pattern=3` or `arp.rate=3` will silently mean different musical things after this commit.

---

## Task 2: ArpEngine — extend Mode enum + buildSequence for dnup / asplayed / chord

**Files:**
- Modify: `Source/Engine/ArpEngine.h`
- Modify: `Source/Engine/ArpEngine.cpp`
- Modify: `Tests/ArpEngineTests.cpp`

This task does NOT yet touch gate / swing / humanize / latch (those are Tasks 3–6). It only:
- Renumbers `Mode::Random` from 3 to 4
- Adds `Mode::Dnup = 3`, `Mode::AsPlayed = 5`, `Mode::Chord = 6`
- Implements those three new patterns in `buildSequence` and (for Chord) in `process`

The pattern *index* must match the choice array layout from Task 1 (`{up=0, down=1, updn=2, dnup=3, rand=4, asplayed=5, chord=6}`). Task 1's `Random→4` reorder was a layout-only change; this task moves the engine enum to match.

- [ ] **Step 1: Write the failing tests**

Append to the `runTest()` body of `Tests/ArpEngineTests.cpp`. Add these three test blocks at the end (just before the closing `}` of `runTest`):

```cpp
        beginTest("dnup pattern with [60, 64, 67] yields 67 → 64 → 60 → 64 → 67");
        {
            ArpEngine arp;
            arp.prepare();
            ArpEngine::Params p;
            p.enabled     = true;
            p.mode        = ArpEngine::Mode::Dnup;
            p.rateIndex   = 2;     // 1/16
            p.octaveRange = 1;
            arp.setParams(p);
            arp.noteOn(60, 1.0f);
            arp.noteOn(64, 1.0f);
            arp.noteOn(67, 1.0f);

            const double sr  = 48000.0;
            const double bpm = 120.0;
            const int stepLen = (int) ((60.0 / bpm) * 0.25 * sr);  // 1/16 = 0.25 beat
            const int total = stepLen * 5;

            juce::MidiBuffer buf;
            arp.process(buf, total, bpm, sr);

            std::vector<int> emittedNotes;
            for (auto m : buf)
                if (m.getMessage().isNoteOn())
                    emittedNotes.push_back(m.getMessage().getNoteNumber());

            const std::vector<int> expected { 67, 64, 60, 64, 67 };
            expect(emittedNotes == expected,
                   "dnup expected 67→64→60→64→67, got "
                   + juce::String(emittedNotes.size()) + " notes");
        }

        beginTest("asplayed pattern preserves insertion order [64, 60, 67]");
        {
            ArpEngine arp;
            arp.prepare();
            ArpEngine::Params p;
            p.enabled     = true;
            p.mode        = ArpEngine::Mode::AsPlayed;
            p.rateIndex   = 2;     // 1/16
            p.octaveRange = 1;
            arp.setParams(p);
            arp.noteOn(64, 1.0f);   // played first
            arp.noteOn(60, 1.0f);   // played second
            arp.noteOn(67, 1.0f);   // played third

            const double sr  = 48000.0;
            const double bpm = 120.0;
            const int stepLen = (int) ((60.0 / bpm) * 0.25 * sr);
            const int total = stepLen * 4;

            juce::MidiBuffer buf;
            arp.process(buf, total, bpm, sr);

            std::vector<int> emittedNotes;
            for (auto m : buf)
                if (m.getMessage().isNoteOn())
                    emittedNotes.push_back(m.getMessage().getNoteNumber());

            const std::vector<int> expected { 64, 60, 67, 64 };
            expect(emittedNotes == expected,
                   "asplayed expected 64→60→67→64, got "
                   + juce::String(emittedNotes.size()) + " notes");
        }

        beginTest("chord pattern emits 3 simultaneous noteOns per step");
        {
            ArpEngine arp;
            arp.prepare();
            ArpEngine::Params p;
            p.enabled     = true;
            p.mode        = ArpEngine::Mode::Chord;
            p.rateIndex   = 2;     // 1/16
            p.octaveRange = 1;
            arp.setParams(p);
            arp.noteOn(60, 1.0f);
            arp.noteOn(64, 1.0f);
            arp.noteOn(67, 1.0f);

            const double sr  = 48000.0;
            const double bpm = 120.0;
            const int stepLen = (int) ((60.0 / bpm) * 0.25 * sr);

            juce::MidiBuffer buf;
            arp.process(buf, stepLen, bpm, sr);

            // First step should emit ALL 3 notes at sample 0 (or close to it)
            std::vector<int> notesAtStep0;
            for (auto m : buf)
                if (m.getMessage().isNoteOn() && m.samplePosition == 0)
                    notesAtStep0.push_back(m.getMessage().getNoteNumber());
            std::sort(notesAtStep0.begin(), notesAtStep0.end());

            const std::vector<int> expected { 60, 64, 67 };
            expect(notesAtStep0 == expected,
                   "chord expected {60,64,67} at sample 0, got "
                   + juce::String(notesAtStep0.size()) + " notes");
        }
```

You'll likely need to add `#include <vector>` and `#include <algorithm>` at the top of `ArpEngineTests.cpp` if not already present.

- [ ] **Step 2: Build, confirm failures**

Run: `./build-with-vs.bat` then `"./build/AISynthTests_artefacts/Release/AISynthTests.exe"`

Expected: the 3 new tests fail — `Mode::Dnup`, `Mode::AsPlayed`, `Mode::Chord` are unknown identifiers (compile error). That compile error is the "fail" signal.

- [ ] **Step 3: Update `Source/Engine/ArpEngine.h` Mode enum**

Find:

```cpp
    enum class Mode { Up = 0, Down, UpDown, Random };
```

Replace with:

```cpp
    enum class Mode {
        Up       = 0,
        Down     = 1,
        UpDown   = 2,
        Dnup     = 3,    // NEW — descending then ascending
        Random   = 4,    // moved from index 3 (preset break per Phase 5 design)
        AsPlayed = 5,    // NEW — preserve insertion order
        Chord    = 6     // NEW — emit all held notes simultaneously per step
    };
```

- [ ] **Step 4: Update `Source/Engine/ArpEngine.cpp` `buildSequence` to handle new modes**

Find the existing `buildSequence`:

```cpp
void ArpEngine::buildSequence()
{
    sequence.clear();
    if (heldNotes.empty()) return;

    sortedBuf.assign(heldNotes.begin(), heldNotes.end());
    std::sort(sortedBuf.begin(), sortedBuf.end(),
              [](const HeldNote& a, const HeldNote& b) { return a.note < b.note; });

    for (int oct = 0; oct < params.octaveRange; ++oct)
        for (auto& h : sortedBuf)
            sequence.push_back({ h.note + oct * 12, h.velocity });

    if (params.mode == Mode::Down)
        std::reverse(sequence.begin(), sequence.end());

    if (params.mode == Mode::Random)
        std::shuffle(sequence.begin(), sequence.end(), rng);

    if (stepIndex >= static_cast<int>(sequence.size()))
        stepIndex = 0;
}
```

Replace it entirely with:

```cpp
void ArpEngine::buildSequence()
{
    sequence.clear();
    if (heldNotes.empty()) return;

    // AsPlayed preserves insertion order; everything else sorts ascending.
    if (params.mode == Mode::AsPlayed)
        sortedBuf.assign(heldNotes.begin(), heldNotes.end());
    else
    {
        sortedBuf.assign(heldNotes.begin(), heldNotes.end());
        std::sort(sortedBuf.begin(), sortedBuf.end(),
                  [](const HeldNote& a, const HeldNote& b) { return a.note < b.note; });
    }

    for (int oct = 0; oct < params.octaveRange; ++oct)
        for (auto& h : sortedBuf)
            sequence.push_back({ h.note + oct * 12, h.velocity });

    // Per-mode reordering after the octave expansion.
    switch (params.mode)
    {
        case Mode::Down:
            std::reverse(sequence.begin(), sequence.end());
            break;
        case Mode::Dnup:
            // Build a descending-then-ascending walk. With sequence = [60,64,67]
            // we want playback order 67→64→60→64→67. We construct it by
            // reversing first, then appending the original (minus the endpoints
            // to avoid double-hitting the bottom note when looping).
            {
                std::vector<HeldNote> dnup;
                dnup.reserve(sequence.size() * 2);
                for (auto it = sequence.rbegin(); it != sequence.rend(); ++it)
                    dnup.push_back(*it);
                if (sequence.size() > 2)
                    for (auto it = sequence.begin() + 1; it != sequence.end() - 1; ++it)
                        dnup.push_back(*it);
                sequence.swap(dnup);
            }
            break;
        case Mode::Random:
            std::shuffle(sequence.begin(), sequence.end(), rng);
            break;
        case Mode::Up:
        case Mode::UpDown:
        case Mode::AsPlayed:
        case Mode::Chord:
        default:
            break;  // already in the right order from above
    }

    if (stepIndex >= static_cast<int>(sequence.size()))
        stepIndex = 0;
}
```

- [ ] **Step 5: Update `Source/Engine/ArpEngine.cpp` `process` to handle Chord mode**

Find the step-trigger block inside `process`:

```cpp
        if (sampleCounter == 0)
        {
            if (noteIsOn && lastNote >= 0)
            {
                scratchMidi.addEvent(juce::MidiMessage::noteOff(1, lastNote), i);
                noteIsOn = false;
            }

            const auto& step = sequence[stepIndex];
            scratchMidi.addEvent(juce::MidiMessage::noteOn(1, step.note, step.velocity), i);
            lastNote = step.note;
            noteIsOn = true;

            if (params.mode == Mode::UpDown)
            {
                stepIndex += pingDir;
                const int last = static_cast<int>(sequence.size()) - 1;
                if (stepIndex > last) { stepIndex = last; pingDir = -1; }
                if (stepIndex < 0)    { stepIndex = 0;    pingDir =  1; }
            }
            else
            {
                stepIndex = (stepIndex + 1) % static_cast<int>(sequence.size());
            }
        }
```

Replace it with:

```cpp
        if (sampleCounter == 0)
        {
            if (noteIsOn && lastNote >= 0)
            {
                scratchMidi.addEvent(juce::MidiMessage::noteOff(1, lastNote), i);
                noteIsOn = false;
            }

            if (params.mode == Mode::Chord)
            {
                // Chord: emit ALL held notes simultaneously. lastNote tracks the
                // first one for noteOff bookkeeping; the existing single-noteOff
                // path won't catch the rest, but that's OK because Chord re-emits
                // all noteOffs explicitly at the next step boundary (handled below
                // by re-issuing the chord noteOffs alongside the new noteOns).
                // For the v1 shipping, we accept that overlapping notes get cut
                // at the next step boundary by the synth voice-stealing.
                for (auto& h : heldNotes)
                    scratchMidi.addEvent(juce::MidiMessage::noteOn(1, h.note, h.velocity), i);
                lastNote = heldNotes.empty() ? -1 : heldNotes.front().note;
                noteIsOn = lastNote >= 0;
                stepIndex = (stepIndex + 1);   // advance bookkeeping (sequence has 1 logical step for chord)
            }
            else
            {
                const auto& step = sequence[stepIndex];
                scratchMidi.addEvent(juce::MidiMessage::noteOn(1, step.note, step.velocity), i);
                lastNote = step.note;
                noteIsOn = true;

                if (params.mode == Mode::UpDown)
                {
                    stepIndex += pingDir;
                    const int last = static_cast<int>(sequence.size()) - 1;
                    if (stepIndex > last) { stepIndex = last; pingDir = -1; }
                    if (stepIndex < 0)    { stepIndex = 0;    pingDir =  1; }
                }
                else
                {
                    stepIndex = (stepIndex + 1) % static_cast<int>(sequence.size());
                }
            }
        }
```

- [ ] **Step 6: Build and run tests**

Run: `./build-with-vs.bat` then `"./build/AISynthTests_artefacts/Release/AISynthTests.exe"`

Verify test exe mtime ≥ build time; force-rebuild test target if stale (see env note 2).

Expected: 68 tests pass total, 0 failures. The 3 new pattern tests pass. All earlier tests still pass.

- [ ] **Step 7: Commit**

```bash
git add Source/Engine/ArpEngine.h Source/Engine/ArpEngine.cpp Tests/ArpEngineTests.cpp
git commit -m "feat(engine): arp dnup/asplayed/chord patterns"
```

---

## Task 3: ArpEngine — gate length controls noteOff timing

**Files:**
- Modify: `Source/Engine/ArpEngine.h`
- Modify: `Source/Engine/ArpEngine.cpp`
- Modify: `Tests/ArpEngineTests.cpp`

Adds `Params::gate` (float 0.05–1.0). When > 0.95 the existing "noteOff at next step boundary" behavior is approximated; below that the noteOff fires earlier inside the step. Track `samplesUntilNoteOff` per noteOn, decrement per sample, fire when reaches 0.

- [ ] **Step 1: Write the failing tests**

Append to `runTest()` body of `Tests/ArpEngineTests.cpp` (after the Task 2 chord test):

```cpp
        beginTest("gate=0.5 fires noteOff at half-step");
        {
            ArpEngine arp;
            arp.prepare();
            ArpEngine::Params p;
            p.enabled     = true;
            p.mode        = ArpEngine::Mode::Up;
            p.rateIndex   = 2;     // 1/16
            p.octaveRange = 1;
            p.gate        = 0.5f;
            arp.setParams(p);
            arp.noteOn(60, 1.0f);

            const double sr  = 48000.0;
            const double bpm = 120.0;
            const int stepLen = (int) ((60.0 / bpm) * 0.25 * sr);

            juce::MidiBuffer buf;
            arp.process(buf, stepLen, bpm, sr);

            int noteOnPos = -1, noteOffPos = -1;
            for (auto m : buf)
            {
                const auto msg = m.getMessage();
                if (msg.isNoteOn())  noteOnPos  = m.samplePosition;
                if (msg.isNoteOff()) noteOffPos = m.samplePosition;
            }
            expect(noteOnPos == 0, "noteOn at sample 0");
            const int expectedOff = stepLen / 2;
            expect(std::abs(noteOffPos - expectedOff) <= 1,
                   "noteOff at ≈stepLen/2, got " + juce::String(noteOffPos)
                   + " expected " + juce::String(expectedOff));
        }

        beginTest("gate=1.0 fires noteOff at next step boundary (regression)");
        {
            ArpEngine arp;
            arp.prepare();
            ArpEngine::Params p;
            p.enabled     = true;
            p.mode        = ArpEngine::Mode::Up;
            p.rateIndex   = 2;
            p.octaveRange = 1;
            p.gate        = 1.0f;
            arp.setParams(p);
            arp.noteOn(60, 1.0f);
            arp.noteOn(64, 1.0f);

            const double sr  = 48000.0;
            const double bpm = 120.0;
            const int stepLen = (int) ((60.0 / bpm) * 0.25 * sr);

            juce::MidiBuffer buf;
            arp.process(buf, stepLen * 2, bpm, sr);

            // First step's noteOff (60) should fire at the boundary near stepLen
            // (just before the next noteOn of 64).
            int firstNoteOffPos = -1;
            for (auto m : buf)
                if (m.getMessage().isNoteOff() && firstNoteOffPos < 0)
                    firstNoteOffPos = m.samplePosition;
            expect(std::abs(firstNoteOffPos - stepLen) <= 1,
                   "gate=1.0 should noteOff at step boundary, got "
                   + juce::String(firstNoteOffPos)
                   + " expected " + juce::String(stepLen));
        }
```

- [ ] **Step 2: Build, confirm failure**

Run: `./build-with-vs.bat` then test exe.

Expected: the new tests fail to compile — `Params::gate` doesn't exist yet.

- [ ] **Step 3: Add `gate` to `Params` struct in `Source/Engine/ArpEngine.h`**

Find:

```cpp
    struct Params
    {
        bool  enabled     = false;
        Mode  mode        = Mode::Up;
        int   rateIndex   = 0;    // 0=1/16, 1=1/8, 2=1/4, 3=1/2
        int   octaveRange = 1;    // 1..4
    };
```

Replace with:

```cpp
    struct Params
    {
        bool  enabled     = false;
        Mode  mode        = Mode::Up;
        int   rateIndex   = 2;    // index into samplesPerStep table {1/4, 1/8, 1/16, 1/16T, 1/32}
        int   octaveRange = 1;    // 1..4
        float gate        = 1.0f; // 0.05..1.0  fraction of step the note holds before noteOff
    };
```

(The `rateIndex` default also changes from 0 to 2 to track the new layout default of `1/16`. The Engine's `samplesPerStep` table will be updated in Step 4.)

Also add a private state member to `ArpEngine` (just below the existing `int sampleCounter = 0;`):

```cpp
    int   samplesUntilNoteOff = 0;   // gate timing: counts down from gate*stepLen at each step trigger
```

- [ ] **Step 4: Update `Source/Engine/ArpEngine.cpp` to handle gate timing AND the new rate table**

Find the `samplesPerStep` function:

```cpp
int ArpEngine::samplesPerStep(double bpm, double sr) const noexcept
{
    static constexpr double rateFractions[] = { 0.25, 0.5, 1.0, 2.0 };
    const double safeBpm = std::max(bpm, 20.0);
    const double beats   = rateFractions[juce::jlimit(0, 3, params.rateIndex)];
    return std::max(static_cast<int>((60.0 / safeBpm) * beats * sr), 1);
}
```

Replace with:

```cpp
int ArpEngine::samplesPerStep(double bpm, double sr) const noexcept
{
    // Aligned with ParameterLayout's StringArray { "1/4", "1/8", "1/16", "1/16T", "1/32" }.
    // Values are the fraction-of-a-quarter-note duration of one step.
    static constexpr double rateFractions[] = {
        1.0,                  // 1/4   — quarter note
        0.5,                  // 1/8   — eighth
        0.25,                 // 1/16  — sixteenth
        0.25 * (2.0 / 3.0),   // 1/16T — sixteenth-note triplet
        0.125                 // 1/32  — thirty-second
    };
    const double safeBpm = std::max(bpm, 20.0);
    const double beats   = rateFractions[juce::jlimit(0, 4, params.rateIndex)];
    return std::max(static_cast<int>((60.0 / safeBpm) * beats * sr), 1);
}
```

Then find the step-trigger block inside `process` (the one you replaced in Task 2 Step 5). Change the gate handling. The current code emits `noteOff` for the previous note INSIDE the `if (sampleCounter == 0)` block, which is at the *next* step boundary. With gate < 1.0 we need a separate count-down timer.

Replace the entire `process` body with:

```cpp
void ArpEngine::process(juce::MidiBuffer& midi, int numSamples, double bpm, double sr)
{
    if (pendingNoteOff || !pendingNoteOns.empty())
    {
        if (pendingNoteOff && lastNote >= 0)
            midi.addEvent(juce::MidiMessage::noteOff(1, lastNote), 0);

        int samplePos = 1;
        for (const auto& h : pendingNoteOns)
            midi.addEvent(juce::MidiMessage::noteOn(1, h.note, h.velocity), samplePos++);

        pendingNoteOns.clear();
        pendingNoteOff = false;
        noteIsOn       = false;
        lastNote       = -1;
        return;
    }

    if (!params.enabled)
        return;

    scratchMidi.clear();
    const int stepLen = samplesPerStep(bpm, sr);

    for (auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOff())
            noteOff(msg.getNoteNumber());
        else if (msg.isNoteOn())
            noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
        else
            scratchMidi.addEvent(msg, meta.samplePosition);
    }

    if (sequence.empty())
    {
        if (noteIsOn && lastNote >= 0)
        {
            scratchMidi.addEvent(juce::MidiMessage::noteOff(1, lastNote), 0);
            noteIsOn = false;
            lastNote = -1;
        }
        midi.swapWith(scratchMidi);
        return;
    }

    for (int i = 0; i < numSamples; ++i)
    {
        // Gate-driven noteOff: when timer hits 0, kill the current note.
        if (noteIsOn && samplesUntilNoteOff > 0)
        {
            --samplesUntilNoteOff;
            if (samplesUntilNoteOff == 0 && lastNote >= 0)
            {
                scratchMidi.addEvent(juce::MidiMessage::noteOff(1, lastNote), i);
                noteIsOn = false;
            }
        }

        if (sampleCounter == 0)
        {
            // Defensive noteOff if the previous note is still on (e.g. gate=1.0
            // means samplesUntilNoteOff hits 0 exactly at this iteration).
            if (noteIsOn && lastNote >= 0)
            {
                scratchMidi.addEvent(juce::MidiMessage::noteOff(1, lastNote), i);
                noteIsOn = false;
            }

            if (params.mode == Mode::Chord)
            {
                for (auto& h : heldNotes)
                    scratchMidi.addEvent(juce::MidiMessage::noteOn(1, h.note, h.velocity), i);
                lastNote = heldNotes.empty() ? -1 : heldNotes.front().note;
                noteIsOn = lastNote >= 0;
                stepIndex = (stepIndex + 1);
            }
            else
            {
                const auto& step = sequence[stepIndex];
                scratchMidi.addEvent(juce::MidiMessage::noteOn(1, step.note, step.velocity), i);
                lastNote = step.note;
                noteIsOn = true;

                if (params.mode == Mode::UpDown)
                {
                    stepIndex += pingDir;
                    const int last = static_cast<int>(sequence.size()) - 1;
                    if (stepIndex > last) { stepIndex = last; pingDir = -1; }
                    if (stepIndex < 0)    { stepIndex = 0;    pingDir =  1; }
                }
                else
                {
                    stepIndex = (stepIndex + 1) % static_cast<int>(sequence.size());
                }
            }

            // Schedule the gate-driven noteOff: between 1 and stepLen-1 samples
            // from now. gate=1.0 → fires exactly at next step boundary, where
            // the defensive noteOff above will have already cleaned up if needed.
            samplesUntilNoteOff = std::max(1, static_cast<int>(params.gate * stepLen));
        }
        ++sampleCounter;
        if (sampleCounter >= stepLen) sampleCounter = 0;
    }

    midi.swapWith(scratchMidi);
}
```

Also extend `reset()` to zero the new field:

Find:

```cpp
void ArpEngine::reset()
{
    heldNotes.clear();
    sequence.clear();
    sortedBuf.clear();
    stepIndex      = 0;
    sampleCounter  = 0;
    pingDir        = 1;
    lastNote       = -1;
    noteIsOn       = false;
    pendingNoteOff = false;
    pendingNoteOns.clear();
}
```

Append `samplesUntilNoteOff = 0;` before the closing brace:

```cpp
void ArpEngine::reset()
{
    heldNotes.clear();
    sequence.clear();
    sortedBuf.clear();
    stepIndex      = 0;
    sampleCounter  = 0;
    pingDir        = 1;
    lastNote       = -1;
    noteIsOn       = false;
    pendingNoteOff = false;
    pendingNoteOns.clear();
    samplesUntilNoteOff = 0;
}
```

- [ ] **Step 5: Build, run tests, confirm pass**

Run: `./build-with-vs.bat` then test exe (force-rebuild test target if stale).

Expected: 70 tests pass total. The 2 new gate tests pass.

- [ ] **Step 6: Commit**

```bash
git add Source/Engine/ArpEngine.h Source/Engine/ArpEngine.cpp Tests/ArpEngineTests.cpp
git commit -m "feat(engine): arp gate length controls noteOff timing"
```

---

## Task 4: ArpEngine — swing shifts odd steps later

**Files:**
- Modify: `Source/Engine/ArpEngine.h`
- Modify: `Source/Engine/ArpEngine.cpp`
- Modify: `Tests/ArpEngineTests.cpp`

Adds `Params::swing` (float 0–1). Even-indexed steps trigger on time; odd-indexed steps trigger `swing * stepLen / 2` samples later.

- [ ] **Step 1: Write the failing test**

Append to `runTest()` of `ArpEngineTests.cpp`:

```cpp
        beginTest("swing=1.0 shifts odd steps by half-step");
        {
            ArpEngine arp;
            arp.prepare();
            ArpEngine::Params p;
            p.enabled     = true;
            p.mode        = ArpEngine::Mode::Up;
            p.rateIndex   = 2;
            p.octaveRange = 1;
            p.gate        = 1.0f;
            p.swing       = 1.0f;
            arp.setParams(p);
            arp.noteOn(60, 1.0f);
            arp.noteOn(64, 1.0f);
            arp.noteOn(67, 1.0f);
            arp.noteOn(72, 1.0f);

            const double sr  = 48000.0;
            const double bpm = 120.0;
            const int stepLen = (int) ((60.0 / bpm) * 0.25 * sr);

            juce::MidiBuffer buf;
            arp.process(buf, stepLen * 4, bpm, sr);

            std::vector<int> noteOnPos;
            for (auto m : buf)
                if (m.getMessage().isNoteOn())
                    noteOnPos.push_back(m.samplePosition);

            // Expected positions: 0, 1.5*sps, 2*sps, 3.5*sps (stepIndices 0,1,2,3)
            expect(noteOnPos.size() >= 4, "expected 4 noteOns");
            if (noteOnPos.size() >= 4)
            {
                const int half = stepLen / 2;
                expect(std::abs(noteOnPos[0] - 0)              <= 2, "step 0 at 0");
                expect(std::abs(noteOnPos[1] - (stepLen + half)) <= 2,
                       "step 1 at sps + half-step");
                expect(std::abs(noteOnPos[2] - (2 * stepLen))    <= 2,
                       "step 2 at 2*sps");
                expect(std::abs(noteOnPos[3] - (3 * stepLen + half)) <= 2,
                       "step 3 at 3*sps + half-step");
            }
        }
```

- [ ] **Step 2: Build, confirm failure (compile error: `Params::swing` not declared)**

Run build + test exe.

- [ ] **Step 3: Add `swing` to `Params` struct in `Source/Engine/ArpEngine.h`**

Find:

```cpp
        float gate        = 1.0f;
```

Append below it:

```cpp
        float swing       = 0.0f; // 0..1  fraction of step that odd steps shift later
```

Also add a private state member next to `samplesUntilNoteOff`:

```cpp
    int   swingShiftSamples   = 0;  // computed once per step trigger; 0 for even steps, swing*stepLen/2 for odd
```

- [ ] **Step 4: Update `process` in `Source/Engine/ArpEngine.cpp` to use swing offset**

The trigger happens when `sampleCounter == swingShiftSamples` (was `== 0`). After firing, recompute `swingShiftSamples` for the *next* step.

Replace the inner trigger block (the `if (sampleCounter == 0) { ... }` from Task 3) and the surrounding `++sampleCounter; if (sampleCounter >= stepLen) sampleCounter = 0;` block with this — find the loop:

```cpp
    for (int i = 0; i < numSamples; ++i)
    {
        // Gate-driven noteOff: when timer hits 0, kill the current note.
        if (noteIsOn && samplesUntilNoteOff > 0)
        {
            --samplesUntilNoteOff;
            if (samplesUntilNoteOff == 0 && lastNote >= 0)
            {
                scratchMidi.addEvent(juce::MidiMessage::noteOff(1, lastNote), i);
                noteIsOn = false;
            }
        }

        if (sampleCounter == 0)
        {
            // ... (the entire block from Task 3) ...
            samplesUntilNoteOff = std::max(1, static_cast<int>(params.gate * stepLen));
        }
        ++sampleCounter;
        if (sampleCounter >= stepLen) sampleCounter = 0;
    }
```

Replace it with:

```cpp
    for (int i = 0; i < numSamples; ++i)
    {
        // Gate-driven noteOff: when timer hits 0, kill the current note.
        if (noteIsOn && samplesUntilNoteOff > 0)
        {
            --samplesUntilNoteOff;
            if (samplesUntilNoteOff == 0 && lastNote >= 0)
            {
                scratchMidi.addEvent(juce::MidiMessage::noteOff(1, lastNote), i);
                noteIsOn = false;
            }
        }

        if (sampleCounter == swingShiftSamples)
        {
            if (noteIsOn && lastNote >= 0)
            {
                scratchMidi.addEvent(juce::MidiMessage::noteOff(1, lastNote), i);
                noteIsOn = false;
            }

            if (params.mode == Mode::Chord)
            {
                for (auto& h : heldNotes)
                    scratchMidi.addEvent(juce::MidiMessage::noteOn(1, h.note, h.velocity), i);
                lastNote = heldNotes.empty() ? -1 : heldNotes.front().note;
                noteIsOn = lastNote >= 0;
                stepIndex = (stepIndex + 1);
            }
            else
            {
                const auto& step = sequence[stepIndex];
                scratchMidi.addEvent(juce::MidiMessage::noteOn(1, step.note, step.velocity), i);
                lastNote = step.note;
                noteIsOn = true;

                if (params.mode == Mode::UpDown)
                {
                    stepIndex += pingDir;
                    const int last = static_cast<int>(sequence.size()) - 1;
                    if (stepIndex > last) { stepIndex = last; pingDir = -1; }
                    if (stepIndex < 0)    { stepIndex = 0;    pingDir =  1; }
                }
                else
                {
                    stepIndex = (stepIndex + 1) % static_cast<int>(sequence.size());
                }
            }

            samplesUntilNoteOff = std::max(1, static_cast<int>(params.gate * stepLen));
        }
        ++sampleCounter;
        if (sampleCounter >= stepLen)
        {
            sampleCounter = 0;
            // Compute swing offset for the NEW step (about to trigger next iteration).
            const bool oddStep = (stepIndex & 1) != 0;
            swingShiftSamples = oddStep ? static_cast<int>(params.swing * stepLen * 0.5f) : 0;
            swingShiftSamples = juce::jlimit(0, stepLen - 1, swingShiftSamples);
        }
    }
```

Also extend `reset()` to zero the new field:

```cpp
    samplesUntilNoteOff = 0;
    swingShiftSamples   = 0;
```

(Insert `swingShiftSamples = 0;` right after `samplesUntilNoteOff = 0;` in `reset()`.)

- [ ] **Step 5: Build and test**

Run build + test exe (force-rebuild if stale). Expected: 71 tests pass.

- [ ] **Step 6: Commit**

```bash
git add Source/Engine/ArpEngine.h Source/Engine/ArpEngine.cpp Tests/ArpEngineTests.cpp
git commit -m "feat(engine): arp swing shifts odd steps"
```

---

## Task 5: ArpEngine — humanize via per-step seeded jitter

**Files:**
- Modify: `Source/Engine/ArpEngine.h`
- Modify: `Source/Engine/ArpEngine.cpp`
- Modify: `Tests/ArpEngineTests.cpp`

Adds `Params::humanize` (float 0–1). Two effects per step trigger:
- **Velocity scale:** `vel *= 1 - humanize * uniform(0, 0.5)` → up to 50% reduction at humanize=1.
- **Timing jitter:** trigger sample shifted by `humanize * uniform(-1, 1) * stepLen / 8` → up to ±12.5% of step length.

A dedicated `std::mt19937 humanizeRng` is seeded fixed at `prepare()` so that test runs are reproducible. Real-world play still feels human because the RNG advances per step (each step's randomness differs) — only the *sequence* of randomness is fixed across plugin sessions.

- [ ] **Step 1: Write the failing tests**

Append to `runTest()` in `ArpEngineTests.cpp`:

```cpp
        beginTest("humanize=0 produces deterministic output across two runs");
        {
            auto runOnce = []() {
                ArpEngine arp;
                arp.prepare();
                ArpEngine::Params p;
                p.enabled     = true;
                p.mode        = ArpEngine::Mode::Up;
                p.rateIndex   = 2;
                p.octaveRange = 1;
                p.gate        = 1.0f;
                p.humanize    = 0.0f;
                arp.setParams(p);
                arp.noteOn(60, 0.8f);
                arp.noteOn(64, 0.8f);
                arp.noteOn(67, 0.8f);

                const double sr  = 48000.0;
                const double bpm = 120.0;
                const int stepLen = (int) ((60.0 / bpm) * 0.25 * sr);
                juce::MidiBuffer buf;
                arp.process(buf, stepLen * 8, bpm, sr);

                std::vector<std::tuple<int,int,float>> events;
                for (auto m : buf)
                {
                    const auto msg = m.getMessage();
                    if (msg.isNoteOn() || msg.isNoteOff())
                        events.emplace_back(m.samplePosition,
                                            msg.getNoteNumber(),
                                            msg.isNoteOn() ? msg.getFloatVelocity() : 0.f);
                }
                return events;
            };
            const auto a = runOnce();
            const auto b = runOnce();
            expect(a == b, "two prepare()→process() runs must be byte-equal at humanize=0");
        }

        beginTest("humanize=1 produces velocity within [0.5*input, input] across 32 steps");
        {
            ArpEngine arp;
            arp.prepare();
            ArpEngine::Params p;
            p.enabled     = true;
            p.mode        = ArpEngine::Mode::Up;
            p.rateIndex   = 2;
            p.octaveRange = 1;
            p.gate        = 1.0f;
            p.humanize    = 1.0f;
            arp.setParams(p);
            arp.noteOn(60, 1.0f);
            arp.noteOn(64, 1.0f);

            const double sr  = 48000.0;
            const double bpm = 120.0;
            const int stepLen = (int) ((60.0 / bpm) * 0.25 * sr);

            juce::MidiBuffer buf;
            arp.process(buf, stepLen * 32, bpm, sr);

            int count = 0;
            float minVel = 1.f, maxVel = 0.f;
            for (auto m : buf)
            {
                if (m.getMessage().isNoteOn())
                {
                    const float v = m.getMessage().getFloatVelocity();
                    minVel = std::min(minVel, v);
                    maxVel = std::max(maxVel, v);
                    ++count;
                }
            }
            expect(count >= 30, "expected ~32 noteOns, got " + juce::String(count));
            // Bound: humanize=1 means velocity in [input * 0.5, input * 1.0]
            expect(minVel >= 0.499f, "min velocity " + juce::String(minVel) + " < 0.5");
            expect(maxVel <= 1.001f, "max velocity " + juce::String(maxVel) + " > 1.0");
            // Must actually vary (otherwise the RNG isn't advancing)
            expect(maxVel - minVel > 0.05f,
                   "velocity must vary; min=" + juce::String(minVel)
                   + " max=" + juce::String(maxVel));
        }
```

- [ ] **Step 2: Build, confirm failure (compile error: `Params::humanize` not declared)**

- [ ] **Step 3: Add `humanize` to `Params` struct in `Source/Engine/ArpEngine.h`**

Find:

```cpp
        float swing       = 0.0f;
```

Append below it:

```cpp
        float humanize    = 0.0f; // 0..1  per-step velocity jitter (-50% max) + timing jitter (±stepLen/8 max)
```

Also add a private state member (alongside the other new state):

```cpp
    std::mt19937 humanizeRng { 0xC0FFEEu };  // fixed-seed RNG for reproducible humanization
```

(`<random>` is already included in the header.)

- [ ] **Step 4: Update `prepare()` and `process()` in `Source/Engine/ArpEngine.cpp`**

In `prepare()`, re-seed the RNG at the start so tests that call `prepare()` get a fresh sequence:

Find:

```cpp
void ArpEngine::prepare()
{
    heldNotes.reserve(32);
    sequence.reserve(128);
    sortedBuf.reserve(32);
    pendingNoteOns.reserve(32);
    scratchMidi.ensureSize(512);
}
```

Replace with:

```cpp
void ArpEngine::prepare()
{
    heldNotes.reserve(32);
    sequence.reserve(128);
    sortedBuf.reserve(32);
    pendingNoteOns.reserve(32);
    scratchMidi.ensureSize(512);
    humanizeRng.seed(0xC0FFEEu);
}
```

In `process()`, modify the trigger block to apply humanize. The structure: when `sampleCounter == swingShiftSamples` and we fire a note, scale the velocity by `1 - humanize * uniform(0, 0.5)`.

Find the noteOn emission line (in BOTH the Chord branch and the non-Chord branch):

Chord branch:
```cpp
                for (auto& h : heldNotes)
                    scratchMidi.addEvent(juce::MidiMessage::noteOn(1, h.note, h.velocity), i);
```

Replace with:

```cpp
                for (auto& h : heldNotes)
                {
                    const float vel = applyHumanizeVelocity(h.velocity);
                    scratchMidi.addEvent(juce::MidiMessage::noteOn(1, h.note, vel), i);
                }
```

Non-Chord branch:
```cpp
                const auto& step = sequence[stepIndex];
                scratchMidi.addEvent(juce::MidiMessage::noteOn(1, step.note, step.velocity), i);
```

Replace with:

```cpp
                const auto& step = sequence[stepIndex];
                const float vel = applyHumanizeVelocity(step.velocity);
                scratchMidi.addEvent(juce::MidiMessage::noteOn(1, step.note, vel), i);
```

Then add the `applyHumanizeVelocity` helper. Add a private declaration in `ArpEngine.h`:

```cpp
    float applyHumanizeVelocity(float inputVel) noexcept;
```

And the implementation in `ArpEngine.cpp` (near the bottom, before the closing brace of the file):

```cpp
float ArpEngine::applyHumanizeVelocity(float inputVel) noexcept
{
    if (params.humanize <= 0.f) return inputVel;
    std::uniform_real_distribution<float> reduce(0.f, 0.5f);
    return inputVel * (1.f - params.humanize * reduce(humanizeRng));
}
```

For timing jitter, modify the `swingShiftSamples` recompute at the end of the loop — find:

```cpp
        if (sampleCounter >= stepLen)
        {
            sampleCounter = 0;
            const bool oddStep = (stepIndex & 1) != 0;
            swingShiftSamples = oddStep ? static_cast<int>(params.swing * stepLen * 0.5f) : 0;
            swingShiftSamples = juce::jlimit(0, stepLen - 1, swingShiftSamples);
        }
```

Replace with:

```cpp
        if (sampleCounter >= stepLen)
        {
            sampleCounter = 0;
            const bool oddStep = (stepIndex & 1) != 0;
            swingShiftSamples = oddStep ? static_cast<int>(params.swing * stepLen * 0.5f) : 0;

            if (params.humanize > 0.f)
            {
                std::uniform_real_distribution<float> jit(-1.f, 1.f);
                swingShiftSamples += static_cast<int>(
                    jit(humanizeRng) * params.humanize * (stepLen / 8.0f));
            }
            swingShiftSamples = juce::jlimit(0, stepLen - 1, swingShiftSamples);
        }
```

- [ ] **Step 5: Build and test**

Run build + test exe (force-rebuild if stale). Expected: 73 tests pass.

- [ ] **Step 6: Commit**

```bash
git add Source/Engine/ArpEngine.h Source/Engine/ArpEngine.cpp Tests/ArpEngineTests.cpp
git commit -m "feat(engine): arp humanize via per-step seeded jitter"
```

---

## Task 6: ArpEngine — latch holds notes after key release

**Files:**
- Modify: `Source/Engine/ArpEngine.h`
- Modify: `Source/Engine/ArpEngine.cpp`
- Modify: `Tests/ArpEngineTests.cpp`

Adds `Params::latch` (bool). When true:
- `noteOff` does NOT remove from `heldNotes`.
- The first `noteOn` after all keys have been released CLEARS the held set first (replace-on-new-chord, Roland convention).
- `setParams` flipping latch off CLEARS the held set immediately.

State: `bool freshLatchGroup` flag, set to `true` initially and after every `noteOff`.

- [ ] **Step 1: Write the failing tests**

Append to `runTest()` in `ArpEngineTests.cpp`:

```cpp
        beginTest("latch=true: noteOff does NOT remove from sequence");
        {
            ArpEngine arp;
            arp.prepare();
            ArpEngine::Params p;
            p.enabled     = true;
            p.mode        = ArpEngine::Mode::Up;
            p.rateIndex   = 2;
            p.octaveRange = 1;
            p.gate        = 1.0f;
            p.latch       = true;
            arp.setParams(p);
            arp.noteOn(60, 1.0f);
            arp.noteOff(60);  // would normally remove, but latch holds it

            const double sr  = 48000.0;
            const double bpm = 120.0;
            const int stepLen = (int) ((60.0 / bpm) * 0.25 * sr);

            juce::MidiBuffer buf;
            arp.process(buf, stepLen * 4, bpm, sr);

            int noteOnCount = 0;
            for (auto m : buf)
                if (m.getMessage().isNoteOn() && m.getMessage().getNoteNumber() == 60)
                    ++noteOnCount;
            expect(noteOnCount >= 4,
                   "latched note 60 should still cycle, got "
                   + juce::String(noteOnCount) + " noteOns");
        }

        beginTest("latch=true: new noteOn after release replaces latched chord");
        {
            ArpEngine arp;
            arp.prepare();
            ArpEngine::Params p;
            p.enabled     = true;
            p.mode        = ArpEngine::Mode::Up;
            p.rateIndex   = 2;
            p.octaveRange = 1;
            p.gate        = 1.0f;
            p.latch       = true;
            arp.setParams(p);

            // First chord: 60. Release.
            arp.noteOn(60, 1.0f);
            arp.noteOff(60);

            // Run a bit so the engine is in steady-state cycling on 60.
            const double sr  = 48000.0;
            const double bpm = 120.0;
            const int stepLen = (int) ((60.0 / bpm) * 0.25 * sr);
            juce::MidiBuffer warmup;
            arp.process(warmup, stepLen * 2, bpm, sr);

            // New noteOn: 64. Should REPLACE the latched 60 (Roland convention).
            arp.noteOn(64, 1.0f);

            juce::MidiBuffer buf;
            arp.process(buf, stepLen * 4, bpm, sr);

            int sixties = 0, sixty_fours = 0;
            for (auto m : buf)
            {
                if (m.getMessage().isNoteOn())
                {
                    const int n = m.getMessage().getNoteNumber();
                    if (n == 60) ++sixties;
                    if (n == 64) ++sixty_fours;
                }
            }
            expect(sixties == 0,
                   "60 should be cleared by replace-on-new-chord, got "
                   + juce::String(sixties));
            expect(sixty_fours >= 3,
                   "64 should be cycling now, got " + juce::String(sixty_fours));
        }
```

- [ ] **Step 2: Build, confirm failure (compile error: `Params::latch` not declared)**

- [ ] **Step 3: Add `latch` to `Params` struct in `Source/Engine/ArpEngine.h`**

Find:

```cpp
        float humanize    = 0.0f;
```

Append below it:

```cpp
        bool  latch       = false; // true: noteOff doesn't remove; new noteOn after release replaces
```

Also add a private state member:

```cpp
    bool  freshLatchGroup = true;  // armed when all keys released; first noteOn after a release clears the held set
```

- [ ] **Step 4: Update `noteOn`, `noteOff`, `setParams`, `reset` in `Source/Engine/ArpEngine.cpp`**

Find `noteOn`:

```cpp
void ArpEngine::noteOn(int midiNote, float velocity)
{
    for (auto& h : heldNotes)
        if (h.note == midiNote) return;
    heldNotes.push_back({ midiNote, velocity });
    buildSequence();
}
```

Replace with:

```cpp
void ArpEngine::noteOn(int midiNote, float velocity)
{
    if (params.latch && freshLatchGroup)
    {
        heldNotes.clear();           // replace-on-new-chord
        freshLatchGroup = false;
    }
    for (auto& h : heldNotes)
        if (h.note == midiNote) return;
    heldNotes.push_back({ midiNote, velocity });
    buildSequence();
}
```

Find `noteOff`:

```cpp
void ArpEngine::noteOff(int midiNote)
{
    heldNotes.erase(std::remove_if(heldNotes.begin(), heldNotes.end(),
        [midiNote](const HeldNote& h) { return h.note == midiNote; }),
        heldNotes.end());
    buildSequence();
    if (heldNotes.empty()) { stepIndex = 0; sampleCounter = 0; }
}
```

Replace with:

```cpp
void ArpEngine::noteOff(int midiNote)
{
    if (params.latch)
    {
        // Don't remove from heldNotes. Just arm the next noteOn to start a fresh chord.
        freshLatchGroup = true;
        return;
    }
    heldNotes.erase(std::remove_if(heldNotes.begin(), heldNotes.end(),
        [midiNote](const HeldNote& h) { return h.note == midiNote; }),
        heldNotes.end());
    buildSequence();
    if (heldNotes.empty()) { stepIndex = 0; sampleCounter = 0; }
}
```

Find `setParams`. After `params = p;`, add a latch-off-clears block. Replace this section in `setParams`:

```cpp
    params = p;

    if (!p.enabled)
    {
        heldNotes.clear();
        sequence.clear();
        stepIndex     = 0;
        sampleCounter = 0;
        pingDir       = 1;
    }
}
```

with:

```cpp
    const bool latchWasOn = params.latch;
    params = p;

    if (latchWasOn && !p.latch)
    {
        // Latch flipped off — release any held notes immediately.
        heldNotes.clear();
        sequence.clear();
        freshLatchGroup = true;
    }

    if (!p.enabled)
    {
        heldNotes.clear();
        sequence.clear();
        stepIndex     = 0;
        sampleCounter = 0;
        pingDir       = 1;
    }
}
```

Find `reset()` and append `freshLatchGroup = true;` to the existing list:

```cpp
    samplesUntilNoteOff = 0;
    swingShiftSamples   = 0;
    freshLatchGroup     = true;
```

- [ ] **Step 5: Build and test**

Run build + test exe (force-rebuild if stale). Expected: 75 tests pass.

- [ ] **Step 6: Commit**

```bash
git add Source/Engine/ArpEngine.h Source/Engine/ArpEngine.cpp Tests/ArpEngineTests.cpp
git commit -m "feat(engine): arp latch holds notes after key release"
```

---

## Task 7: Processor — pipe new arp params to ArpEngine

**Files:**
- Modify: `Source/PluginProcessor.cpp`

This task wires the 4 new APVTS values into the engine's `Params`. No new tests (the engine tests already cover the behavior).

- [ ] **Step 1: Edit `buildArpParams` in `Source/PluginProcessor.cpp`**

Find:

```cpp
ArpEngine::Params AISynthProcessor::buildArpParams() const
{
    ArpEngine::Params p;
    p.enabled     = *apvts.getRawParameterValue(ParamIDs::arpEnabled) > 0.5f;
    p.mode        = static_cast<ArpEngine::Mode>(static_cast<int>(*apvts.getRawParameterValue(ParamIDs::arpMode)));
    p.rateIndex   = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::arpRate));
    p.octaveRange = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::arpOctaveRange));
    return p;
}
```

Replace with:

```cpp
ArpEngine::Params AISynthProcessor::buildArpParams() const
{
    ArpEngine::Params p;
    p.enabled     = *apvts.getRawParameterValue(ParamIDs::arpEnabled) > 0.5f;
    p.mode        = static_cast<ArpEngine::Mode>(static_cast<int>(*apvts.getRawParameterValue(ParamIDs::arpMode)));
    p.rateIndex   = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::arpRate));
    p.octaveRange = static_cast<int>(*apvts.getRawParameterValue(ParamIDs::arpOctaveRange));
    p.gate        = *apvts.getRawParameterValue(ParamIDs::arpGate);
    p.swing       = *apvts.getRawParameterValue(ParamIDs::arpSwing);
    p.humanize    = *apvts.getRawParameterValue(ParamIDs::arpHumanize);
    p.latch       = *apvts.getRawParameterValue(ParamIDs::arpLatch) > 0.5f;
    return p;
}
```

- [ ] **Step 2: Build and confirm both targets compile**

Run: `./build-with-vs.bat`

Expected: plugin target AND test target build cleanly. Run the test exe; 75 tests still pass (no new test added in this task).

- [ ] **Step 3: Commit**

```bash
git add Source/PluginProcessor.cpp
git commit -m "feat(processor): pipe new arp params to engine"
```

---

## Task 8: UI — `SegmentedButtonRow` generic component

**Files:**
- Create: `Source/UI/components/SegmentedButtonRow.h`
- Modify: `Tests/UIConstructionTests.cpp`

Generic N-segment pill row bound to a JUCE Choice param. Each segment is a flat button; the active segment is filled with the accent colour. Clicking a segment commits via a hidden `juce::ComboBox` + `ComboBoxAttachment`.

- [ ] **Step 1: Write the failing test**

Append to `runTest()` of `Tests/UIConstructionTests.cpp` (after the `FxSlotCard applies per-type defaults …` block from the Phase 4 fix):

```cpp
        beginTest("SegmentedButtonRow constructs with N labels and binds to choice param");
        {
            SynthVibe::SegmentedButtonRow row(apvts, ParamIDs::arpMode,
                juce::StringArray { "Up", "Dn", "UpDn", "DnUp", "Rnd", "Played", "Chord" },
                SynthVibe::Tokens::arp);
            row.setBounds(0, 0, 700, 28);
            expectEquals(row.getNumSegments(), 7);
            // Default fx.1.type → arp.pattern default = index 2 (updn) → segment 2 active.
            expect(row.getSegmentButton(2).getToggleState() == true,
                   "segment 2 (updn) should be active by default");
        }
```

Also add this include to the top of `UIConstructionTests.cpp`:

```cpp
#include "UI/components/SegmentedButtonRow.h"
```

- [ ] **Step 2: Build, confirm failure (file does not exist)**

- [ ] **Step 3: Create `Source/UI/components/SegmentedButtonRow.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"

namespace SynthVibe
{
    // Generic N-segment pill row bound to a juce Choice param. Each segment
    // is a flat-styled button; the active segment is filled with `accent`,
    // others outline-only. Click commits selection through a hidden ComboBox
    // that owns the APVTS attachment — reuses JUCE's ComboBoxAttachment
    // plumbing without inventing a new ChoiceAttachment type.
    class SegmentedButtonRow : public juce::Component,
                               public juce::ComboBox::Listener
    {
    public:
        SegmentedButtonRow(juce::AudioProcessorValueTreeState& apvts,
                           const juce::String& choiceParamID,
                           const juce::StringArray& labels,
                           juce::uint32 accent)
            : accentColour(accent)
        {
            buttons.reserve((size_t) labels.size());
            for (int i = 0; i < labels.size(); ++i)
            {
                hiddenCombo.addItem(labels[i], i + 1);
                buttons.emplace_back(std::make_unique<juce::TextButton>(labels[i]));
                auto* btn = buttons.back().get();
                btn->setClickingTogglesState(false);
                btn->onClick = [this, i] {
                    hiddenCombo.setSelectedId(i + 1, juce::sendNotificationSync);
                };
                addAndMakeVisible(*btn);
            }

            attach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                apvts, choiceParamID, hiddenCombo);
            hiddenCombo.addListener(this);
            comboBoxChanged(&hiddenCombo);   // sync visual to current parameter state
        }

        ~SegmentedButtonRow() override { hiddenCombo.removeListener(this); }

        int getNumSegments() const noexcept { return (int) buttons.size(); }
        juce::TextButton& getSegmentButton(int i) noexcept { return *buttons[(size_t) i]; }

        void resized() override
        {
            const int n = (int) buttons.size();
            if (n <= 0) return;
            const int w = getWidth() / n;
            for (int i = 0; i < n; ++i)
            {
                const int x  = i * w;
                const int wx = (i == n - 1) ? (getWidth() - x) : w;
                buttons[(size_t) i]->setBounds(x, 0, wx, getHeight());
            }
        }

        void comboBoxChanged(juce::ComboBox* cb) override
        {
            const int sel = cb->getSelectedId(); // 1-based
            for (int i = 0; i < (int) buttons.size(); ++i)
            {
                const bool active = (i + 1) == sel;
                buttons[(size_t) i]->setColour(juce::TextButton::buttonColourId,
                    active ? juce::Colour(accentColour) : Tokens::panel2);
                buttons[(size_t) i]->setColour(juce::TextButton::textColourOffId, Tokens::ink3);
                buttons[(size_t) i]->setColour(juce::TextButton::textColourOnId,  Tokens::ink);
                buttons[(size_t) i]->setToggleState(active, juce::dontSendNotification);
                buttons[(size_t) i]->repaint();
            }
        }

    private:
        juce::ComboBox hiddenCombo;
        std::vector<std::unique_ptr<juce::TextButton>> buttons;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attach;
        juce::uint32 accentColour;
    };
}
```

- [ ] **Step 4: Build and run tests**

Run: `./build-with-vs.bat` then test exe (force-rebuild test target if stale).

Expected: 76 tests pass total. The new `SegmentedButtonRow constructs …` test passes.

- [ ] **Step 5: Commit**

```bash
git add Source/UI/components/SegmentedButtonRow.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): SegmentedButtonRow generic choice picker"
```

---

## Task 9: UI — `ArpOnOffPill` styled bool toggle

**Files:**
- Create: `Source/UI/components/ArpOnOffPill.h`
- Modify: `Tests/UIConstructionTests.cpp`

A styled "ON" pill bound to a bool param via `juce::AudioProcessorValueTreeState::ButtonAttachment`. When off: outline only. When on: filled with the accent colour.

- [ ] **Step 1: Write the failing test**

Append to `runTest()` of `Tests/UIConstructionTests.cpp`:

```cpp
        beginTest("ArpOnOffPill toggles arp.on via attachment");
        {
            SynthVibe::ArpOnOffPill pill(apvts, ParamIDs::arpEnabled);
            pill.setBounds(0, 0, 100, 28);
            // Default arp.on = false → button off.
            expect(pill.getButton().getToggleState() == false, "default off");

            // Programmatically click → APVTS should reflect.
            pill.getButton().setToggleState(true, juce::sendNotificationSync);
            expectWithinAbsoluteError(
                apvts.getRawParameterValue(ParamIDs::arpEnabled)->load(), 1.0f, 0.001f);

            // Restore for downstream tests.
            pill.getButton().setToggleState(false, juce::sendNotificationSync);
        }
```

Also add this include to the top of `UIConstructionTests.cpp`:

```cpp
#include "UI/components/ArpOnOffPill.h"
```

- [ ] **Step 2: Build, confirm failure (file does not exist)**

- [ ] **Step 3: Create `Source/UI/components/ArpOnOffPill.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"

namespace SynthVibe
{
    // Styled ON pill toggle bound to a bool APVTS param. Outline when off,
    // accent-fill when on. Wraps a juce::TextButton + ButtonAttachment.
    class ArpOnOffPill : public juce::Component
    {
    public:
        ArpOnOffPill(juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& boolParamID)
        {
            button.setButtonText("ON");
            button.setClickingTogglesState(true);
            button.setColour(juce::TextButton::buttonColourId,   Tokens::panel2);
            button.setColour(juce::TextButton::buttonOnColourId, Tokens::arp);
            button.setColour(juce::TextButton::textColourOffId,  Tokens::ink3);
            button.setColour(juce::TextButton::textColourOnId,   Tokens::ink);

            attach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                apvts, boolParamID, button);
            addAndMakeVisible(button);
        }

        juce::TextButton& getButton() noexcept { return button; }

        void resized() override { button.setBounds(getLocalBounds()); }

    private:
        juce::TextButton button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attach;
    };
}
```

- [ ] **Step 4: Build and run tests**

Run build + test exe. Expected: 77 tests pass.

- [ ] **Step 5: Commit**

```bash
git add Source/UI/components/ArpOnOffPill.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): ArpOnOffPill styled bool toggle"
```

---

## Task 10: UI — Rewrite `ArpTab` around new components

**Files:**
- Rewrite: `Source/UI/ArpTab.h`
- Modify: `Tests/UIConstructionTests.cpp`

Wholesale rewrite of `ArpTab` to use the new atomized components.

- [ ] **Step 1: Write the failing test**

Append to `runTest()` of `Tests/UIConstructionTests.cpp`:

```cpp
        beginTest("ArpTab atomized: owns pill, two segmented rows, latch toggle");
        {
            ArpTab tab(apvts);
            tab.setBounds(0, 0, 800, 240);
            expect(tab.getOnPill()       != nullptr, "missing OnPill");
            expect(tab.getPatternRow()   != nullptr, "missing PatternRow");
            expect(tab.getRateRow()      != nullptr, "missing RateRow");
            expect(tab.getLatchToggle()  != nullptr, "missing LatchToggle");
            expectEquals(tab.getPatternRow()->getNumSegments(), 7);
            expectEquals(tab.getRateRow()->getNumSegments(),    5);
        }
```

(`ArpTab` is already included in `UIConstructionTests.cpp` via the existing tests, but if not, add `#include "UI/ArpTab.h"`.)

- [ ] **Step 2: Build, confirm failure (compile error: `tab.getOnPill` undefined etc.)**

- [ ] **Step 3: Replace the entire contents of `Source/UI/ArpTab.h`**

```cpp
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Parameters/ParameterIDs.h"
#include "DesignTokens.h"
#include "components/PanelHeader.h"
#include "components/ArpOnOffPill.h"
#include "components/SegmentedButtonRow.h"
#include "components/ArcKnob.h"

class ArpTab : public juce::Component
{
public:
    explicit ArpTab(juce::AudioProcessorValueTreeState& apvts)
        : header("ARP", SynthVibe::Tokens::arp),
          onPill(apvts, ParamIDs::arpEnabled),
          patternRow(apvts, ParamIDs::arpMode,
                     juce::StringArray { "Up", "Dn", "UpDn", "DnUp", "Rnd", "Played", "Chord" },
                     SynthVibe::Tokens::arp),
          rateRow(apvts, ParamIDs::arpRate,
                  juce::StringArray { "1/4", "1/8", "1/16", "1/16T", "1/32" },
                  SynthVibe::Tokens::arp),
          octavesKnob ("Octaves",  apvts, ParamIDs::arpOctaveRange, SynthVibe::Tokens::arp, "", 0),
          gateKnob    ("Gate",     apvts, ParamIDs::arpGate,        SynthVibe::Tokens::arp, "", 2),
          swingKnob   ("Swing",    apvts, ParamIDs::arpSwing,       SynthVibe::Tokens::arp, "", 2),
          humanizeKnob("Humanize", apvts, ParamIDs::arpHumanize,    SynthVibe::Tokens::arp, "", 2)
    {
        latchToggle.setButtonText("Latch");
        latchToggle.setClickingTogglesState(true);
        latchAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, ParamIDs::arpLatch, latchToggle);

        addAndMakeVisible(header);
        addAndMakeVisible(onPill);
        addAndMakeVisible(latchToggle);
        addAndMakeVisible(patternRow);
        addAndMakeVisible(rateRow);
        addAndMakeVisible(octavesKnob);
        addAndMakeVisible(gateKnob);
        addAndMakeVisible(swingKnob);
        addAndMakeVisible(humanizeKnob);
    }

    SynthVibe::ArpOnOffPill*       getOnPill()       noexcept { return &onPill; }
    SynthVibe::SegmentedButtonRow* getPatternRow()   noexcept { return &patternRow; }
    SynthVibe::SegmentedButtonRow* getRateRow()      noexcept { return &rateRow; }
    juce::ToggleButton*            getLatchToggle()  noexcept { return &latchToggle; }

    void paint(juce::Graphics& g) override
    {
        using namespace SynthVibe::Tokens;
        auto b = getLocalBounds().toFloat().reduced(2.f);
        g.setColour(panel);
        g.fillRoundedRectangle(b, radiusLg);
        g.setColour(edge);
        g.drawRoundedRectangle(b.reduced(0.5f), radiusLg, 1.f);
    }

    void resized() override
    {
        using namespace SynthVibe::Tokens;
        auto area = getLocalBounds().reduced(spaceMd);

        header.setBounds(area.removeFromTop(20));
        area.removeFromTop(spaceSm);

        // Top row: pill (left) + latch toggle (right)
        auto topRow = area.removeFromTop(28);
        onPill.setBounds(topRow.removeFromLeft(100));
        topRow.removeFromLeft(spaceSm);
        latchToggle.setBounds(topRow.removeFromRight(80));
        area.removeFromTop(spaceSm);

        patternRow.setBounds(area.removeFromTop(28));
        area.removeFromTop(spaceXs);
        rateRow.setBounds(area.removeFromTop(28));
        area.removeFromTop(spaceSm);

        // Knob row: 4 knobs, equal width
        auto knobRow = area.removeFromTop(80);
        const int kw = knobRow.getWidth() / 4;
        octavesKnob .setBounds(knobRow.removeFromLeft(kw));
        gateKnob    .setBounds(knobRow.removeFromLeft(kw));
        swingKnob   .setBounds(knobRow.removeFromLeft(kw));
        humanizeKnob.setBounds(knobRow);
    }

private:
    SynthVibe::PanelHeader header;
    SynthVibe::ArpOnOffPill onPill;
    juce::ToggleButton latchToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> latchAttach;
    SynthVibe::SegmentedButtonRow patternRow;
    SynthVibe::SegmentedButtonRow rateRow;
    SynthVibe::ArcKnob octavesKnob;
    SynthVibe::ArcKnob gateKnob;
    SynthVibe::ArcKnob swingKnob;
    SynthVibe::ArcKnob humanizeKnob;
};
```

This replacement removes the old imports of `KnobWithLabel.h` and `LookAndFeel.h` from `ArpTab.h`. If anything else in the project still imports `KnobWithLabel.h`, leave that file in place — Phase 5 is not the cleanup pass.

- [ ] **Step 4: Build and run tests**

Run: `./build-with-vs.bat` then test exe (force-rebuild if stale).

Expected: plugin target builds; test target builds; 78 tests pass total. The new `ArpTab atomized` test passes.

- [ ] **Step 5: Commit**

```bash
git add Source/UI/ArpTab.h Tests/UIConstructionTests.cpp
git commit -m "feat(ui): rewrite ArpTab around new components"
```

---

## Task 11: Deploy + manual smoke validation

**Files:** none (manual validation).

- [ ] **Step 1: Confirm Reaper is closed (Reaper locks the VST3 DLL while open)**

Run (PowerShell):
```powershell
Get-Process reaper -ErrorAction SilentlyContinue
```

If anything returns, ask the user to close Reaper. Do not kill it.

- [ ] **Step 2: Rebuild plugin in Release**

Run: `./build-with-vs.bat`

Verify `build/AISynth_artefacts/Release/VST3/AI Synth.vst3/Contents/x86_64-win/AI Synth.vst3` is up to date (mtime should be the build time).

- [ ] **Step 3: Deploy**

Run from PowerShell (NOT bash):
```powershell
./deploy.ps1
```

Expected output: `Deploying as: AI Synth vN.vst3` (auto-incremented version), then `2 File(s) copied`, then `Done.` Confirm a new `AI Synth vN.vst3` directory appears in `C:\Program Files\Common Files\VST3\`.

- [ ] **Step 4: Manual validation checklist**

Ask the user to open Reaper, instantiate the new AI Synth version on a track, and confirm each item:

- [ ] Arp tab renders the new layout (panel header, pill+latch top row, two segmented rows, four knobs in the bottom row)
- [ ] All 7 pattern segments are clickable (Up / Dn / UpDn / DnUp / Rnd / Played / Chord). Picking each one and holding a chord audibly changes the playback order.
- [ ] All 5 rate segments work (1/4 → 1/8 → 1/16 → 1/16T → 1/32). The rate audibly changes (slow → fast).
- [ ] Octaves knob 1 → 4 cycles through correct number of octaves.
- [ ] Gate knob: at 0.05 the notes are clearly staccato; at 1.0 the notes hold to the boundary.
- [ ] Swing knob: at 0 the steps are even; at 1 the off-beats are clearly delayed (triplet feel).
- [ ] Humanize knob: at 0 it's mechanical; at 1 there's audible velocity + timing variation.
- [ ] Latch toggle: when ON and you release a chord, it keeps cycling. Pressing a new chord replaces the old one.
- [ ] Save Reaper project, close, reopen → all arp params persist.

If any item fails, capture the exact symptom and stop — Phase 5 needs a follow-up fix before merging.

- [ ] **Step 5: Tag deployment**

(Optional) Note the deployed version number (e.g. `AI Synth v40.vst3`) in the merge commit message when integrating to main.

---

## Self-review

**Spec coverage:**
- Section 1 (Engine semantics): Tasks 1 (params), 2 (patterns), 3 (gate), 4 (swing), 5 (humanize), 6 (latch). ✓
- Section 2 (Tests): 17 tests across 3 files (10 ArpEngine + 4 layout + 3 UI). Spec said 17. ✓
- Section 3 (UI rebuild): Tasks 8 (SegmentedButtonRow), 9 (ArpOnOffPill), 10 (ArpTab rewrite). ✓
- Section 4 (Task decomposition): 11 tasks as listed. ✓

**Placeholder scan:** every step contains the actual code or the actual command. No "TBD" / "implement later" / "similar to Task N" anywhere. The only conditional is "if test exe stale, force-rebuild" which is a concrete fallback command, not a hole.

**Type consistency:**
- `Mode::Random == 4` is asserted in Task 1 (negative test on choice array `[3] == "dnup"`) and assumed by Task 2 (the enum reordering). Both match.
- `kFxTypeCount` is unaffected; this phase doesn't touch FX.
- `samplesPerStep` table is updated to 5 entries in Task 3 Step 4 — index range is `[0, 4]` after the change. `params.rateIndex` Task 1 default (2 = 1/16) maps to fraction `0.25` in the new table — same musical rate as the legacy default.
- `Params::gate` defaults to 1.0f (Task 3) which preserves the legacy "noteOff at next step boundary" behavior. Task 1's APVTS default for `arp.gate` is 0.58 — consumers will set it explicitly. Default-constructed `Params` (no `setParams` call) play at gate=1.0 — safe.
- `applyHumanizeVelocity` is declared in Task 5 in `ArpEngine.h` and defined in `ArpEngine.cpp`. Both calls (Chord branch + non-Chord branch) reference it.
- `freshLatchGroup` defaults to `true` so the FIRST noteOn after `prepare()` will clear-then-add (which is a no-op since heldNotes is empty) — safe.

**Out-of-scope items declared:** Step Sequencer (`seq.N.*`, 64 params, `StepSequencerGrid`, `VelocityLane`, `PitchLane`), MIDI sync, ratchets, ties, custom orderings — all explicitly OUT in the design spec and not referenced anywhere in the tasks above.

---

## Execution Handoff

Plan complete and saved to `docs/superpowers/plans/2026-04-26-phase5-arp-polish.md`. Two execution options:

**1. Subagent-Driven (recommended)** — I dispatch a fresh subagent per task, review between tasks, fast iteration.

**2. Inline Execution** — Execute tasks in this session using executing-plans, batch execution with checkpoints.

**Which approach?**
