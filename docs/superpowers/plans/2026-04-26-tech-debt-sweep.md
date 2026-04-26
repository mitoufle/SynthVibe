# Tech Debt Sweep Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix the chord-stale-state regression in `ArpEngine` (mode-switch from `Chord` → other while playing leaves N-1 voices stuck), and clean up the working tree (`.gitignore` build/test scratch, track project fonts and Phase 3/4 design plans, track shared `.claude/settings.json`).

**Architecture:** Two diff surfaces. (1) `Source/Engine/ArpEngine.cpp::setParams` gains a 3-line guard that arms the existing `pendingNoteOff` flush path when leaving Chord mode while a note is on; one regression test in `Tests/ArpEngineTests.cpp` validates the fix. (2) Repo content sweep — `.gitignore` extension, three `git add` groups (fonts, plans, settings), one `rm` of stale build logs.

**Tech Stack:** C++17, JUCE 7.0.9, juce::UnitTest, CMake, git, bash (Windows). Branch: `tech-debt-sweep` from `main` at `2de8843`.

---

## Source spec

`docs/superpowers/specs/2026-04-26-tech-debt-sweep-design.md` — read it first if you weren't in the brainstorming session. The bug analysis lives there, this plan is verbatim diffs and shell commands.

---

## File Structure

| File | Touch | Why |
|---|---|---|
| `Source/Engine/ArpEngine.cpp` | Modify (setParams) | Insert the 3-line guard that arms `pendingNoteOff` when leaving Chord mode |
| `Tests/ArpEngineTests.cpp` | Modify (append test) | Add regression test asserting all chord voices receive noteOff on mode change |
| `.gitignore` | Modify (append section) | Ignore build/test scratch and per-user Claude state |
| `Source/UI/Fonts/**` | Add (track) | Project fonts (Inter, JetBrains Mono, Instrument Serif) — required by `Source/UI/Fonts.h` |
| `docs/superpowers/plans/2026-04-24-handover.md` | Add (track) | Design history |
| `docs/superpowers/plans/2026-04-24-phase3-modmatrix.md` | Add (track) | Design history |
| `docs/superpowers/plans/2026-04-25-modengine.md` | Add (track) | Design history |
| `docs/superpowers/plans/2026-04-25-phase4-fx-slots.md` | Add (track) | Design history |
| `.claude/settings.json` | Add (track) | Shareable Claude Code settings |

Stale files removed from the working tree (no git diff — they were never tracked):

```
build.log build_output.txt build_step2.txt build_tests_only.txt build_tests_step4.txt
test_build.log test_build_err.txt test_build_out.txt test_out.log test_output.txt test_results.log
AISynth.zip
```

---

## Task 0: Branch setup

**Files:** none (git operation only)

- [ ] **Step 0.1: Verify clean state on main**

```bash
git status --short -uno
```

Expected: empty output (no tracked changes). Untracked files (build*.log, fonts, plans, etc.) are expected — they are the input to this sweep.

- [ ] **Step 0.2: Verify HEAD**

```bash
git rev-parse --short HEAD
```

Expected: `2de8843` (post-Phase 5 ship). If it differs, stop — this plan was authored against `2de8843` and a different base may have new test counts or file layouts.

- [ ] **Step 0.3: Create branch**

```bash
git checkout -b tech-debt-sweep
```

Expected: `Switched to a new branch 'tech-debt-sweep'`.

---

## Task 1: ArpEngine chord mode-switch fix + regression test

**Files:**
- Modify: `Source/Engine/ArpEngine.cpp:6-49` (insert into `setParams`)
- Modify: `Tests/ArpEngineTests.cpp:712` (append test block before the closing `}`)

The test goes first (TDD). Confirm it fails on the current `ArpEngine::setParams`, then apply the fix and confirm pass.

- [ ] **Step 1.1: Append the regression test**

Open `Tests/ArpEngineTests.cpp`. The current file ends with:

