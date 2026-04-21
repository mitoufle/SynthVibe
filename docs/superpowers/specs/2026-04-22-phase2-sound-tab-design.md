# SynthVibe Phase 2 — Sound Tab visual rewrite

**Date:** 2026-04-22
**Scope:** Replace the current `SoundTab` with the mockup's layout using 7 new atomic UI components, refine the ADSR envelope to exponential curves, and scaffold the responsive breakpoint API. Engine features from the roadmap (osc phase/PWM, filter drive, keytrack, osc2 unison, LP12/Notch filter types) are **deferred to Phase 2b**.

**Companion documents:**
- Roadmap: `docs/superpowers/plans/2026-04-21-design-roadmap.md`
- Mockup source of truth: `docs/ClaudeDesign/AISynth V1 Hi-Fi.html`
- Component inventory: `docs/ClaudeDesign/docs/components.md`

---

## Objectif

Phase 1 a posé les tokens, LookAndFeel, Fonts et le TopBar. Les tabs internes (SoundTab en premier) tournent encore avec l'ancienne esthétique `KnobWithLabel` + legacy colour aliases. Phase 2 remplace SoundTab entièrement :

- Mêmes 3 colonnes mais layout et composants conformes au mockup
- 7 nouveaux atomic components réutilisables (seront consommés aussi par Phase 3/4)
- Envelope ADSR passé en courbes exponentielles analog-style (refinement isolé)
- Introduction du scaffold `enum class BP` pour responsive (implémentation complète = Phase 5)

Phase 2 ne touche **aucun paramètre APVTS** (tous déjà présents depuis Phase 1 Task 0). Aucune nouvelle feature audio. Les knobs manquants du mockup (`osc1.phase`, `osc1.pwm`, `filt.drive`, `filt.keytrack`, bloc unison osc2) sont **omis** du layout Phase 2 et ajoutés par Phase 2b en même temps que leur engine.

---

## 1. Couches & fichiers

### 1.1 Component layer — `Source/UI/components/`

Sept nouveaux headers. Tous `#include "../DesignTokens.h"` et `"../Fonts.h"`.

