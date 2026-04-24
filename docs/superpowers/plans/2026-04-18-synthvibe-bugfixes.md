# SynthVibe — Bugfixes & Optimisations (Phase 1 Post-Review) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Corriger les 6 bugs critiques et 4 problèmes importants identifiés lors de la code review de Phase 1 (stuck notes, allocations audio thread, interpolation chorus, optimisations per-sample).

**Architecture:** Corrections ciblées fichier par fichier — aucune restructuration. ArpEngine reçoit un flag `pendingNoteOff` pour émettre les noteOff manquants sans changer l'interface publique. Voice.cpp reçoit des guards conditionnels. PluginProcessor.cpp snapshote les params une seule fois par bloc.

**Tech Stack:** C++17, JUCE 7.0.9, Visual Studio 18 2026. Build: cmake via `CMAKE="C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"` depuis le worktree `C:/Users/mitoufle/.config/superpowers/worktrees/AISynth/phase1-rich-synth/`.

---

## File Map

| Fichier | Action | Ce qui change |
|---|---|---|
| `Source/Engine/ArpEngine.h` | Modify | Ajouter `pendingNoteOff`, `sorted` membre, supprimer `static rng` |
| `Source/Engine/ArpEngine.cpp` | Modify | 6 bugs critiques/mineurs (stuck notes, BPM div/0, MIDI passthrough, allocations, UpDown, rng) |
| `Source/FX/Chorus.cpp` | Modify | Fix interpolation + stéréo width |
| `Source/PluginProcessor.cpp` | Modify | Snapshot params 1×/bloc, master vol après FX |
| `Source/Engine/Voice.cpp` | Modify | Guards conditionnels pitch/filter per-sample |

---

## Task 1 : ArpEngine — stuck notes + BPM safety + MIDI passthrough

**Bugs corrigés :** #1, #2, #4, #6

**Files:**
- Modify: `Source/Engine/ArpEngine.h`
- Modify: `Source/Engine/ArpEngine.cpp`

- [ ] **Step 1 : Ajouter `pendingNoteOff` dans ArpEngine.h**

Remplacer la section private de `ArpEngine.h` :

```cpp
private:
    Params params;

    struct HeldNote { int note; float velocity; };
    std::vector<HeldNote> heldNotes;
    std::vector<HeldNote> sequence;

    int   stepIndex     = 0;
    int   sampleCounter = 0;
    int   pingDir       = 1;
    int   lastNote      = -1;
    bool  noteIsOn      = false;
    bool  pendingNoteOff = false;   // set when arp disabled mid-note

    void buildSequence();
    int  samplesPerStep(double bpm, double sr) const noexcept;
};
```

- [ ] **Step 2 : Corriger `setParams()` — ne pas effacer lastNote/noteIsOn immédiatement**

Remplacer la fonction `setParams` dans `ArpEngine.cpp` :

```cpp
void ArpEngine::setParams(const Params& p)
{
    if (!p.enabled && params.enabled && noteIsOn && lastNote >= 0)
        pendingNoteOff = true;   // process() émettra le noteOff au prochain bloc

    params = p;

    if (!p.enabled)
    {
        heldNotes.clear();
        sequence.clear();
        stepIndex     = 0;
        sampleCounter = 0;
        pingDir       = 1;
        // Ne pas toucher noteIsOn/lastNote — process() les nettoie après le noteOff
    }
}
```

- [ ] **Step 3 : Corriger `reset()` — réinitialisation complète (appelé manuellement si besoin)**

```cpp
void ArpEngine::reset()
{
    heldNotes.clear();
    sequence.clear();
    stepIndex      = 0;
    sampleCounter  = 0;
    pingDir        = 1;
    lastNote       = -1;
    noteIsOn       = false;
    pendingNoteOff = false;
}
```

- [ ] **Step 4 : Corriger `samplesPerStep()` — protéger contre BPM=0 et stepLen=0**

```cpp
int ArpEngine::samplesPerStep(double bpm, double sr) const noexcept
{
    static constexpr double rateFractions[] = { 0.25, 0.5, 1.0, 2.0 };
    const double safeBpm = std::max(bpm, 20.0);
    const double beats   = rateFractions[juce::jlimit(0, 3, params.rateIndex)];
    return std::max(static_cast<int>((60.0 / safeBpm) * beats * sr), 1);
}
```

