# SynthVibe — AI Contract (Claude → Plugin)

How the plugin talks to Claude and what it expects back.

---

## Request

The plugin POSTs to `https://api.anthropic.com/v1/messages` (or a proxy) with:

```json
{
  "model": "claude-sonnet-4-5",
  "max_tokens": 2048,
  "system": "<SYSTEM PROMPT, see below>",
  "messages": [
    { "role": "user", "content": "a dusty pad that breathes slowly" }
  ],
  "tools": [{ "name": "set_patch", "input_schema": { ... see below ... } }],
  "tool_choice": { "type": "tool", "name": "set_patch" }
}
```

Forcing the `set_patch` tool guarantees a structured response.

### System prompt (template)

> You design sounds for SynthVibe, a polyphonic subtractive synth. Given a user's description, return a patch as a JSON object via the `set_patch` tool. Only use the parameter IDs listed in the schema. All float values are normalized 0..1. Integer choices use the index (0 = first option). Return 3 distinct variations by calling `set_patch` 3 times in separate <variation> blocks. Keep your response short.

---

## Response schema (the `set_patch` tool)

```json
{
  "type": "object",
  "properties": {
    "name":        { "type": "string", "maxLength": 32 },
    "description": { "type": "string", "maxLength": 120 },
    "tags":        { "type": "array", "items": { "type": "string" }, "maxItems": 4 },
    "params":      {
      "type": "object",
      "description": "Sparse map of paramId -> normalized value (0..1) or choice index.",
      "additionalProperties": { "type": "number" }
    }
  },
  "required": ["name", "params"]
}
```

### Rules

1. **Sparse is fine.** Model only returns params it wants to change. Unspecified params keep their current values.
2. **Values are always numbers.** Floats 0..1 for continuous params; integer 0..N-1 for choice params (plugin knows which is which by paramId).
3. **Unknown paramIds are silently ignored.** (Log a warning; don't fail.)
4. **Out-of-range values are clamped.**
5. **No nested objects** in `params` — flat dictionary only.

### Example response

```json
{
  "name": "Breathing Dust",
  "description": "Soft pad, slow LFO, warm filter",
  "tags": ["pad", "ambient", "warm"],
  "params": {
    "osc1.wave": 2,
    "osc2.wave": 0,
    "osc2.tune": 0.458,
    "filt.cutoff": 0.35,
    "filt.res": 0.22,
    "env.amp.attack": 0.72,
    "env.amp.release": 0.88,
    "lfo1.rate": 0.08,
    "mod.1.src": 1,
    "mod.1.dst": 6,
    "mod.1.amount": 0.62
  }
}
```

---

## Multi-variation flow

When the user clicks "Generate 4", the plugin asks Claude for 4 distinct variations in ONE message using parallel tool calls. The UI shows each as a card; picking one calls `apply_patch(variation)` locally.

---

## Apply semantics

1. Plugin looks up each paramId in its `APVTS`.
2. For continuous params, calls `beginChangeGesture()` → `setValueNotifyingHost()` with the new normalized value → `endChangeGesture()`.
3. UI tweens the knob visually over 400ms (pure cosmetic; the engine has already jumped).
4. Engine uses `LinearSmoothedValue` internally so the audible change is smooth regardless.

---

## Error handling

| Failure | Behavior |
|---|---|
| Network error | Toast "AI unavailable", modal stays open, retry button |
| Claude returns non-JSON | Log, show "couldn't parse result", don't apply |
| Timeout (>20s) | Cancel, show timeout message |
| Partial / malformed JSON | Apply the valid keys, warn on the rest |
| API key missing | Modal redirects to Settings → API Key |

---

## Privacy / cost notes

- API key stored via JUCE `PropertiesFile` in OS user config dir.
- Each request is ~500 tokens in, ~800 out → roughly $0.01–0.03 per generation at Sonnet rates.
- Consider local caching: same prompt text → return cached result for 10 minutes.
- Log prompts locally for the history drawer; never send prior prompts as context unless user opts in.

---

## Files to produce in Claude Code

- `Source/AI/ClaudeClient.h/.cpp` — HTTP + JSON round-trip
- `Source/AI/PatchApplier.h/.cpp` — takes a response, writes to APVTS
- `Source/AI/SystemPrompt.cpp` — the system prompt as a string, plus the full paramId index
- `Source/AI/ParamIdIndex.h` — generated list of all paramIds + types, for prompt injection
