# Phase A.1 — AI Layer Backend Design Spec

> **Scope:** the headless, UI-less Claude API client that translates a natural-language prompt into N synth-patch variations. Ships **no UI** — the headline feature (textbox + Generate button + variations strip) lives in the follow-up phase **A.2**. A.1 lays the testable foundation: HTTP roundtrip, system prompt + ParamIdIndex, tool-use parsing, APVTS application, API key storage, and tests (offline mocks + optional gated live).

**Source of truth (existing):** `docs/ClaudeDesign/docs/ai-contract.md` — request/response format, set_patch tool, sparse param map, apply semantics. This spec adapts the contract to A.1 (backend-only) and locks the engineering decisions left open in the contract.

**Goal sentence:** ship `Source/AI/*` with `ClaudeClient::requestPatches(prompt, N, callback)` end-to-end testable (mocks in CI, real Anthropic call locally if `ANTHROPIC_API_KEY` is set), without exposing any user-visible UI.

---

## What this phase does NOT ship

- Plugin UI for prompts, variations, or API-key entry → **Phase A.2**
- Local prompt cache (dedupe identical prompts within a TTL) → A.3 polish or later
- Prompt history / drawer → A.3 polish
- `Source/API/APIServer.h` (local HTTP REST) → **sub-layer B** (MCP)
- MCP server, MCP transport → **sub-layer B**
- Cost telemetry / dashboards / per-call logging beyond `juce::Logger` warnings → out
- Streaming responses → out (we use non-streaming JSON; tool-use guarantees structured output)
- Multiple chained turns / conversational memory → out (single-turn, stateless)

---

## Architecture

### Layers

```
caller (A.2 UI in the future, or unit tests today)
   │ requestPatches(prompt, N, callback)
   ▼
ClaudeClient                       ← public façade, message-thread API
   │   - assembles request body (model, system block, tools, tool_choice, messages)
   │   - reads API key via ApiKeyStore
   │   - delegates HTTP to a Transport (real: JuceHttpTransport; tests: FakeTransport)
   │   - parses tool_use blocks → std::vector<Variation>
   │   - posts callback to MessageManager (callback fires on message thread)
   │
   ├── Transport (interface)
   │     │ post(url, headers, body, timeoutMs) → Response{status, body}
   │     ▼
   │   JuceHttpTransport — wraps juce::URL + juce::WebInputStream
   │   FakeTransport     — test double, returns canned Response
   │
   ├── SystemPrompt::build() : juce::String
   │     └── concatenates static template + ParamIdIndex::renderForPrompt()
   │
   ├── ParamIdIndex (manual std::vector<ParamEntry>)
   │     └── drift-test asserts parity with APVTS at runtime
   │
   └── ApiKeyStore (juce::JSON-backed file in user app data dir)

PatchApplier(apvts).apply(variation) → ApplyReport
   │   - message-thread only (jassert)
   │   - per-param: beginChangeGesture / setValueNotifyingHost / endChangeGesture
   │   - unknown IDs counted; out-of-range values clamped
```

### Files (new)

| File | Lines (est.) | Responsibility |
|---|---|---|
| `Source/AI/Variation.h` | ~25 | POD struct shared by Client + Applier |
| `Source/AI/ClaudeClient.h` | ~80 | Public API, enums, structs |
| `Source/AI/ClaudeClient.cpp` | ~300 | Request body, parsing, threading, cancellation |
| `Source/AI/Transport.h` | ~25 | Interface + Response struct |
| `Source/AI/JuceHttpTransport.h` | ~15 | Header-only stub for the real transport |
| `Source/AI/JuceHttpTransport.cpp` | ~80 | `juce::WebInputStream` wrapping |
| `Source/AI/PatchApplier.h` | ~30 | Public API + ApplyReport |
| `Source/AI/PatchApplier.cpp` | ~80 | APVTS gestures, clamping, unknown counting |
| `Source/AI/SystemPrompt.h` | ~10 | `juce::String build()` |
| `Source/AI/SystemPrompt.cpp` | ~60 | Template + index rendering |
| `Source/AI/ParamIdIndex.h` | ~30 | `ParamEntry` struct + namespace API |
| `Source/AI/ParamIdIndex.cpp` | ~250 | Manual list of all ~185 APVTS paramIds + metadata |
| `Source/AI/ApiKeyStore.h` | ~20 | Public API |
| `Source/AI/ApiKeyStore.cpp` | ~60 | `juce::JSON` r/w (small `{anthropic_api_key:...}` file) |
| **Tests:** | | |
| `Tests/ClaudeClientTests.cpp` | ~400 | 9 mocked test blocks |
| `Tests/PatchApplierTests.cpp` | ~250 | 6 test blocks |
| `Tests/SystemPromptTests.cpp` | ~80 | 3 test blocks |
| `Tests/ParamIdIndexDriftTests.cpp` | ~100 | 3 test blocks (drift gate) |
| `Tests/ClaudeClientLiveTests.cpp` | ~100 | 1 gated test block |
| `Tests/FakeTransport.h` | ~50 | Test double, header-only |

