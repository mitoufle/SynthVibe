#include <juce_core/juce_core.h>
#include "Engine/ArpEngine.h"
#include <vector>
#include <algorithm>

struct ArpEngineTests : public juce::UnitTest
{
    ArpEngineTests() : juce::UnitTest("ArpEngine", "AISynth") {}

    void runTest() override
    {
        beginTest("held notes are re-emitted as noteOns when arp is disabled");

        ArpEngine arp;
        arp.prepare();

        // Enable arp
        ArpEngine::Params on;
        on.enabled    = true;
        on.mode       = ArpEngine::Mode::Up;
        on.rateIndex  = 2;   // 1/16 note — slow enough that step won't fire in 32 samples
        on.octaveRange = 1;
        arp.setParams(on);

        // User presses note 60 while arp is on.
        // process() consumes the noteOn into heldNotes — SynthEngine sees only arp events.
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);
        arp.process(midi, 32, 120.0, 44100.0);

        // Disable the arp while note 60 is still physically held.
        ArpEngine::Params off = on;
        off.enabled = false;
        arp.setParams(off);

        // process() should re-emit noteOn(60) so SynthEngine can resume the held note.
        juce::MidiBuffer output;
        arp.process(output, 32, 120.0, 44100.0);

        bool foundNoteOn60 = false;
        for (auto meta : output)
            if (meta.getMessage().isNoteOn() &&
                meta.getMessage().getNoteNumber() == 60)
                foundNoteOn60 = true;

        expect(foundNoteOn60,
               "noteOn(60) must appear in output after arp disable so SynthEngine plays the held note");

        // ----------------------------------------------------------------
        // Test 2: held notes must NOT be re-emitted on a second process() call
        // ----------------------------------------------------------------
        beginTest("held notes are not re-emitted on second process() call after arp disable");

        juce::MidiBuffer output2;
        arp.process(output2, 32, 120.0, 44100.0);

        bool secondNoteOn = false;
        for (auto meta : output2)
            if (meta.getMessage().isNoteOn() && meta.getMessage().getNoteNumber() == 60)
                secondNoteOn = true;

        expect(!secondNoteOn,
               "noteOn(60) must not appear in second process() call after arp disable");

        // ----------------------------------------------------------------
        // Test 3: multi-note + velocity preservation
        // ----------------------------------------------------------------
        beginTest("all held notes with original velocities are re-emitted when arp is disabled");

        ArpEngine arp2;
        arp2.prepare();

        ArpEngine::Params on2;
        on2.enabled    = true;
        on2.mode       = ArpEngine::Mode::Up;
        on2.rateIndex  = 2;
        on2.octaveRange = 1;
        arp2.setParams(on2);