```cpp
            const std::vector<int> expected { 60, 64, 67 };
            expect(noteOffs == expected,
                   "expected noteOff for all 3 chord voices on release, got "
                   + juce::String(noteOffs.size()) + " noteOffs");
        }
    }
};

static ArpEngineTests arpEngineTests;
```

Insert this block immediately before the closing `}` of `runTest()` (i.e. before the `}` on the line after the last `expect(...)` of the existing chord+release-all test, the one that closes the inner `{}` block — match the indentation of the existing tests):

```cpp
        // ----------------------------------------------------------------
        // Test 19 (regression): Chord -> non-Chord mid-play flushes all chord voices
        // ----------------------------------------------------------------
        beginTest("chord -> up mode-switch mid-play emits noteOff for all chord voices");
        {
            ArpEngine arp;
            arp.prepare();

            ArpEngine::Params chord;
            chord.enabled     = true;
            chord.mode        = ArpEngine::Mode::Chord;
            chord.rateIndex   = 2;       // 1/16 in the 5-entry rate table
            chord.octaveRange = 1;
            chord.gate        = 1.0f;
            arp.setParams(chord);
            arp.noteOn(60, 1.0f);
            arp.noteOn(64, 1.0f);
            arp.noteOn(67, 1.0f);

            const double sr     = 48000.0;
            const double bpm    = 120.0;
            const int stepLen   = (int) ((60.0 / bpm) * 0.25 * sr);

            // Trigger the chord step: noteIsOn=true, currentChordNotes={60,64,67}
            juce::MidiBuffer warmup;
            arp.process(warmup, stepLen / 4, bpm, sr);

            // Switch mode Chord -> Up while still playing.
            ArpEngine::Params toUp = chord;
            toUp.mode = ArpEngine::Mode::Up;
            arp.setParams(toUp);

            // Next process() must flush all 3 chord voices, not just lastNote.
            juce::MidiBuffer buf;
            arp.process(buf, 1, bpm, sr);

            std::vector<int> noteOffs;
            for (auto m : buf)
                if (m.getMessage().isNoteOff())
                    noteOffs.push_back(m.getMessage().getNoteNumber());
            std::sort(noteOffs.begin(), noteOffs.end());

            const std::vector<int> expected { 60, 64, 67 };
            expect(noteOffs == expected,
                   "Chord->Up switch must emit noteOff for {60,64,67}, got "
                   + juce::String(noteOffs.size()) + " noteOffs");
        }
```

- [ ] **Step 1.2: Build the test target and confirm the new test fails**

```bash
cmd //c '"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul && cmake --build build --config Release --target AISynthTests'
```

Expected: build succeeds.

```bash
./build/AISynthTests_artefacts/Release/AISynthTests.exe 2>&1 | tail -40
```

Expected: at least one **FAIL** line referencing `"chord -> up mode-switch mid-play emits noteOff for all chord voices"`. The failure message will say something like `Chord->Up switch must emit noteOff for {60,64,67}, got 1 noteOffs` (only `lastNote` was flushed; voices 64 and 67 are stuck).

If the test PASSES on the unfixed code, stop and re-read `Source/Engine/ArpEngine.cpp:269-302` — either the bug doesn't exist on this codebase (advisory) or the test setup didn't trigger noteIsOn properly. Do not advance to step 1.3.

- [ ] **Step 1.3: Apply the fix in `ArpEngine::setParams`**

Open `Source/Engine/ArpEngine.cpp`. The current `setParams` starts at line 6:

```cpp
void ArpEngine::setParams(const Params& p)
{
    if (!p.enabled && params.enabled && noteIsOn && lastNote >= 0)
        pendingNoteOff = true;

    // Transition: enabled → disabled (only fires once; params still holds old state here)
    if (!p.enabled && params.enabled)
    {
        if (!heldNotes.empty())
            pendingNoteOns.assign(heldNotes.begin(), heldNotes.end());
    }

    // Transition: disabled → enabled (clear any stale state from a prior disable)
    if (p.enabled && !params.enabled)
    {
        pendingNoteOns.clear();
        pendingNoteOff = false;
        needsAllNotesOff = true;   // clear any synth voices held while arp was off
    }

    const bool latchWasOn = params.latch;
    params = p;
```

