# Tech Debt Sweep — Design Spec

> **Scope:** chirurgical. One regression fix in `ArpEngine`, plus a content-tracking sweep on the working tree (`.gitignore`, untracked Fonts, untracked design plans, untracked Claude settings). No file splits, no FXChain/APIServer changes — those wait for the AI layer phase.

**Source of truth for what's missing:** `git status` of the working tree at HEAD `2de8843` (post-Phase 5 ship).

---

## Goal

Leave the repo in a state where:

- A user switching arp `Mode` from `Chord` to anything else mid-play does not get stuck-on chord voices.
- A fresh `git clone` builds without the user having to manually re-add fonts.
- Phase 3/4/5 design plans are part of the version history.
- `.claude/settings.json` (shareable) is tracked; `.claude/settings.local.json` and `scheduled_tasks.lock` (per-user runtime) are ignored.
- Build/test scratch files (`build*.log`, `test*.txt`, `AISynth.zip`) no longer pollute `git status`.

---

## What this phase does NOT ship

- File splits (no prod file > 400 LOC justifies a split)
- Activation of `FXChain` stub — Phase 4 already shipped FxSlots, the old FXChain shell is irrelevant; we leave it
- Activation of `APIServer` stub — owned by the next phase (AI layer)
- Font BinaryData embedding — `Source/UI/Fonts.h` keeps its 3 `TODO(font-embedding)` markers; tracked-on-disk fonts unblock cloning
- Migration helpers, preset breaking changes, new params

---

## Architecture

No architectural changes. Two diff surfaces:

1. **Code** — `Source/Engine/ArpEngine.cpp::setParams()` gains a 3-line guard; `Tests/ArpEngineTests.cpp` gains 1 regression test block.
2. **Repo content** — `.gitignore` extended; new files added under `git add`; obsolete files `rm`-ed.

---

## Engine fix

### Bug

When `ArpEngine::setParams(p)` is called with `params.mode == Mode::Chord && p.mode != Mode::Chord` while `noteIsOn == true`:

- `currentChordNotes` still holds the N notes emitted by the previous Chord step (populated at `ArpEngine.cpp:311–316`).
- `params = p` overwrites mode in place. The next gate-driven noteOff path (`ArpEngine.cpp:269–282`) checks `params.mode == Mode::Chord` — now false — and falls into the `else` branch, emitting noteOff only for `lastNote`.
- The remaining N-1 chord voices stay on. Audible result: hung notes until manual key release or arp disable.

The same hazard exists at `ArpEngine.cpp:289–302` (the swing-shift defensive noteOff path).

The current source acknowledges the limitation at `ArpEngine.cpp:309–310`:
```
// Note: stale currentChordNotes from a Chord→other mode switch are
// handled by the mode==Chord guards above; v1 limitation.
```

### Fix

In `setParams`, when leaving `Chord` mode while a note is on, arm `pendingNoteOff = true`. The existing `pendingNoteOff` path at `ArpEngine.cpp:189–214` already flushes `currentChordNotes` correctly:

```cpp
if (pendingNoteOff && lastNote >= 0)
{
    if (!currentChordNotes.empty())
    {
        for (int cn : currentChordNotes)
            midi.addEvent(juce::MidiMessage::noteOff(1, cn), 0);
        currentChordNotes.clear();
    }
    else
    {
        midi.addEvent(juce::MidiMessage::noteOff(1, lastNote), 0);
    }
}
```

Patch (insertion point: after the existing `enabled→disabled` transition block, before `params = p`):

```cpp
// Transition: Chord → other while playing — flush all chord voices to avoid hung notes
if (params.mode == Mode::Chord && p.mode != Mode::Chord && noteIsOn)
    pendingNoteOff = true;
```

After the flush, `process()` clears `noteIsOn`, `lastNote`, `pendingNoteOns` and returns early (existing behavior at `ArpEngine.cpp:209–213`). The next `process()` call enters with clean state and starts the new mode's sequence on the next step boundary.

### Test

`Tests/ArpEngineTests.cpp` — one new test block:

**`chord→up mid-play emits noteOff for all chord voices`**
1. `prepare()`, `setParams({enabled=true, mode=Chord, octaveRange=1, rateIndex=2 (1/16), gate=1.0, swing=0, humanize=0, latch=false})`.
2. `noteOn(60, 1.0); noteOn(64, 1.0); noteOn(67, 1.0);`
3. `process(buf, samplesPerStep, bpm=120, sr=48000)` — expect 3 simultaneous noteOns (sample position 0, channel 1, notes {60,64,67}).
4. `setParams({same as above but mode=Up})` — should arm pendingNoteOff.
5. `process(buf2, 1, bpm=120, sr=48000)` — expect 3 noteOffs at sample 0 for {60,64,67}, no noteOn.
6. Assert: `buf2` contains exactly 3 noteOff events with channel 1 and pitches `{60, 64, 67}` (use a `std::set<int>` to ignore order).

Total post-Phase-5 test count: 83 → 84.

---

## Repo content

### `.gitignore` additions

Append to existing `.gitignore`:

```
# Build / test scratch artifacts
build*.log
test*.log
build_*.txt
test_*.txt
test_results.log
test_out.log
AISynth.zip

# Per-user Claude state
.claude/settings.local.json
.claude/scheduled_tasks.lock
```

Patterns are deliberately narrow (no bare `*.log` / `*.txt`) so we do not silently exclude `OFL.txt`, `AUTHORS.txt` inside `Source/UI/Fonts/`, `README.md`, or any future tracked text artifact.

