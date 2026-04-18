# SynthVibe Phase 1 — Moteur riche + UI

**Date:** 2026-04-18  
**Scope:** Dual oscillator, 2 LFOs, Chorus FX, Arpeggiateur simple, UI layout horizontal

---

## Objectif

Transformer le moteur mono-oscillateur actuel en un synthétiseur polyvalent capable de couvrir basses, leads, pads et arps. Phase 1 se concentre entièrement sur le moteur audio et l'UI — l'AI layer viendra après.

---

## 1. Moteur audio

### 1.1 Dual oscillator

Chaque voix passe de 1 à 2 oscillateurs. La classe `Oscillator` existante est réutilisée sans modification — instanciée deux fois dans `Voice`.

**OscParams** (nouveau struct dans Voice.h) :
```cpp
struct OscParams {
    Waveform waveform  = Waveform::Saw;
    int      octave    = 0;       // -2..+2
    int      semitone  = 0;       // -12..+12
    float    detune    = 0.f;     // cents, -100..+100
    float    level     = 1.f;     // 0..1
};
```

**VoiceParams** remplace le champ oscillateur unique par :
```cpp
OscParams osc1;
OscParams osc2;
```

Dans `Voice::getNextSample()`, les deux oscillateurs sont sommés avec leurs levels respectifs avant d'entrer dans le filter.

Migration : les anciens paramètres `osc_waveform`, `osc_detune`, `osc_octave` deviennent `osc1_*`. `osc2_*` sont initialisés à des valeurs neutres (level 0 par défaut pour ne pas casser les presets existants).

### 1.2 Deux LFOs

Deux LFOs indépendants tournent per-sample dans `Voice::getNextSample()`.

**LfoDest** (nouvel enum dans Voice.h) :
```cpp
enum class LfoDest { Pitch, Filter, Amp, Detune };
```

**LfoParams** :
```cpp
struct LfoParams {
    Waveform shape = Waveform::Sine;
    float    rate  = 1.f;    // Hz, 0.01..20
    float    depth = 0.f;    // 0..1
    LfoDest  dest  = LfoDest::Pitch;
};
```

Les LFOs réutilisent la classe `Oscillator` en mode non-audio (fréquence basse). Chaque LFO produit une valeur -1..+1 scalée par `depth`, appliquée à sa destination :
- **Pitch** : offset en cents sur les deux oscillateurs
- **Filter** : offset additif sur filterCutoff (en Hz)
- **Amp** : multiplicateur sur le gain de sortie
- **Detune** : offset en cents sur osc1 seulement (effet chorus léger)

### 1.3 Migration buildVoiceParams()

`PluginProcessor::buildVoiceParams()` est étendu pour lire les 18 nouveaux paramètres osc1/osc2 et les 8 paramètres LFO1/LFO2 depuis l'APVTS.

---

## 2. FX

### 2.1 Chorus

Nouveau fichier `Source/FX/Chorus.h/.cpp` suivant exactement le pattern `Delay` :

```cpp
struct ChorusParams { float rate; float depth; float mix; };

class Chorus {
public:
    void prepare(const juce::dsp::ProcessSpec&);
    void setParams(const ChorusParams&);
    void process(juce::AudioBuffer<float>&);
    void reset();
};
```

Implémentation : delay line modulée par LFO interne (rate 0.1–5 Hz, depth = amplitude de modulation en ms, mix = dry/wet).

### 2.2 FXChain activée

`FXChain` (Source/FX/FXChain.h) est complétée pour contenir `Delay` + `Chorus` :

```cpp
class FXChain {
public:
    void prepare(const juce::dsp::ProcessSpec&);
    void setParams(const Delay::Params&, const ChorusParams&);
    void process(juce::AudioBuffer<float>&);
    void reset();
private:
    Delay  delay;
    Chorus chorus;
};
```

`Source/FX/FXChain.cpp` est décommenté dans CMakeLists.txt. `PluginProcessor` remplace l'appel direct à `delay` par `fxChain.process()`.

---

## 3. Arpeggiateur

### 3.1 ArpEngine

Nouveau fichier `Source/Engine/ArpEngine.h/.cpp`.

```cpp
class ArpEngine {
public:
    enum class Mode { Up, Down, UpDown, Random };

    struct Params {
        bool  enabled     = false;
        Mode  mode        = Mode::Up;
        float rate        = 0.25f;  // fraction de noire : 0.25=double croche, 0.5=croche, 1=noire
        int   octaveRange = 1;      // 1..4
    };

    void setParams(const Params&);
    void noteOn(int midiNote, float velocity);
    void noteOff(int midiNote);
    void process(juce::MidiBuffer& midi, int numSamples, double bpm, double sampleRate);
    void reset();
};
```