Insert the new guard between the `disabled→enabled` transition block and the `latchWasOn` line (i.e. after the closing `}` of the `if (p.enabled && !params.enabled)` block, before `const bool latchWasOn`). The result should look like:

```cpp
    // Transition: disabled → enabled (clear any stale state from a prior disable)
    if (p.enabled && !params.enabled)
    {
        pendingNoteOns.clear();
        pendingNoteOff = false;
        needsAllNotesOff = true;   // clear any synth voices held while arp was off
    }

    // Transition: Chord → other while playing — arm a flush so the existing
    // pendingNoteOff path emits noteOff for every entry in currentChordNotes,
    // not just lastNote. Without this, voices 2..N of the previous chord step
    // stay stuck on after the mode switch.
    if (params.mode == Mode::Chord && p.mode != Mode::Chord && noteIsOn)
        pendingNoteOff = true;

    const bool latchWasOn = params.latch;
    params = p;
```

The guard reads `params.mode` (old mode) before `params = p` overwrites it. The `noteIsOn` clause guarantees `currentChordNotes` is non-empty (steady-state invariant: when `noteIsOn==true` in Chord mode, `currentChordNotes` holds the active voices — populated at `ArpEngine.cpp:311-316`).

- [ ] **Step 1.4: Rebuild and run the test, confirm it now passes**

```bash
cmd //c '"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul && cmake --build build --config Release --target AISynthTests'
```

Expected: build succeeds.

```bash
./build/AISynthTests_artefacts/Release/AISynthTests.exe 2>&1 | tail -20
```

Expected output should contain:

```
All tests completed successfully
```

and the failure count line should be `0 fail`. The test count should be **84** (was 83 post-Phase 5; +1 from this commit).

If the new test still fails: re-check the insertion point. The guard MUST run before `params = p`. If `params = p` runs first, `params.mode` already equals the new mode and the guard never fires.

- [ ] **Step 1.5: Build the plugin target to ensure no compile regression**

```bash
cmd //c '"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul && cmake --build build --config Release --target AISynth_VST3'
```

Expected: build succeeds. The plugin target was independently green at HEAD `2de8843`; a `setParams` edit shouldn't break it but verify.

Also verify mtime to defend against the silent-skip-on-error behavior of `build-with-vs.bat`:

```bash
stat -c '%Y %n' "build/AISynth_artefacts/Release/VST3/AI Synth.vst3" 2>/dev/null || ls -l "build/AISynth_artefacts/Release/VST3/AI Synth.vst3"
```

Expected: timestamp newer than the moment Step 1.5 started.

- [ ] **Step 1.6: Commit**

```bash
git add Source/Engine/ArpEngine.cpp Tests/ArpEngineTests.cpp
git commit -m "$(cat <<'EOF'
fix(engine): flush chord notes when arp mode changes mid-play

When ArpEngine::setParams is called with mode transitioning from Chord to
any other mode while a note is on, currentChordNotes still holds the N
voices of the previous chord step. The next gate-driven noteOff path
(ArpEngine.cpp:269-282) sees mode != Chord, falls into the lastNote-only
branch, and leaves voices 2..N stuck on.

Arm pendingNoteOff in setParams when this transition is detected. The
existing pendingNoteOff entry path in process() (lines 189-214) already
handles the chord-flush correctly by iterating currentChordNotes.

Adds one regression test (Tests/ArpEngineTests.cpp) covering Chord -> Up
mid-play; test count 83 -> 84.

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

Expected: commit succeeds. `git log --oneline -1` shows the new commit on `tech-debt-sweep`.

---

## Task 2: Gitignore build/test scratch + clean working tree

**Files:**
- Modify: `.gitignore`
- Remove (disk only): 12 stale build/test scratch files at repo root

- [ ] **Step 2.1: Inspect current `.gitignore`**

```bash
cat .gitignore
```

Expected output (current state):

```
build/
.vs/
.superpowers/
.worktrees/
*.user
*.suo
*.aps
CMakeCache.txt
CMakeFiles/
cmake_install.cmake
.worktrees/
```

(Note the duplicate `.worktrees/` line — leave it; not the focus of this sweep.)

- [ ] **Step 2.2: Append the new patterns**

Append this block to `.gitignore`:

```