### Files to remove from working tree

```
build.log
build_output.txt
build_step2.txt
build_tests_only.txt
build_tests_step4.txt
test_build.log
test_build_err.txt
test_build_out.txt
test_out.log
test_output.txt
test_results.log
AISynth.zip
```

12 files, all already untracked (no `git rm`, just `rm`). After the gitignore commit, `git status` no longer surfaces them.

### Files to track (`git add`)

| Group | Paths | Notes |
|---|---|---|
| Fonts | `Source/UI/Fonts/Instrument_Serif/`, `Source/UI/Fonts/JetBrainsMono-2.304/`, `Source/UI/Fonts/inter/` | Three font families, ~30+ `.ttf` + `OFL.txt` + `AUTHORS.txt`. Loaded from disk by `Source/UI/Fonts.h`; required for plugin to render correctly on a fresh clone. |
| Plans | `docs/superpowers/plans/2026-04-24-handover.md`, `docs/superpowers/plans/2026-04-24-phase3-modmatrix.md`, `docs/superpowers/plans/2026-04-25-modengine.md`, `docs/superpowers/plans/2026-04-25-phase4-fx-slots.md` | Design history for Phases 3/4 + handover. Phase 5 plan is already tracked. |
| Claude shared settings | `.claude/settings.json` | Shareable hooks/prefs. `settings.local.json` stays ignored (per-user). |

---

## Tasks

| # | Commit | What |
|---|---|---|
| 1 | `fix(engine): flush chord notes when arp mode changes mid-play` | `ArpEngine.cpp::setParams` 3-line guard + 1 regression test in `ArpEngineTests.cpp`. Build + run tests, expect 84/84 pass. |
| 2 | `chore: gitignore build/test artifacts and runtime locks` | Append patterns to `.gitignore`, then `rm` the 12 stale files on disk. The `rm` carries no git diff (files were never tracked); the commit's diff is only the `.gitignore` change. |
| 3 | `chore(ui): commit project fonts (Inter, JetBrains Mono, Instrument Serif)` | `git add Source/UI/Fonts/`. Verify nothing in `Source/UI/Fonts/` is excluded by the new ignore patterns. |
| 4 | `chore(docs): commit Phase 3/4/5 plans and handover` | `git add` the 4 listed plan files. |
| 5 | `chore(claude): commit shared settings.json` | `git add .claude/settings.json`. Confirm `settings.local.json` and `scheduled_tasks.lock` remain untracked. |
| 6 | Final verification (no commit) | `git status` clean, `Source/UI/Fonts/` indexed, build + tests still green from Task 1's verification. |

**Branch:** `tech-debt-sweep` (from `main` at `2de8843`).

**Commit ordering rationale:** the engine fix (Task 1) is independent of the content sweep — it can be reverted in isolation if the regression test catches a different issue. Tasks 2 and 3 are split (gitignore vs `rm`) so the diff for each is small and focused. Tasks 4–6 add tracked content — order between them doesn't matter, the listed order groups by directory.

**No breaking middle:** plugin and tests build green after every commit.

---

## Migration / compat

- **No preset format change.** The chord fix is purely a runtime state-flush; no APVTS or saved-state changes.
- **No parameter ID change.** No new params, no renames.
- **No build system change.** `CMakeLists.txt` untouched (Fonts/ contents are loaded at runtime via `Source/UI/Fonts.h`, not built into the binary at compile time).

---

## Self-review

- **Placeholder scan:** every commit has a concrete diff target and a verification step. No TBDs.
- **Internal consistency:** the `pendingNoteOff` flush path (`ArpEngine.cpp:189–214`) already handles `currentChordNotes` correctly; we are not introducing a new chord-flush path, only an additional condition under which the existing path is armed. The fix in `setParams` runs before `params = p` so the comparison `params.mode == Mode::Chord` reads the *old* mode, which is the intent.
- **Scope check:** chord stale + `.gitignore` + 3 categories of `git add`. Six commits. Single-session executable.
- **Ambiguity check:**
  - "When leaving Chord mode" — defined as `params.mode == Mode::Chord && p.mode != Mode::Chord` evaluated *before* `params = p`. Locked.
  - "While playing" — `noteIsOn == true`. Locked. (If `noteIsOn == false`, `currentChordNotes` is already empty by invariant — see `ArpEngine.cpp:250, 275, 295, 311`.)
  - "Test pitch order" — assert as a set, not a sequence. Locked.
  - Gitignore patterns — `build*.log` / `test*.log` / `build_*.txt` / `test_*.txt` / explicit `test_results.log`, `test_out.log` are precise enough to avoid catching `OFL.txt`, `AUTHORS.txt` in `Source/UI/Fonts/`. Locked.
- **Out-of-scope items declared:** file splits, FXChain/APIServer activation, font BinaryData embedding, preset migrations.

---

## Execution Handoff

Next step: invoke `superpowers:writing-plans` to produce the detailed task-by-task implementation plan at `docs/superpowers/plans/2026-04-26-tech-debt-sweep.md`. That plan will include the verbatim diffs for Task 1 (engine fix + test) and the exact shell commands for Tasks 2–6.

Execution mode (subagent-driven vs inline) is your call after the plan is written. Given the small scope (one code change, one test, five `git add`/`rm`/edit operations), inline single-session execution is likely the right pick — but defer the call until plan-writing is done.