- [ ] **Step 5 : Corriger `process()` — pendingNoteOff + stuck note sur séquence vide + MIDI passthrough**

Remplacer la fonction `process` entière :

```cpp
void ArpEngine::process(juce::MidiBuffer& midi, int numSamples, double bpm, double sr)
{
    // Émettre le noteOff en attente (arp désactivé pendant qu'une note sonnait)
    if (pendingNoteOff && lastNote >= 0)
    {
        midi.addEvent(juce::MidiMessage::noteOff(1, lastNote), 0);
        pendingNoteOff = false;
        noteIsOn       = false;
        lastNote       = -1;
        return;
    }

    if (!params.enabled)
        return;

    juce::MidiBuffer output;
    const int stepLen = samplesPerStep(bpm, sr);

    // Lire les events entrants : noteOn/noteOff mis à jour dans l'état interne,
    // tout le reste (CC, pitchbend, aftertouch…) passe directement dans output.
    for (auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOff())
            noteOff(msg.getNoteNumber());
        else if (msg.isNoteOn())
            noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
        else
            output.addEvent(msg, meta.samplePosition);
    }

    // Si la séquence est vide (toutes touches relâchées), émettre le noteOff en attente
    if (sequence.empty())
    {
        if (noteIsOn && lastNote >= 0)
        {
            output.addEvent(juce::MidiMessage::noteOff(1, lastNote), 0);
            noteIsOn = false;
            lastNote = -1;
        }
        midi.swapWith(output);
        return;
    }

    for (int i = 0; i < numSamples; ++i)
    {
        if (sampleCounter == 0)
        {
            if (noteIsOn && lastNote >= 0)
            {
                output.addEvent(juce::MidiMessage::noteOff(1, lastNote), i);
                noteIsOn = false;
            }

            const auto& step = sequence[stepIndex];
            output.addEvent(juce::MidiMessage::noteOn(1, step.note, step.velocity), i);
            lastNote = step.note;
            noteIsOn = true;

            if (params.mode == Mode::UpDown)
            {
                stepIndex += pingDir;
                if (stepIndex >= static_cast<int>(sequence.size()) - 1) pingDir = -1;
                if (stepIndex <= 0) pingDir = 1;
                stepIndex = juce::jlimit(0, static_cast<int>(sequence.size()) - 1, stepIndex);
            }
            else
            {
                stepIndex = (stepIndex + 1) % static_cast<int>(sequence.size());
            }
        }
        ++sampleCounter;
        if (sampleCounter >= stepLen) sampleCounter = 0;
    }

    midi.swapWith(output);
}
```

- [ ] **Step 6 : Build**

```bat
CMAKE="C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
cd "C:/Users/mitoufle/.config/superpowers/worktrees/AISynth/phase1-rich-synth"
"$CMAKE" --build build --config Release 2>&1 | grep -E "(error C|\.vst3|\.exe)"
```
Attendu : `AISynth_Standalone.vcxproj -> ... AI Synth.exe` sans erreur.

- [ ] **Step 7 : Commit**

```bash
cd "C:/Users/mitoufle/.config/superpowers/worktrees/AISynth/phase1-rich-synth"
git add Source/Engine/ArpEngine.h Source/Engine/ArpEngine.cpp
git commit -m "fix: ArpEngine stuck notes, BPM div-by-zero, MIDI passthrough"
```

---

## Task 2 : ArpEngine — éliminer les allocations sur l'audio thread

**Bug corrigé :** #3

**Files:**
- Modify: `Source/Engine/ArpEngine.h`
- Modify: `Source/Engine/ArpEngine.cpp`

- [ ] **Step 1 : Ajouter le membre `sorted` pré-alloué dans ArpEngine.h**

Dans la section `private`, ajouter après `sequence` :

```cpp
    std::vector<HeldNote> heldNotes;
    std::vector<HeldNote> sequence;
    std::vector<HeldNote> sortedBuf;  // buffer de travail pour buildSequence(), évite l'alloc per-call
```

- [ ] **Step 2 : Pré-allouer les vecteurs dans `reset()` (appelé à l'init)**

Ajouter en bas de `reset()` :

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
}
```

Et ajouter une méthode `prepare()` appelée depuis `PluginProcessor::prepareToPlay` :

Dans `ArpEngine.h`, ajouter dans la section public :
```cpp
    void prepare();