# Build / test scratch artifacts (left behind by build-with-vs.bat captures)
build*.log
test*.log
build_*.txt
test_*.txt
test_results.log
test_out.log
AISynth.zip

# Per-user Claude Code state
.claude/settings.local.json
.claude/scheduled_tasks.lock
```

The patterns are deliberately narrow. `*.log`/`*.txt` would swallow `OFL.txt` and `AUTHORS.txt` inside `Source/UI/Fonts/` — Task 3 needs those tracked.

- [ ] **Step 2.3: Verify the patterns ignore exactly the intended files**

```bash
git check-ignore -v build.log build_output.txt test_results.log test_out.log AISynth.zip .claude/settings.local.json .claude/scheduled_tasks.lock
```

Expected: every line resolves to a `.gitignore:<lineno>:<pattern>` match.

```bash
git check-ignore -v Source/UI/Fonts/Instrument_Serif/OFL.txt Source/UI/Fonts/JetBrainsMono-2.304/AUTHORS.txt .claude/settings.json
```

Expected: empty output and exit code 1 (none of these are ignored).

- [ ] **Step 2.4: Remove stale scratch files from disk**

```bash
rm -f build.log build_output.txt build_step2.txt build_tests_only.txt build_tests_step4.txt \
      test_build.log test_build_err.txt test_build_out.txt test_out.log test_output.txt test_results.log \
      AISynth.zip
```

Expected: silent success. These files were never tracked, so the operation produces no git diff.

- [ ] **Step 2.5: Verify `git status` after gitignore + rm**

```bash
git status --short
```

Expected: `M  .gitignore` plus untracked files now restricted to:
- `Source/UI/Fonts/` (Task 3)
- `docs/superpowers/plans/2026-04-2[45]-*.md` (Task 4)
- `.claude/settings.json` (Task 5)

The 12 build/test scratch files MUST be gone. `.claude/scheduled_tasks.lock` and `.claude/settings.local.json` MUST NOT appear.

- [ ] **Step 2.6: Commit**

```bash
git add .gitignore
git commit -m "$(cat <<'EOF'
chore: gitignore build/test scratch and per-user Claude state

Adds narrow patterns for the .log / .txt / .zip artifacts left behind by
build-with-vs.bat output captures, plus the per-user .claude runtime
state (settings.local.json, scheduled_tasks.lock).

Patterns avoid bare *.log / *.txt to keep OFL.txt and AUTHORS.txt under
Source/UI/Fonts/ tracked.

Companion disk cleanup: rm -f of 12 stale scratch files at repo root.
Those files were never tracked so the cleanup carries no git diff.

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

Expected: commit succeeds. `git log --oneline -2` shows two commits on the branch.

---

## Task 3: Track project fonts

**Files:** Add `Source/UI/Fonts/Instrument_Serif/`, `Source/UI/Fonts/JetBrainsMono-2.304/`, `Source/UI/Fonts/inter/`.

These directories exist on disk and contain the .ttf files loaded at runtime by `Source/UI/Fonts.h`. The user added them locally; a fresh `git clone` lacks them and the plugin renders with fallback fonts.

- [ ] **Step 3.1: Sanity-check directory contents**

```bash
ls Source/UI/Fonts/
```

Expected: at least `Instrument_Serif`, `JetBrainsMono-2.304`, `inter` (subdirs) plus possibly licence files.

```bash
find Source/UI/Fonts -type f | wc -l
```