        juce::MidiBuffer midi2;
        midi2.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);
        midi2.addEvent(juce::MidiMessage::noteOn(1, 64, 0.5f), 1);
        arp2.process(midi2, 32, 120.0, 44100.0);

        ArpEngine::Params off2 = on2;
        off2.enabled = false;
        arp2.setParams(off2);

        juce::MidiBuffer out2;
        arp2.process(out2, 32, 120.0, 44100.0);

        bool found60 = false, found64 = false;
        float vel60 = 0.f, vel64 = 0.f;
        for (auto meta : out2)
        {
            auto msg = meta.getMessage();
            if (msg.isNoteOn())
            {
                if (msg.getNoteNumber() == 60) { found60 = true; vel60 = msg.getFloatVelocity(); }
                if (msg.getNoteNumber() == 64) { found64 = true; vel64 = msg.getFloatVelocity(); }
            }
        }

        expect(found60 && found64,
               "both held notes (60 and 64) must be re-emitted");
        expectWithinAbsoluteError(vel60, 0.8f, 0.01f,
               "note 60 velocity must be preserved");
        expectWithinAbsoluteError(vel64, 0.5f, 0.01f,
               "note 64 velocity must be preserved");

        // ----------------------------------------------------------------
        // Test 4: dnup pattern
        // ----------------------------------------------------------------
        beginTest("dnup pattern with [60, 64, 67] yields 67 -> 64 -> 60 -> 64 -> 67");
        {
            ArpEngine arp;
            arp.prepare();
            ArpEngine::Params p;
            p.enabled     = true;
            p.mode        = ArpEngine::Mode::Dnup;
            p.rateIndex   = 2;     // index 2 = 1/16 in new 5-entry table
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
                   "dnup expected 67->64->60->64->67, got "
                   + juce::String(emittedNotes.size()) + " notes");
        }

        // ----------------------------------------------------------------
        // Test 5: asplayed pattern
        // ----------------------------------------------------------------
        beginTest("asplayed pattern preserves insertion order [64, 60, 67]");
        {
            ArpEngine arp;
            arp.prepare();
            ArpEngine::Params p;
            p.enabled     = true;
            p.mode        = ArpEngine::Mode::AsPlayed;
            p.rateIndex   = 2;     // index 2 = 1/16 in new 5-entry table
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
                   "asplayed expected 64->60->67->64, got "
                   + juce::String(emittedNotes.size()) + " notes");
        }

        // ----------------------------------------------------------------
        // Test 6: chord pattern
        // ----------------------------------------------------------------
        beginTest("chord pattern emits 3 simultaneous noteOns per step");
        {
            ArpEngine arp;
            arp.prepare();
            ArpEngine::Params p;
            p.enabled     = true;
            p.mode        = ArpEngine::Mode::Chord;
            p.rateIndex   = 2;     // index 2 = 1/16 in new 5-entry table
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

        // ----------------------------------------------------------------
        // Test 7: chord pattern with octaveRange=2 emits both octaves simultaneously
        // ----------------------------------------------------------------
        beginTest("chord pattern with octaveRange=2 emits both octaves simultaneously");
        {
            ArpEngine arp;
            arp.prepare();
            ArpEngine::Params p;
            p.enabled     = true;
            p.mode        = ArpEngine::Mode::Chord;
            p.rateIndex   = 2;     // index 2 = 1/16 in new 5-entry table
            p.octaveRange = 2;
            arp.setParams(p);
            arp.noteOn(60, 1.0f);
            arp.noteOn(64, 1.0f);
            arp.noteOn(67, 1.0f);

            const double sr  = 48000.0;
            const double bpm = 120.0;
            const int stepLen = (int) ((60.0 / bpm) * 0.25 * sr);

            juce::MidiBuffer buf;
            arp.process(buf, stepLen, bpm, sr);

            std::vector<int> notesAtStep0;
            for (auto m : buf)
                if (m.getMessage().isNoteOn() && m.samplePosition == 0)
                    notesAtStep0.push_back(m.getMessage().getNoteNumber());
            std::sort(notesAtStep0.begin(), notesAtStep0.end());

            const std::vector<int> expected { 60, 64, 67, 72, 76, 79 };
            expect(notesAtStep0 == expected,
                   "chord+oct2 expected {60,64,67,72,76,79} at sample 0, got "
                   + juce::String(notesAtStep0.size()) + " notes");
        }

        // ----------------------------------------------------------------
        // Test 8: gate=0.5 fires noteOff at half-step
        // ----------------------------------------------------------------
        beginTest("gate=0.5 fires noteOff at half-step");
        {
            ArpEngine arp;
            arp.prepare();
            ArpEngine::Params p;
            p.enabled     = true;
            p.mode        = ArpEngine::Mode::Up;
            p.rateIndex   = 2;     // 1/16 (new table)
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
                   "noteOff at ~stepLen/2, got " + juce::String(noteOffPos)
                   + " expected " + juce::String(expectedOff));
        }

        // ----------------------------------------------------------------
        // Test 9: gate=1.0 fires noteOff at next step boundary (regression)
        // ----------------------------------------------------------------
        beginTest("gate=1.0 fires noteOff at next step boundary (regression)");
        {
            ArpEngine arp;
            arp.prepare();
            ArpEngine::Params p;
            p.enabled     = true;
            p.mode        = ArpEngine::Mode::Up;
            p.rateIndex   = 2;     // 1/16 (new table)
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
    }
};

static ArpEngineTests arpEngineTests;
