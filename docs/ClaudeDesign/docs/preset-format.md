# SynthVibe — Preset Format

Two files per preset. The `.synthvibe` file is what a DAW sees/loads. The `.synthvibe.json` sidecar is optional metadata used by the History drawer.

---

## `.synthvibe` (primary, XML via JUCE ValueTree)

```xml
<SynthVibePreset version="1" id="abc123-uuid" name="Breathing Dust">
  <State>
    <!-- APVTS::state exported as-is -->
    <PARAM id="master.vol" value="0.8"/>
    <PARAM id="osc1.wave" value="0"/>
    <PARAM id="osc1.level" value="0.82"/>
    <!-- ... all APVTS params ... -->
  </State>
  <Meta>
    <Author>user</Author>
    <Created ts="2026-04-21T14:33:02Z"/>
    <Modified ts="2026-04-21T14:33:02Z"/>
  </Meta>
</SynthVibePreset>
```

- Written by `juce::AudioProcessor::getStateInformation`.
- Compatible with DAW session save/load out of the box.
- No prompt text, no tags, no AI metadata — keeps DAW state compact.

## `.synthvibe.json` (sidecar, optional)

Lives next to the `.synthvibe` file with the same stem.

```json
{
  "id": "abc123-uuid",
  "name": "Breathing Dust",
  "prompt": "a dusty pad that breathes slowly",
  "tags": ["pad", "ambient", "warm"],
  "author": "user",
  "created": "2026-04-21T14:33:02Z",
  "modified": "2026-04-21T14:33:02Z",
  "parent": null,
  "ai": {
    "model": "claude-sonnet-4-5",
    "variation_of": "uuid-of-sibling-variation-or-null",
    "raw_response": { /* the full Claude response verbatim */ }
  },
  "history": [
    {
      "ts": "2026-04-21T14:33:02Z",
      "prompt": "a dusty pad that breathes slowly",
      "params_changed": ["osc1.wave", "filt.cutoff", "env.amp.release"]
    }
  ]
}
```

---

## History drawer data source

The drawer reads all sidecars in the `~/Documents/SynthVibe/History/` folder, indexed by `modified` ts descending. Search matches against `name`, `prompt`, `tags`.

### Directory layout

```
~/Documents/SynthVibe/
├─ Presets/         # user-saved named presets (both files)
│  ├─ My Pad.synthvibe
│  └─ My Pad.synthvibe.json
├─ Factory/         # bundled with plugin install
└─ History/         # every AI generation auto-saved (sidecar + state)
   ├─ 2026-04-21T14-33-02_breathing-dust.synthvibe
   ├─ 2026-04-21T14-33-02_breathing-dust.synthvibe.json
   └─ ...
```

- History files are capped at **500 entries**; oldest are pruned when exceeded.
- "Save" in the UI copies the current history entry to `Presets/` and lets the user rename.

---

## Versioning

`version="1"` in the XML root. When we bump schema:
- v2+ reader handles v1 input.
- v1 reader silently ignores unknown elements.
- Never rename v1 paramIds; only add new ones.

## Export / import

- **Export:** zips both files together as `.synthvibepack` for sharing.
- **Import:** unzip, write to `Presets/` if not already present (dedupe by uuid).