**Fonctionnement :**
- Maintient une liste des notes tenues (sorted by pitch).
- Génère des notes-on/off dans le `MidiBuffer` à intervalles calculés en samples : `samplesPerStep = (60.0 / bpm) * rate * sampleRate`.
- Modes : Up (ascending), Down (descending), UpDown (ping-pong), Random (shuffle à chaque cycle).
- OctaveRange : duplique les notes sur N octaves supplémentaires dans la liste avant de parcourir.
- Quand `enabled = false`, le buffer MIDI passe sans modification.

### 3.2 Intégration dans PluginProcessor

Dans `processBlock`, avant `synth.handleMidiMessage()` :
```cpp
double bpm = 120.0;
if (auto pos = getPlayHead()->getPosition())
    if (auto b = pos->getBpm()) bpm = *b;
arp.process(midiBuffer, buffer.getNumSamples(), bpm, getSampleRate());
```

---

## 4. Paramètres APVTS

Tous les nouveaux IDs dans `ParameterIDs.h`, tous les ranges dans `ParameterLayout.cpp`.

| Groupe | IDs |
|---|---|
| Osc 1 | `osc1_waveform`, `osc1_octave`, `osc1_semitone`, `osc1_detune`, `osc1_level` |
| Osc 2 | `osc2_waveform`, `osc2_octave`, `osc2_semitone`, `osc2_detune`, `osc2_level` |
| LFO 1 | `lfo1_shape`, `lfo1_rate`, `lfo1_depth`, `lfo1_dest` |
| LFO 2 | `lfo2_shape`, `lfo2_rate`, `lfo2_depth`, `lfo2_dest` |
| Chorus | `chorus_rate`, `chorus_depth`, `chorus_mix` |
| Arp | `arp_enabled`, `arp_mode`, `arp_rate`, `arp_octave_range` |

**Anciens paramètres migrés :** `osc_waveform` → `osc1_waveform`, `osc_detune` → `osc1_detune`, `osc_octave` → `osc1_octave`.

---

## 5. UI Layout

Layout horizontal classique (style Minimoog) — tout visible simultanément, pas d'onglets.

```
┌─────────────────────────────────────────────────────────────┐
│  [Preset selector]                          [Master volume]  │
├──────────────────────┬──────────────────────────────────────┤
│  OSC 1               │  OSC 2                               │
│  wave oct semi det   │  wave oct semi det lvl               │
├──────────────────────┴──────────┬────────────┬─────────────┤
│  FILTER                         │  AMP ENV   │  FILTER ENV  │
│  type  cutoff  res  envAmt      │  A D S R   │  A D S R     │
├─────────────────────────────────┴────────────┴─────────────┤
│  LFO 1                          │  LFO 2                    │
│  shape  rate  depth  dest       │  shape  rate  depth  dest │
├─────────────────────────────────┴───────────────────────────┤
│  DELAY  time fb mix  │  CHORUS  rate depth mix  │  ARP  on mode rate oct │
└─────────────────────────────────────────────────────────────┘
```

Couleurs par section définies dans `LookAndFeel.h` (jamais de literals dans l'editor).  
Composants UI : `KnobWithLabel` existant réutilisé, nouveaux `WaveformSelector` (combobox) et `LfoDestSelector`.

---

## 6. Fichiers modifiés / créés

**Modifiés :**
- `Source/Engine/Voice.h` / `Voice.cpp` — OscParams, LfoParams, dual osc, 2 LFOs
- `Source/Engine/SynthEngine.h` — inchangé (passe VoiceParams tel quel)
- `Source/Parameters/ParameterIDs.h` — nouveaux IDs
- `Source/Parameters/ParameterLayout.cpp` — nouveaux ranges
- `Source/PluginProcessor.cpp` — buildVoiceParams, FXChain, ArpEngine
- `Source/FX/FXChain.h` — implémentation complète
- `Source/UI/LookAndFeel.h` — nouvelles couleurs sections
- `Source/PluginEditor.h/.cpp` — nouveau layout
- `CMakeLists.txt` — décommenter FXChain.cpp, ajouter Chorus.cpp, ArpEngine.cpp

**Créés :**
- `Source/FX/Chorus.h` / `Chorus.cpp`
- `Source/FX/FXChain.cpp`
- `Source/Engine/ArpEngine.h` / `ArpEngine.cpp`

---

## Hors scope Phase 1

- Reverb, Distortion, LFO sync au tempo DAW
- AI layer / Claude API
- Sub oscillator
- Modulation matrix avancée