Expected: a positive number (~30+ depending on which Inter/JetBrains Mono weights are present). Report it for the commit body.

- [ ] **Step 3.2: Stage the directory**

```bash
git add Source/UI/Fonts/
```

- [ ] **Step 3.3: Confirm no scratch files leaked into the staging set**

```bash
git status --short | grep '^A '
```

Expected: every staged path starts with `Source/UI/Fonts/`. No `*.log`, `*.txt` outside that tree.

```bash
git diff --cached --stat | tail -3
```

Expected: a summary line like `XX files changed, YY insertions(+), 0 deletions(-)`. All inserts (binary fonts).

- [ ] **Step 3.4: Commit**

```bash
git commit -m "$(cat <<'EOF'
chore(ui): commit project fonts (Inter, JetBrains Mono, Instrument Serif)

Source/UI/Fonts.h loads these TTFs from disk at runtime; without them
tracked, a fresh clone falls back to system fonts and the plugin's
typography breaks. License files (OFL.txt, AUTHORS.txt) are included.

Font BinaryData embedding is still a TODO (see Source/UI/Fonts.h
TODO(font-embedding) markers); tracking on disk unblocks cloning in the
meantime.

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

Expected: commit succeeds.

---

## Task 4: Track Phase 3/4 design plans

**Files:** Add the 4 untracked plan files.

- [ ] **Step 4.1: List the plans currently in the directory**

```bash
ls docs/superpowers/plans/
```

Expected to include at minimum:
- `2026-04-24-handover.md`
- `2026-04-24-phase3-modmatrix.md`
- `2026-04-25-modengine.md`
- `2026-04-25-phase4-fx-slots.md`

(Possibly also Phase 5 / 6 plans already tracked — leave them.)

- [ ] **Step 4.2: Stage exactly those four plan files**

```bash
git add docs/superpowers/plans/2026-04-24-handover.md \
        docs/superpowers/plans/2026-04-24-phase3-modmatrix.md \
        docs/superpowers/plans/2026-04-25-modengine.md \
        docs/superpowers/plans/2026-04-25-phase4-fx-slots.md
```

- [ ] **Step 4.3: Confirm staging**

```bash
git diff --cached --name-only
```

Expected output: exactly those 4 paths.

- [ ] **Step 4.4: Commit**

```bash
git commit -m "$(cat <<'EOF'
chore(docs): commit Phase 3/4 plans and 2026-04-24 handover

Adds the design history that was sitting untracked in
docs/superpowers/plans/ since Phase 3. The handover doc plus the Phase 3
mod-matrix plan, the mod-engine plan, and the Phase 4 FX-slots plan.
Phase 5 plan was already tracked.

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

Expected: commit succeeds.

---

## Task 5: Track shared `.claude/settings.json`

**Files:** Add `.claude/settings.json` only. `.claude/settings.local.json` and `.claude/scheduled_tasks.lock` stay ignored (Task 2 added the patterns).

- [ ] **Step 5.1: Verify `.claude/settings.json` is not ignored, the others are**

```bash
git check-ignore -v .claude/settings.json
```

Expected: empty output, exit code 1 (not ignored).

```bash
git check-ignore -v .claude/settings.local.json .claude/scheduled_tasks.lock
```

Expected: both lines resolve to a `.gitignore:<lineno>` match (ignored).

- [ ] **Step 5.2: Stage**

```bash
git add .claude/settings.json
```

- [ ] **Step 5.3: Confirm only one file staged**

```bash
git diff --cached --name-only
```

Expected output: exactly `.claude/settings.json`.

- [ ] **Step 5.4: Commit**

```bash
git commit -m "$(cat <<'EOF'
chore(claude): commit shared .claude/settings.json

Tracks the shared Claude Code project settings so collaborators get the
same hooks/permissions baseline. Per-user overrides live in
.claude/settings.local.json (gitignored).

Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
EOF
)"
```

Expected: commit succeeds.

---