### Files (modified)

| File | Change |
|---|---|
| `CMakeLists.txt` | Add `Source/AI/*.cpp` to plugin target sources; add 5 new test files + `FakeTransport.h` IDE listing to test target |

No CMake `find_package` additions — everything uses JUCE built-ins (`juce::URL`, `juce::WebInputStream`, `juce::JSON`, `juce::Thread`, `juce::MessageManager`).

---

## Public API surface

### `Source/AI/Variation.h`

```cpp
#pragma once
#include <juce_core/juce_core.h>
#include <map>

struct Variation {
    juce::String name;                          // Claude-supplied (≤ 32 chars)
    juce::String description;                   // optional, ≤ 120 chars
    juce::StringArray tags;                     // optional, ≤ 4 entries
    std::map<juce::String, double> params;      // sparse paramId → normalized value or choice index
};
```

### `Source/AI/ClaudeClient.h`

```cpp
#pragma once
#include "Variation.h"
#include "Transport.h"
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <functional>
#include <atomic>
#include <memory>

class ApiKeyStore;

enum class ClaudeClientError {
    None,
    MissingApiKey,
    NetworkFailure,
    Timeout,
    HttpError,        // non-2xx; httpStatus filled
    ParseError,       // JSON malformed
    SchemaError       // valid JSON but no usable set_patch tool_use blocks
};

struct ClaudeResponse {
    ClaudeClientError error = ClaudeClientError::None;
    int httpStatus = 0;
    juce::String message;                       // human-readable (used by A.2)
    std::vector<Variation> variations;          // empty if error != None
};

class ClaudeClient {
public:
    // `transport` ownership: caller. Lifetime must exceed this client.
    // (Tests inject FakeTransport; production wires JuceHttpTransport.)
    ClaudeClient(ApiKeyStore& keyStore, Transport& transport);
    ~ClaudeClient();

    // Async: callback runs on the message thread.
    // numVariations: default 4, capped to [1, 8].
    // Subsequent calls supersede in-flight requests (the older callback is dropped).
    void requestPatches(juce::String prompt,
                        int numVariations,
                        std::function<void(ClaudeResponse)> callback);

    void setModel(juce::String model);   // default "claude-sonnet-4-6"
    void setTimeoutSeconds(int seconds); // default 20

private:
    // ... worker thread, std::atomic generation counter for cancellation, etc.
};
```

### `Source/AI/Transport.h`

```cpp
#pragma once
#include <juce_core/juce_core.h>

struct HttpResponse {
    int status = 0;                  // 0 = network failure
    juce::String body;               // empty on network failure
    bool timedOut = false;
};

class Transport {
public:
    virtual ~Transport() = default;
    virtual HttpResponse post(const juce::URL& url,
                              const juce::StringPairArray& headers,
                              const juce::String& body,
                              int timeoutMs) = 0;
};
```

### `Source/AI/JuceHttpTransport.h` / `.cpp`

```cpp
class JuceHttpTransport : public Transport {
public:
    HttpResponse post(const juce::URL&, const juce::StringPairArray&,
                      const juce::String& body, int timeoutMs) override;
};
```

Implementation: `juce::WebInputStream` with `withConnectionTimeoutMs(timeoutMs)`, `withExtraHeaders(headers as "K: V\r\n" string)`, `withCustomBody(body)`. Reads the entire response body to a `juce::String`. On null/exception, returns `{status:0, body:"", timedOut:false}` for network failure or `timedOut:true` if `juce::WebInputStream` reports timeout.