```

Dans `ArpEngine.cpp` :
```cpp
void ArpEngine::prepare()
{
    heldNotes.reserve(32);
    sequence.reserve(128);   // 4 octaves × 32 touches max
    sortedBuf.reserve(32);
}
```

- [ ] **Step 3 : Appeler `arp.prepare()` dans PluginProcessor.cpp**

Dans `Source/PluginProcessor.cpp`, dans `prepareToPlay()` :

```cpp
void AISynthProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec { sampleRate,
                                  static_cast<juce::uint32>(samplesPerBlock),
                                  2 };
    synth.prepare(spec);
    fxChain.prepare(sampleRate, samplesPerBlock);
    arp.prepare();
}
```

- [ ] **Step 4 : Utiliser `sortedBuf` dans `buildSequence()` — supprimer la copie locale**

Remplacer `buildSequence()` entier :

```cpp
void ArpEngine::buildSequence()
{
    sequence.clear();
    if (heldNotes.empty()) return;

    // Réutiliser le buffer pré-alloué au lieu de créer un vecteur local
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

- [ ] **Step 5 : Déplacer `rng` en membre privé (fix #12 au passage)**

Dans `ArpEngine.h`, section private, ajouter :
```cpp
    std::mt19937 rng { std::random_device{}() };
```

Dans `ArpEngine.cpp`, supprimer la ligne `static std::mt19937 rng { std::random_device{}() };` dans `buildSequence()` (elle est désormais dans le header).

- [ ] **Step 6 : Build**

```bat
CMAKE="C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
cd "C:/Users/mitoufle/.config/superpowers/worktrees/AISynth/phase1-rich-synth"
"$CMAKE" --build build --config Release 2>&1 | grep -E "(error C|\.vst3|\.exe)"
```
Attendu : build propre sans erreur.

- [ ] **Step 7 : Commit**

```bash
cd "C:/Users/mitoufle/.config/superpowers/worktrees/AISynth/phase1-rich-synth"
git add Source/Engine/ArpEngine.h Source/Engine/ArpEngine.cpp Source/PluginProcessor.cpp
git commit -m "fix: ArpEngine pre-allocate vectors, move rng to member, call prepare()"
```

---

## Task 3 : Chorus — interpolation correcte + largeur stéréo

**Bugs corrigés :** #5, #14

**Files:**
- Modify: `Source/FX/Chorus.cpp`

- [ ] **Step 1 : Corriger `readInterpolated()` — éliminer la fraction négative**

Remplacer la méthode `readInterpolated` entière dans `Chorus.cpp` :

```cpp
float Chorus::readInterpolated(const std::vector<float>& buf, float delSamples) const noexcept
{
    float readPosF = static_cast<float>(writePos) - delSamples;
    // S'assurer que readPosF est positif avant le cast (même pattern que Delay.cpp)
    while (readPosF < 0.f) readPosF += static_cast<float>(bufSize);
    const int   i0   = static_cast<int>(readPosF) % bufSize;
    const int   i1   = (i0 + 1) % bufSize;
    const float frac = readPosF - std::floor(readPosF);
    return buf[i0] * (1.f - frac) + buf[i1] * frac;
}
```

- [ ] **Step 2 : Ajouter la largeur stéréo — offset de phase 90° sur le canal droit**

Dans `Chorus.h`, section private, ajouter un second compteur de phase LFO :
```cpp
    double lfoPhase  = 0.0;
    double lfoPhaseR = 0.25;  // 90° de décalage pour la largeur stéréo
```

Dans `Chorus.cpp`, méthode `reset()`, ajouter :
```cpp
void Chorus::reset()
{
    std::fill(bufL.begin(), bufL.end(), 0.f);
    std::fill(bufR.begin(), bufR.end(), 0.f);
    writePos  = 0;
    lfoPhase  = 0.0;
    lfoPhaseR = 0.25;
}
```

Dans `Chorus.cpp`, méthode `process()`, remplacer le bloc LFO et le traitement R :

```cpp
void Chorus::process(juce::AudioBuffer<float>& buffer)
{
    if (params.mix < 0.001f)
        return;

    const int   numSamples   = buffer.getNumSamples();
    float*      chL          = buffer.getWritePointer(0);
    float*      chR          = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : chL;
    const float lfoIncrement = static_cast<float>(params.rate / sampleRate);

    for (int i = 0; i < numSamples; ++i)
    {
        // LFO canal gauche
        const float lfoL = 0.5f * (1.f + std::sin(juce::MathConstants<float>::twoPi * static_cast<float>(lfoPhase)));
        lfoPhase += lfoIncrement;
        if (lfoPhase >= 1.0) lfoPhase -= 1.0;

        // LFO canal droit : 90° de décalage → largeur stéréo
        const float lfoR = 0.5f * (1.f + std::sin(juce::MathConstants<float>::twoPi * static_cast<float>(lfoPhaseR)));
        lfoPhaseR += lfoIncrement;
        if (lfoPhaseR >= 1.0) lfoPhaseR -= 1.0;

        const float delaySamplesL = static_cast<float>((0.007 + params.depth * lfoL) * sampleRate);
        const float delaySamplesR = static_cast<float>((0.007 + params.depth * lfoR) * sampleRate);

        bufL[writePos] = chL[i];
        bufR[writePos] = chR[i];

        const float wetL = readInterpolated(bufL, delaySamplesL);
        const float wetR = readInterpolated(bufR, delaySamplesR);

        chL[i] = chL[i] * (1.f - params.mix) + wetL * params.mix;
        chR[i] = chR[i] * (1.f - params.mix) + wetR * params.mix;

        writePos = (writePos + 1) % bufSize;
    }
}
```

- [ ] **Step 3 : Build**

```bat
CMAKE="C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
cd "C:/Users/mitoufle/.config/superpowers/worktrees/AISynth/phase1-rich-synth"
"$CMAKE" --build build --config Release 2>&1 | grep -E "(error C|\.vst3|\.exe)"
```
Attendu : build propre.

- [ ] **Step 4 : Commit**

```bash
cd "C:/Users/mitoufle/.config/superpowers/worktrees/AISynth/phase1-rich-synth"
git add Source/FX/Chorus.h Source/FX/Chorus.cpp
git commit -m "fix: Chorus interpolation (negative frac), add stereo width (90deg LFO offset)"
```

---

## Task 4 : PluginProcessor — snapshot params 1×/bloc + master vol après FX

**Bugs corrigés :** #9, #10

**Files:**
- Modify: `Source/PluginProcessor.cpp`

- [ ] **Step 1 : Snapshotter VoiceParams une seule fois avant la boucle MIDI**

Dans `processBlock`, remplacer les deux appels `buildVoiceParams()` dans la boucle par un snapshot unique :

```cpp
void AISynthProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Snapshot APVTS une seule fois par bloc (évite 26 getRawParameterValue × N events)
    const VoiceParams vp = buildVoiceParams();

    // Arpeggiator
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) bpm = *b;
    arp.setParams(buildArpParams());
    arp.process(midiMessages, buffer.getNumSamples(), bpm, getSampleRate());

    // Dispatch MIDI events à positions sample-accurate
    int currentSample = 0;
    for (const auto meta : midiMessages)
    {
        const int eventPos = meta.samplePosition;
        if (eventPos > currentSample)
        {
            synth.setParams(vp);
            synth.processBlock(buffer, currentSample, eventPos - currentSample);
            currentSample = eventPos;
        }
        synth.handleMidiMessage(meta.getMessage());
    }

    const int remaining = buffer.getNumSamples() - currentSample;
    if (remaining > 0) {
        synth.setParams(vp);
        synth.processBlock(buffer, currentSample, remaining);
    }

    // FX chain en premier, PUIS master volume (ordre correct : post-fader)
    fxChain.setParams(buildDelayParams(), buildChorusParams());
    fxChain.process(buffer);

    const float masterVol = *apvts.getRawParameterValue(ParamIDs::masterVolume);
    buffer.applyGain(masterVol);
}
```

- [ ] **Step 2 : Build**

```bat
CMAKE="C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
cd "C:/Users/mitoufle/.config/superpowers/worktrees/AISynth/phase1-rich-synth"
"$CMAKE" --build build --config Release 2>&1 | grep -E "(error C|\.vst3|\.exe)"
```
Attendu : build propre.

- [ ] **Step 3 : Commit**

```bash
cd "C:/Users/mitoufle/.config/superpowers/worktrees/AISynth/phase1-rich-synth"
git add Source/PluginProcessor.cpp
git commit -m "fix: snapshot VoiceParams once per block, move master vol after FX chain"
```

---

## Task 5 : Voice — guards conditionnels per-sample (pitch + filter)

**Bugs corrigés :** #7, #8

**Files:**
- Modify: `Source/Engine/Voice.cpp`

- [ ] **Step 1 : Ajouter les guards dans `getNextSample()`**

Remplacer le corps de `getNextSample()` entier dans `Voice.cpp` :

```cpp
float Voice::getNextSample()
{
    if (!ampEnv.isActive())
        return 0.f;

    const float l1 = lfo1Osc.getNextSample() * params.lfo1.depth;
    const float l2 = lfo2Osc.getNextSample() * params.lfo2.depth;

    // Pitch LFO — uniquement si au moins un LFO cible le pitch
    const bool hasPitchLfo = (params.lfo1.dest == LfoDest::Pitch && params.lfo1.depth > 0.f)
                           || (params.lfo2.dest == LfoDest::Pitch && params.lfo2.depth > 0.f);
    if (hasPitchLfo && currentNote >= 0)
    {
        const float pitchCents = (params.lfo1.dest == LfoDest::Pitch ? l1 * 100.f : 0.f)
                               + (params.lfo2.dest == LfoDest::Pitch ? l2 * 100.f : 0.f);
        const double pitchRatio = std::pow(2.0, pitchCents / 1200.0);
        osc1.setFrequency(midiNoteToHz(currentNote, params.osc1.octave, params.osc1.semitone) * pitchRatio);
        osc2.setFrequency(midiNoteToHz(currentNote, params.osc2.octave, params.osc2.semitone) * pitchRatio);
    }

    // Detune LFO
    const bool hasDetuneLfo = (params.lfo1.dest == LfoDest::Detune && params.lfo1.depth > 0.f)
                            || (params.lfo2.dest == LfoDest::Detune && params.lfo2.depth > 0.f);
    if (hasDetuneLfo)
    {
        const float detuneMod = (params.lfo1.dest == LfoDest::Detune ? l1 * 50.f : 0.f)
                              + (params.lfo2.dest == LfoDest::Detune ? l2 * 50.f : 0.f);
        osc1.setDetuneCents(params.osc1.detune + detuneMod);
    }

    float sample = osc1.getNextSample() * params.osc1.level
                 + osc2.getNextSample() * params.osc2.level;

    // Filter — recalcul des coefficients seulement si la cutoff change
    const float envMod = fltEnv.getNextSample();
    const bool hasFilterMod = (params.lfo1.dest == LfoDest::Filter && params.lfo1.depth > 0.f)
                            || (params.lfo2.dest == LfoDest::Filter && params.lfo2.depth > 0.f)
                            || (params.filterEnvAmt != 0.f);
    if (hasFilterMod)
    {
        float cutoff = params.filterCutoff * (1.f + params.filterEnvAmt * envMod);
        cutoff += (params.lfo1.dest == LfoDest::Filter ? l1 * 4000.f : 0.f);
        cutoff += (params.lfo2.dest == LfoDest::Filter ? l2 * 4000.f : 0.f);
        cutoff = juce::jlimit(20.f, 20000.f, cutoff);
        filter.setCutoff(cutoff);
    }

    sample = filter.processSample(sample);

    float ampGain = ampEnv.getNextSample() * velocity;
    const bool hasAmpLfo = (params.lfo1.dest == LfoDest::Amp && params.lfo1.depth > 0.f)
                         || (params.lfo2.dest == LfoDest::Amp && params.lfo2.depth > 0.f);
    if (hasAmpLfo)
    {
        ampGain *= 1.f + (params.lfo1.dest == LfoDest::Amp ? l1 * 0.5f : 0.f);
        ampGain *= 1.f + (params.lfo2.dest == LfoDest::Amp ? l2 * 0.5f : 0.f);
    }
    sample *= ampGain;

    return sample;
}
```

- [ ] **Step 2 : Build**

```bat
CMAKE="C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
cd "C:/Users/mitoufle/.config/superpowers/worktrees/AISynth/phase1-rich-synth"
"$CMAKE" --build build --config Release 2>&1 | grep -E "(error C|\.vst3|\.exe)"
```
Attendu : build propre.

- [ ] **Step 3 : Commit**

```bash
cd "C:/Users/mitoufle/.config/superpowers/worktrees/AISynth/phase1-rich-synth"
git add Source/Engine/Voice.cpp
git commit -m "perf: Voice skip pow/setCutoff per-sample when no LFO modulation active"
```

---

## Task 6 : ArpEngine — UpDown boundary fix

**Bug corrigé :** #11

**Files:**
- Modify: `Source/Engine/ArpEngine.cpp`

- [ ] **Step 1 : Corriger la logique UpDown dans `process()`**

Le comportement attendu pour [A, B, C] : A B C C B A B C C B A…
(les notes aux extremes se répètent, les notes intermédiaires ne se répètent pas)

Dans `process()`, remplacer le bloc UpDown :

```cpp
            if (params.mode == Mode::UpDown)
            {
                // Avancer d'abord, puis détecter le rebond
                stepIndex += pingDir;
                if (stepIndex >= static_cast<int>(sequence.size()))
                {
                    // Dépassé la fin : rebondir et rejouer la dernière note
                    pingDir   = -1;
                    stepIndex = static_cast<int>(sequence.size()) - 1;
                }
                else if (stepIndex < 0)
                {
                    // Dépassé le début : rebondir et rejouer la première note
                    pingDir   = 1;
                    stepIndex = 0;
                }
            }
```

- [ ] **Step 2 : Build**

```bat
CMAKE="C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
cd "C:/Users/mitoufle/.config/superpowers/worktrees/AISynth/phase1-rich-synth"
"$CMAKE" --build build --config Release 2>&1 | grep -E "(error C|\.vst3|\.exe)"
```
Attendu : build propre.

- [ ] **Step 3 : Commit**

```bash
cd "C:/Users/mitoufle/.config/superpowers/worktrees/AISynth/phase1-rich-synth"
git add Source/Engine/ArpEngine.cpp
git commit -m "fix: ArpEngine UpDown repeats boundary notes correctly (A B C C B A)"
```

---

## Self-review checklist

- [x] Bug #1 (stuck note / disable) → Task 1 Step 2 : `pendingNoteOff` flag + `process()` l'émet
- [x] Bug #2 (stuck note / séquence vide) → Task 1 Step 5 : early-return avec noteOff
- [x] Bug #3 (allocations audio thread) → Task 2 : `reserve()`, `sortedBuf` membre, `swapWith`
- [x] Bug #4 (BPM div/0) → Task 1 Step 4 : `safeBpm = max(bpm, 20)`, `max(stepLen, 1)`
- [x] Bug #5 (Chorus interpolation) → Task 3 Step 1 : wrap positif avant cast
- [x] Bug #6 (MIDI passthrough) → Task 1 Step 5 : passthrough dans la boucle de scan
- [x] Bug #7 (setCutoff/tan per-sample) → Task 5 : guard `hasFilterMod`
- [x] Bug #8 (pow per-sample sans pitch LFO) → Task 5 : guard `hasPitchLfo`
- [x] Bug #9 (buildVoiceParams N×/bloc) → Task 4 : snapshot unique `vp`
- [x] Bug #10 (master vol avant FX) → Task 4 : `applyGain` déplacé après `fxChain.process`
- [x] Bug #11 (UpDown boundary) → Task 6
- [x] Bug #12 (static rng) → Task 2 Step 5 : rng membre
- [x] Bug #14 (Chorus mono) → Task 3 Step 2 : `lfoPhaseR = 0.25`
- [ ] Bug #13 (SmoothedValue) — **hors scope** de ce plan, à traiter séparément
- [x] Pas de placeholder ni TBD
- [x] Types cohérents : `HeldNote`, `sortedBuf` (Task 2) utilisé dans `buildSequence()` (même Task 2)
- [x] `prepare()` ajouté dans l'interface publique de `ArpEngine.h` ET appelé dans `PluginProcessor::prepareToPlay()`
- [x] `lfoPhaseR` ajouté dans `Chorus.h` ET initialisé dans `reset()`