| Fichier | Responsabilité | Data source |
|---|---|---|
| `OscilloscopeView.h` | Rendu forme d'onde de l'oscillateur, drift ambiant au repos | Synthétique depuis le choice param `oscN.wave` (pas d'audio tap) |
| `WaveTypeSelect.h` | Pill-dropdown 4 items (Sine/Saw/Square/Triangle) | `ComboBoxAttachment` sur `oscN.wave` |
| `FilterResponseView.h` | Courbe de réponse fréquentielle live | Synthétique : équation biquad depuis `filt.{type,cutoff,res}` |
| `FilterTypeSelect.h` | Row de 3 pills (LP / HP / BP) mutuellement exclusives | `ParameterAttachment` sur `filt.type` |
| `EnvelopeEditor.h` | 4 nodes ADSR drag-edit, rendu des courbes exponentielles | 4 `ParameterAttachment` sur `env.{amp,filt}.{attack,decay,sustain,release}` |
| `PanelHeader.h` | Section-dot coloré + titre uppercase, bande horizontale | Props (colour, title) |
| `KnobTooltip.h` | Readout flottant durant le drag d'un `ArcKnob` | Callback depuis ArcKnob |

### 1.2 Layout layer — `Source/UI/SoundTab.h`

Rewrite complet. Toujours 3 colonnes. Toujours stacked dans la 3e colonne (Filter en haut, AmpEnv milieu, FltEnv bas). Mais :
- `KnobWithLabel` → `SynthVibe::ArcKnob`
- Combos (`osc{1,2}WaveBox`, `filterTypeBox`) → `WaveTypeSelect` / `FilterTypeSelect`
- Ajout d'`OscilloscopeView` au-dessus des knobs dans chaque panel OSC
- Ajout d'`FilterResponseView` dans le panel Filter
- Remplacement des 4 knobs ADSR de chaque panel env par un `EnvelopeEditor` (les knobs restent disponibles en lecture sous forme de readout inline dans l'editor — pas de doublon visuel)
- Chaque panel gagne un `PanelHeader` au lieu du `drawPanel()` inline
- Nouvelle méthode `layoutFor(SynthVibe::BP)` — tous les setBounds passent par là
- `paint()` n'a plus rien à faire (les headers paintent leur propre titre)

### 1.3 Engine refinement — `Source/Engine/Envelope.cpp`

Changement ciblé des stages Decay et Release. L'Attack reste linéaire (comportement le plus punchy, standard analog).

**Math** :
```cpp
// Pré-calculé dans setParams (pour éviter exp() per-sample) :
const float decayCoeff   = std::exp(-1.f / (std::max(0.0001f, params.decay)   * sr));
const float releaseCoeff = std::exp(-1.f / (std::max(0.0001f, params.release) * sr));

// Dans getNextSample, Decay :
currentLevel = params.sustain + (currentLevel - params.sustain) * decayCoeff;
if (currentLevel - params.sustain < 1e-5f) { currentLevel = params.sustain; stage = Stage::Sustain; }

// Release :
currentLevel *= releaseCoeff;
if (currentLevel < 1e-5f) { currentLevel = 0.f; stage = Stage::Idle; }
```

**Changement de sémantique** : le param `decay`/`release` devient le time-constant τ (temps pour atteindre ~63 % de la descente finale), plus "time to 99%". Le comportement subjectif reste "plus lent = plus long" — pas de re-tuning presets nécessaire (les presets factory ont déjà été supprimés par l'utilisateur).

**setParams doit recalculer les coeffs** à chaque changement ; il est déjà appelé depuis `buildVoiceParams()` avant chaque block, donc pas de nouveau plumbing.

**Tests de non-régression** : ajouter un test `ExponentialEnvelopeTests` qui vérifie qu'en sustain à 0.5, après 4×τ de decay, `currentLevel` est à ~2 % au-dessus de sustain (exp(-4) ≈ 0.018).

### 1.4 Breakpoint scaffold — `Source/PluginEditor.{h,cpp}`

Dans un nouveau header `Source/UI/Breakpoint.h` :
```cpp
namespace SynthVibe {
    enum class BP { Compact, Default, Wide };
    inline BP breakpointForWidth(int w) {
        return w < 1150 ? BP::Compact : w < 1550 ? BP::Default : BP::Wide;
    }
}
```

`PluginEditor::resized()` calcule le BP, puis appelle `soundTab.layoutFor(bp)` (nouvelle méthode). Les autres tabs gardent leur `resized()` inchangé — ils verront BP arriver quand ils seront réécrits.

Phase 2 **n'implémente pas** de layouts différents pour Compact/Wide. Les 3 branches appellent le même code pour l'instant. L'infra est là ; Phase 5 la remplira.

---

## 2. Data sources & rendering details

### 2.1 OscilloscopeView — rendu synthétique

`paint()` échantillonne une période de la forme d'onde (64 points suffisent) depuis le waveform courant :
- Sine → `sin(2π·t)`
- Saw → `2t - 1`
- Square → `t < 0.5 ? 1 : -1`
- Triangle → `4·|t - 0.5| - 1`

Le drift ambiant : `t += (deltaTime / Tokens::Motion::ambientScopeMs)` via un `juce::Timer` à 30 Hz, modulo 1. Pas d'audio, pas de ring buffer, pas de concurrence thread audio/UI.

Couleur du trait : `Tokens::osc`. Épaisseur : 1.5 px. Glow léger (`Path` strokée deux fois, seconde passe alpha 0.3).

### 2.2 FilterResponseView — biquad analytique

La courbe affichée est la réponse en fréquence théorique d'un biquad second-ordre, calculée en 120 points logarithmiquement répartis entre 20 Hz et sampleRate/2. Formule standard :
```
H(ω) = (b0 + b1·z⁻¹ + b2·z⁻²) / (1 + a1·z⁻¹ + a2·z⁻²)
```
avec les coefficients biquad calculés depuis `filt.cutoff` (fc), `filt.res` (Q) et `filt.type` (LP/HP/BP) selon les RBJ cookbook formulas — même code que `Source/Engine/Filter.cpp`.

Pour éviter la duplication, extraire ces calculs dans `Source/Engine/FilterCoefficients.h` (nouveau) et le faire consommer par Filter.cpp **et** FilterResponseView. Ce refactor est inclus dans Phase 2.

### 2.3 EnvelopeEditor — interaction + rendu

**Rendu** : 4 nodes (attack-peak, decay-corner, sustain-corner-release-start, release-end). Le chemin entre nodes reflète exactement ce que l'engine produit :
- Attack : segment linéaire depuis (0, 0) vers (tAttack, 1)
- Decay : courbe exponentielle `y(t) = sustain + (1 - sustain) · exp(-t/τ_decay)` depuis (tAttack, 1) vers (tAttack + visualDecayWindow, sustain)
- Sustain : ligne horizontale à `sustain` jusqu'au point de release
- Release : `y(t) = sustain · exp(-t/τ_release)` depuis (releaseStart, sustain) vers (releaseEnd, 0)

**Interaction** :
- Drag du node 1 (attack peak) → écrit `attack` param via `ParameterAttachment::setValueAsCompleteGesture`
- Drag du node 2 (decay corner) → écrit `decay` (via X) et `sustain` (via Y)
- Drag du node 3 (sustain end) → écrit seulement `sustain` (Y)
- Drag du node 4 (release end) → écrit `release` (X)

Les 4 knobs ADSR existants restent accessibles ailleurs dans le flow (mod matrix, AI layer). L'editor et les knobs réfèrent aux mêmes params via APVTS ; ils se synchronisent naturellement.

**Readout inline** : en-dessous de la courbe, une ligne `a 12ms · d 120ms · s 0.50 · r 200ms` en `Fonts::mono` taille `Font::label`.

---

## 3. Fichiers

### Nouveaux
```
Source/UI/Breakpoint.h
Source/UI/components/OscilloscopeView.h
Source/UI/components/WaveTypeSelect.h
Source/UI/components/FilterResponseView.h
Source/UI/components/FilterTypeSelect.h
Source/UI/components/EnvelopeEditor.h
Source/UI/components/PanelHeader.h
Source/UI/components/KnobTooltip.h
Source/Engine/FilterCoefficients.h
Tests/ExponentialEnvelopeTests.cpp
Tests/FilterCoefficientsTests.cpp
```

### Modifiés
```
Source/UI/SoundTab.h           — rewrite complet
Source/Engine/Envelope.cpp     — decay + release exponentiels
Source/Engine/Envelope.h       — setParams pré-calcule les coeffs
Source/Engine/Filter.cpp       — utilise FilterCoefficients.h
Source/PluginEditor.cpp        — scaffold BP, appelle soundTab.layoutFor(bp)
Source/PluginEditor.h          — include Breakpoint.h
CMakeLists.txt                 — ajoute les 2 nouveaux Tests
```

### Left untouched
- `Source/UI/KnobWithLabel.h` — encore utilisé par ModTab/FXTab/ArpTab
- `Source/UI/LookAndFeel.h` colour aliases — encore utilisés par ModTab/FXTab/ArpTab/BottomBar/SynthSection
- `Source/UI/BottomBar.h`, `Source/UI/ModTab.h`, `Source/UI/FXTab.h`, `Source/UI/ArpTab.h`, `Source/UI/SynthSection.h` — inchangés

---

## 4. Tests

| Test | Type | Vérifie |
|---|---|---|
| `ExponentialEnvelopeTests` | headless engine | Decay converge vers sustain selon `exp(-t/τ)` avec la bonne τ ; release décline selon `exp(-t/τ_r)` ; noteOn durant release retrigger smooth (pas de snap à 0) |
| `FilterCoefficientsTests` | headless engine | Coefficients RBJ identiques à ceux précédemment calculés inline dans Filter.cpp (regression test sur le refactor) |
| `UIConstructionTests` (étendu) | headless UI | Chaque nouveau component se construit et bind à APVTS sans crash |

Pas de test visuel des courbes (EnvelopeEditor, FilterResponseView, OscilloscopeView) — validation manuelle en DAW.

---

## 5. Risques & mitigations

| Risque | Mitigation |
|---|---|
| Envelope exponential change les presets recréés durant Phase 2 sonnent différemment après réouverture (τ-vs-linear-ramp difference) | Les presets ont été supprimés ; commit envelope en tout premier dans la branche Phase 2, avant rewrite UI, pour que les presets créés durant les tests UI soient déjà au nouveau comportement |
| FilterResponseView re-calcule les coeffs à chaque `repaint()` | Throttle à 30 Hz via Timer interne ; les coeffs biquad sont ~15 flops, négligeable |
| EnvelopeEditor drag d'un coin a 2 axes (ex. decay corner écrit decay + sustain) : risque de créer 2 undo entries | Utiliser `beginChangeGesture`/`endChangeGesture` autour du drag complet, pas à chaque pixel |
| `layoutFor(BP)` ajouté à SoundTab mais pas aux autres tabs : `PluginEditor::resized()` appelle des méthodes différentes selon le tab | Acceptable : une ligne spéciale dans PluginEditor pour SoundTab. Quand les autres tabs migrent (Phase 3/4), ils gagnent leur propre `layoutFor(BP)` et on refactorise en base class commune à ce moment-là |
| 7 composants en un seul phase = gros PR | Commits granulaires (1 commit par component, style Phase 1 Tasks 4-9), merge en fin |

---

## 6. Questions résolues (référence au roadmap)

| Question roadmap | Résolution Phase 2 |
|---|---|
| OscilloscopeView data source : per-voice audio tap vs mixed block ? | **Ni l'un ni l'autre** : rendu synthétique depuis le param (option C, ajoutée) |
| EnvelopeEditor interaction : ADSR only vs s-curve ? | **ADSR only**, mais avec courbes exponentielles analog-style matchant l'engine |
| Scope : UI + engine features en un phase ? | **UI only** + envelope refinement isolé. Phase 2b prendra les engine features (phase/PWM/drive/keytrack/osc2 unison/LP12/Notch) |
| Legacy KnobWithLabel deletion | **Reporté** à quand ModTab/FXTab/ArpTab migrent |
| Legacy colour aliases | **Reportés** pour la même raison |
| FilterTypeSelect pills | **3 pills (LP/HP/BP)** en Phase 2. LP12/Notch arrivent Phase 2b avec leur DSP |

---

## 7. Scope exclu explicitement

Éléments présents dans le mockup mais **pas dans Phase 2** :
- Knobs `osc1.phase`, `osc1.pwm`, `osc2.phase`, `osc2.pwm` → Phase 2b
- Knobs `filt.drive`, `filt.keytrack` → Phase 2b
- Bloc unison osc2 (`osc2.uni.{voices,detune,spread}`) → Phase 2b
- Filtres LP12 et Notch → Phase 2b
- Animations micro-interactions (tick-flash au knob round-values, reset-flash, ring modulation indicator) → Phase 5
- Responsive layouts réels pour Compact et Wide → Phase 5 (le scaffold BP est posé, les trois branches sont équivalentes pour l'instant)
- MIDI learn, accessibility, keyboard nudging → Phase 5