### `Source/AI/PatchApplier.h`

```cpp
#pragma once
#include "Variation.h"
#include <juce_audio_processors/juce_audio_processors.h>

struct ApplyReport {
    int applied = 0;
    int unknown = 0;       // paramIds not in APVTS
    int clamped = 0;       // values clamped to range
};

class PatchApplier {
public:
    explicit PatchApplier(juce::AudioProcessorValueTreeState& apvts);

    // MUST be called on the message thread (jassert checks).
    ApplyReport apply(const Variation& variation);

private:
    juce::AudioProcessorValueTreeState& apvts;
};
```

Per-param algorithm:

```cpp
for (auto& [id, value] : variation.params) {
    auto* p = apvts.getParameter(id);
    if (! p) { ++report.unknown; continue; }

    p->beginChangeGesture();
    const float clamped = clampToRange(*p, value);          // counts ++report.clamped if value != clamped
    p->setValueNotifyingHost(p->convertTo0to1(clamped));     // normalized 0..1
    p->endChangeGesture();
    ++report.applied;
}
```

`clampToRange`:
- For `RangedAudioParameter`: `range = p->getNormalisableRange();` clamp to `[range.start, range.end]`.
- The Claude contract says continuous params arrive as 0..1 normalized. The applier accepts BOTH 0..1-normalized floats AND raw choice indices (0..N-1) because the Claude contract returns both shapes by paramId type. Detection: if the parameter is `AudioParameterChoice` or `AudioParameterInt`, the value is treated as a raw index; otherwise (`AudioParameterFloat`/`AudioParameterBool`) it's treated as 0..1.

### `Source/AI/SystemPrompt.h` / `.cpp`

```cpp
namespace SystemPrompt {
    juce::String build();
}
```

`build()` returns:

```
You design sounds for SynthVibe, a polyphonic subtractive synth with mod
matrix and FX chain. Given a user's description, return between 1 and 4
distinct patches via parallel set_patch tool calls.

Rules:
- Use only paramIds listed below.
- Float params: value in [0..1] (normalized).
- Choice / Int params: integer index 0..(N-1) where 0 = first option.
- Bool params: 0 or 1.
- Output sparse: only include params you want to change.
- Do not nest objects.

Available parameters:
<ParamIdIndex::renderForPrompt() output>
```

`ParamIdIndex::renderForPrompt()` produces a markdown-style table:

```
| id              | label              | type   | range / choices                       | default |
|-----------------|--------------------|--------|---------------------------------------|---------|
| osc1.wave       | OSC 1 Waveform     | choice | sine, saw, square, triangle           | 0       |
| osc1.level      | OSC 1 Level        | float  | 0..1                                  | 0.7     |
| filt.cutoff     | Filter Cutoff      | float  | 0..1                                  | 0.5     |
| ...             | ...                | ...    | ...                                   | ...     |
```

`build()` deterministic: same string every call within a process. Wrapped in `cache_control: ephemeral` in the request body so Anthropic caches the prefix.

### `Source/AI/ParamIdIndex.h` / `.cpp`

```cpp
struct ParamEntry {
    juce::String id;
    juce::String label;
    enum class Type { Float, Choice, Bool, Int } type;
    double minValue = 0.0;
    double maxValue = 1.0;
    double defaultValue = 0.0;
    juce::StringArray choices;   // populated only for Type::Choice
};

namespace ParamIdIndex {
    const std::vector<ParamEntry>& entries();          // singleton list
    const ParamEntry* find(const juce::String& id);    // nullptr if missing
    juce::String renderForPrompt();                    // markdown table
}
```

`entries()` returns a static const list initialised once. The list is **maintained by hand** in `ParamIdIndex.cpp`. Drift between this list and the actual APVTS is caught by `Tests/ParamIdIndexDriftTests.cpp` at CI time.

When a developer adds a param to `ParameterLayout.cpp`, the drift test fails until they add a corresponding `{...}` entry here. This is the trade we accepted (vs a build-time Python generator): one slightly-tedious copy-paste per new param, in exchange for zero extra build-system complexity.

### `Source/AI/ApiKeyStore.h` / `.cpp`