## Task 6: Final verification

**Files:** none. Verification only, no commit.

- [ ] **Step 6.1: `git status` clean**

```bash
git status --short
```

Expected: empty output. If anything appears, investigate before declaring done.

- [ ] **Step 6.2: Tree summary**

```bash
git log --oneline tech-debt-sweep ^main
```

Expected: 5 commits (Tasks 1–5), in order:
```
<sha> chore(claude): commit shared .claude/settings.json
<sha> chore(docs): commit Phase 3/4 plans and 2026-04-24 handover
<sha> chore(ui): commit project fonts (Inter, JetBrains Mono, Instrument Serif)
<sha> chore: gitignore build/test scratch and per-user Claude state
<sha> fix(engine): flush chord notes when arp mode changes mid-play
```

- [ ] **Step 6.3: Re-run tests on the final state**

```bash
./build/AISynthTests_artefacts/Release/AISynthTests.exe 2>&1 | tail -10
```

Expected: `0 fail` and **84** tests in the summary. If the test count is 83, Task 1's test was lost — investigate.

- [ ] **Step 6.4: Plugin still builds**

```bash
cmd //c '"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul && cmake --build build --config Release --target AISynth_VST3'
```

Expected: build succeeds. Force a target rebuild if mtime is stale (build-with-vs.bat silent-skip hazard — see CLAUDE.md memory).

- [ ] **Step 6.5: (optional, manual) Reaper smoke test**

User runs the plugin in Reaper, switches arp from Chord → any other mode mid-chord, confirms no hung notes. This step is a hands-on verification gate; skip when running unattended.

- [ ] **Step 6.6: Hand back**

Surface to the user:
- Branch `tech-debt-sweep` ready to merge to main (5 commits).
- Test count 83 → 84.
- Working tree clean, scratch artifacts ignored, fonts/plans/settings tracked.
- Suggest a single PR titled e.g. `chore: tech-debt sweep — chord stale fix + repo hygiene`.

The `superpowers:finishing-a-development-branch` skill is the appropriate next handoff.

---

## Self-review

**Spec coverage:**
- Spec §"Goal" — covered by Tasks 1 (engine) + 2/3/4/5 (working tree).
- Spec §"Engine fix" — Task 1 (steps 1.1–1.6), with the exact patch shown verbatim and the test reproducing the noteOff-set assertion.
- Spec §"Repo content / .gitignore additions" — Task 2 (steps 2.1–2.6).
- Spec §"Files to remove from working tree" — Task 2 step 2.4.
- Spec §"Files to track" — Tasks 3, 4, 5.
- Spec §"Migration / compat" — verified by Task 6 (no preset/param/CMake change).
- Spec §"Adjacent issue noticed" (disable path) — explicitly out of scope, not addressed.

**Placeholder scan:** every code block is verbatim, every command is concrete, every "expected output" is specific. No `// ...` ellipses in code, no "etc." in expected outputs, no "fix as needed" in steps.

**Type / signature consistency:** the test uses `ArpEngine::Mode::Chord` and `ArpEngine::Mode::Up` (matching `Source/Engine/ArpEngine.h`). The fix patch uses `Mode::Chord` (unqualified inside `ArpEngine.cpp` because of the `using namespace`-equivalent member context). Both match the existing code's style at lines 271 / 304 of the same file.

**No spec gaps found.** Plan is implementable as written.

---

## Execution Handoff

Plan complete and saved to `docs/superpowers/plans/2026-04-26-tech-debt-sweep.md`. Two execution options:

1. **Subagent-Driven (recommended for this codebase per Phase 4/5 precedent)** — I dispatch a fresh subagent per task, review between tasks. Deterministic, well-bounded scope, low risk of context pollution.
2. **Inline Execution** — Execute tasks in this session. Faster for a 6-task sweep with small diffs, but my conversation context grows.

Either works. Given the size (one engine fix + five chore commits), inline is plausible. Phase 4/5 preference was subagent-driven; defer to user.