```cpp
class ApiKeyStore {
public:
    ApiKeyStore();                              // production: uses default user config path
    explicit ApiKeyStore(juce::File path);      // tests: inject a temp file for hermetic CI

    juce::String load() const;                  // empty if not set
    void save(const juce::String& key);         // writes to the path
    void clear();
    juce::File getStorePath() const;            // for tests + future Settings UI
};
```

Storage path: `juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory) / "AISynth" / "settings.json"`. Format: a tiny `juce::var` JSON object `{"anthropic_api_key": "<key>"}`. Created lazy on first save. **Never logged**, never committed (path is per-OS user dir, outside the repo).

Tests use a temp file via `juce::File::createTempFile`; a test-only constructor takes an explicit `juce::File path` to keep CI hermetic.

---

## Threading + lifecycle

### Request flow

1. Caller (A.2 UI later, or test today) invokes `requestPatches(prompt, 4, callback)` on the **message thread**.
2. ClaudeClient increments an internal `std::atomic<uint64_t> generation`; captures the current generation as `myGen`.
3. ClaudeClient enqueues work onto a **single worker `juce::Thread`** that runs at most one HTTP call at a time. If the worker is already running, the previous gen's callback is implicitly stale (will be discarded on completion).
4. Worker thread:
   - Reads the API key via `ApiKeyStore::load()`. If empty → posts `{error: MissingApiKey}` back to message thread (via `MessageManager::callAsync`) and exits.
   - Builds request body (JSON via `juce::JSON::toString` of a manually-constructed `juce::DynamicObject`).
   - Calls `transport.post(...)` — blocks worker.
   - On non-2xx → posts `{error: HttpError, status}`.
   - On null/exception → posts `{error: NetworkFailure}` or `{Timeout}`.
   - On 2xx body: parses JSON. Extracts `content[]` array. Filters items where `type == "tool_use"` AND `name == "set_patch"`. For each, parses `input.params` into `Variation::params`. If zero parsed → `SchemaError`. If some parsed → return what we got, error stays `None`. If body is unparseable JSON → `ParseError`.
5. Worker calls `juce::MessageManager::callAsync([gen=myGen, callback, response, this]{ if (gen == this->liveGen) callback(response); })`.
6. The lambda checks the captured generation against the current `liveGen` member — if a newer request superseded this one, the callback is silently dropped.

### Cancellation / supersession

Each `requestPatches` call increments `generation`. The completion lambda checks if its captured `myGen` still matches `liveGen` (set by the latest request). If not, callback is dropped. No transport-level `cancel()`; the in-flight HTTP completes naturally but its result is discarded. Acceptable for A.1 — we don't have aggressive use-cases yet (UI button is debounced in A.2).

### Destructor safety

`~ClaudeClient` sets `shutdownFlag = true`, calls `worker.signalThreadShouldExit()` and `worker.waitForThreadToExit(2000)`. The completion lambda checks `shutdownFlag` (in addition to generation) before invoking the callback. If shutdownFlag is true, callback is never invoked. Tests cover this (test #9 in the table above).

---

## Request body

Exact shape sent to `https://api.anthropic.com/v1/messages`:

```json
{
  "model": "claude-sonnet-4-6",
  "max_tokens": 2048,
  "system": [
    {
      "type": "text",
      "text": "<SystemPrompt::build() output, ~5–8k tokens depending on param count>",
      "cache_control": { "type": "ephemeral" }
    }
  ],
  "messages": [
    { "role": "user", "content": "<the user prompt verbatim>" }
  ],
  "tools": [
    {
      "name": "set_patch",
      "description": "Apply a synth patch as a sparse map of paramId -> value.",
      "input_schema": {
        "type": "object",
        "properties": {
          "name":        { "type": "string", "maxLength": 32 },
          "description": { "type": "string", "maxLength": 120 },
          "tags":        { "type": "array",  "items": { "type": "string" }, "maxItems": 4 },
          "params":      {
            "type": "object",
            "description": "Sparse map of paramId -> normalized value (0..1) or choice index.",
            "additionalProperties": { "type": "number" }
          }
        },
        "required": ["name", "params"]
      }
    }
  ],
  "tool_choice": { "type": "any" }
}
```

Headers:
- `x-api-key: <key from ApiKeyStore>`
- `anthropic-version: 2023-06-01`
- `content-type: application/json`

**Why `tool_choice: any` rather than `{type:"tool", name:"set_patch"}`** — to permit Claude to emit **multiple parallel** `set_patch` tool_use blocks in a single response (the model can call the same tool N times when "any" is set + the system prompt asks for N variations). With `{type:"tool"}` plus `disable_parallel_tool_use:false`, multi-call behaviour is also possible but less robust in practice. `tool_choice: any` is the documented Anthropic pattern for forcing tool use while allowing parallelism.

**Why `claude-sonnet-4-6` (not 4.5 from `ai-contract.md`)** — 4.5 is retired. 4.6 is the current Sonnet, cost/speed sweet spot for this task. Configurable via `setModel()`; A.2 Settings UI will expose it.

---

## Tests (full list)

**`Tests/ClaudeClientTests.cpp`** — 9 blocks:

1. `requestPatches with 4 valid tool_use blocks parses 4 Variations` — feed canned response; assert `variations.size() == 4`, names + params populated.
2. `requestPatches with 1 valid tool_use block returns 1 Variation` — Claude can return fewer; not an error.
3. `tool_use blocks with name != "set_patch" are filtered out` — feed mixed block list; only set_patch parsed.
4. `response with no tool_use blocks returns SchemaError` — text-only response.
5. `malformed JSON body returns ParseError` — half-string `{`.
6. `non-2xx HTTP returns HttpError with status` — feed status 401 and 500; assert `httpStatus`.
7. `network failure returns NetworkFailure` — Transport returns `{status:0}`.
8. `timeout returns Timeout` — Transport returns `{timedOut:true}`.
9. `missing API key returns MissingApiKey, transport not called` — `ApiKeyStore::clear()`, instrument FakeTransport with a counter, assert it stayed 0.
10. `superseded request: only 2nd callback fires` — start a slow request, then a fast one; assert 1st callback never fired, 2nd did, with the 2nd's body.
11. `destructor while in-flight: callback dropped` — start request, destroy client, assert callback was never invoked even though Transport ran to completion.

(Renumbered: 11 blocks total, not 9 — corrected from Section 3's table.)

**`Tests/PatchApplierTests.cpp`** — 6 blocks:

12. `applies known floats correctly` — `{osc1.level: 0.7, filt.cutoff: 0.3}`, assert APVTS reads back.
13. `applies known choice index correctly` — `{osc1.wave: 2}`, assert APVTS choice value == 2.
14. `unknown IDs are counted, not crashing` — `{bogus.id: 0.5}`, assert `report.unknown == 1`, `applied == 0`.
15. `out-of-range float clamped` — `{osc1.level: 1.5}` → 1.0, `report.clamped == 1`.
16. `gestures fire (1 begin + 1 end per applied param)` — `juce::AudioProcessorParameter::Listener` counts; for 3 applied params, expect 3 begin and 3 end events.
17. `empty params is no-op` — `report.applied == 0`, no listeners triggered.

**`Tests/SystemPromptTests.cpp`** — 3 blocks:

18. `build() returns non-empty string` — sanity.
19. `prompt contains template anchors` — string contains "set_patch", "Sparse", "0..1", "choice".
20. `prompt contains every ParamIdIndex entry id` — for each `entry.id`, assert `prompt.contains(entry.id)`.

**`Tests/ParamIdIndexDriftTests.cpp`** — 3 blocks:

21. `every APVTS paramId has a ParamIdIndex entry` — instantiate APVTS via the existing factory, iterate `getParameters()`, fail if any is not in `ParamIdIndex::entries()`.
22. `every ParamIdIndex entry has an APVTS counterpart` — reverse direction; catches stale entries after a param removal.
23. `Type::Choice entries have choices.size() == int(maxValue + 1)` — internal consistency.

**`Tests/ClaudeClientLiveTests.cpp`** — 1 block, gated:

24. `live request with N=1 returns a parseable Variation` — guarded by `if (juce::SystemStats::getEnvironmentVariable("ANTHROPIC_API_KEY", "").isEmpty()) return;`. Issues a real POST with prompt "warm pad with slow attack". Asserts `error == None`, `variations.size() >= 1`, `variations[0].params` is non-empty, and at least one paramId in `variations[0].params` is found via `ParamIdIndex::find(...)`.

Test count delta: **+24 blocks** (84 → 108 post-A.1). (Spec Section 3 said +22; recount = 24 because `ClaudeClientTests.cpp` got two extra renumbered cases: `tool_use filtered by name`, and the supersession case was previously merged.)

---

## Error handling matrix

| Failure | A.1 returned `ClaudeResponse` | A.2 binding (ref. only) |
|---|---|---|
| `ApiKeyStore::load()` returns empty | `{error: MissingApiKey, message: "no API key set"}`, no HTTP | redirect to Settings panel |
| `juce::WebInputStream` is null OR throws | `{error: NetworkFailure, message: "could not reach Anthropic API"}` | "AI unavailable" toast + retry |
| HTTP status not in [200, 299] | `{error: HttpError, httpStatus: <s>, message: "<body excerpt>"}` | format with status (401/429/5xx have specific copy) |
| Connection time exceeds `timeoutSeconds` | `{error: Timeout, message: "timed out (Ns)"}` | "timed out" toast |
| Body not parseable as JSON | `{error: ParseError, message: "<excerpt>"}` | "couldn't parse Claude's response" |
| JSON parsed, zero `set_patch` tool_use blocks | `{error: SchemaError, message: "Claude returned no patches"}` | "no patches generated" |
| JSON parsed, ≥1 valid blocks + ≥1 malformed | `{error: None, variations: [<valid>], message: ""}`. Malformed entries logged via `juce::Logger::warning`. | optional warn toast |

---

## Migration / compat

- **No preset format change.** No APVTS schema change.
- **No new build dependency.** Everything uses JUCE built-ins.
- **No commit-tracked secrets.** API key in user config dir only; gitignore not affected.
- **CMake additive.** Plugin target gains 12 new `.cpp` files; test target gains 5 new `.cpp` files. No removals.
- **Phase A.2 contract:** A.2's UI uses `ClaudeClient::requestPatches(...)` and `PatchApplier::apply(...)` through their public headers. The signatures locked here are the wire between A.1 and A.2.

---

## Self-review

- **Placeholder scan:** every API has explicit signatures, every test has explicit asserts. No "TBD". No "appropriate error handling" — error type is enumerated.
- **Internal consistency:** `ClaudeClient` constructor takes `Transport&` by reference (caller-owned). `JuceHttpTransport` is the production impl. `FakeTransport` is the test impl. Both satisfy the same `Transport` interface. Test count recount = 24 blocks (corrected from Section 3's brainstorming-time estimate of 22).
- **Scope check:** strictly backend. The deliverable (`ClaudeClient::requestPatches` callable from a unit test, returning parsed Variations or a typed error) is achievable in a single 8–10 commit phase. UI deferred. MCP deferred.
- **Ambiguity check:**
  - "tool_choice: any" — explicitly chosen over `{type:"tool",name:"set_patch"}` to maximise reliability of multiple-variation responses.
  - "set_patch params semantic: 0..1 vs raw index" — locked: PatchApplier inspects the APVTS parameter's runtime type (`AudioParameterChoice`/`AudioParameterInt` → raw index; `AudioParameterFloat`/`AudioParameterBool` → 0..1). Documented.
  - "Threading" — single worker thread, supersession via atomic generation counter, callback dispatched via `MessageManager::callAsync`. Locked.
  - "Drift between APVTS and ParamIdIndex" — caught by `Tests/ParamIdIndexDriftTests.cpp` at CI time. Manual upkeep accepted.
  - "Storage path for API key" — `userApplicationDataDirectory / AISynth / settings.json`. Tests use temp file via constructor overload. Locked.

---

## Execution Handoff

Next step: invoke `superpowers:writing-plans` to produce the detailed task-by-task implementation plan at `docs/superpowers/plans/2026-04-26-ai-layer-a1.md`. The plan will sequence ~10 commits in TDD order (start with `ParamIdIndex` + drift test, then `SystemPrompt`, then `ApiKeyStore`, then `Transport` + `JuceHttpTransport`, then `ClaudeClient` happy path, then error branches, then cancellation, then `PatchApplier`, then live test, then CMake wiring).

Execution mode (subagent-driven vs inline) is your call after the plan is written. Given size (~10 commits, several .cpp files with non-trivial logic), **subagent-driven is recommended** — fresh subagent per task gives clean separation, easier review of the parsing/threading logic where mistakes are subtle.
