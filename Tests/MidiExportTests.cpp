#include "engine/MidiExport.h"
#include "engine/RollLibrary.h"

namespace rk
{

class MidiExportTests : public juce::UnitTest
{
public:
    MidiExportTests() : juce::UnitTest ("MIDI export", "RollsKilla") {}

    void runTest() override
    {
        RollLibrary lib;
        const auto& preset = lib.getPreset (lib.search ("Bando Evangelium").front());

        beginTest ("exported file reloads with tempo, length and notes");
        {
            MidiExportOptions o;
            o.bpm = 143.0;
            o.rootNote = 60;

            auto pattern = preset.pattern;
            pattern.notes[3].muted = true;

            juce::TemporaryFile tmp (".mid");
            expect (writeMidiFile (pattern, o, tmp.getFile()));

            juce::FileInputStream in (tmp.getFile());
            juce::MidiFile reloaded;
            expect (reloaded.readFrom (in));
            expectEquals (reloaded.getTimeFormat(), (short) kTicksPerBeat);
            expectEquals (reloaded.getNumTracks(), 2);

            juce::MidiMessageSequence tempos;
            reloaded.findAllTempoEvents (tempos);
            expectEquals (tempos.getNumEvents(), 1);
            expectWithinAbsoluteError (60.0 / tempos.getEventPointer (0)->message.getTempoSecondsPerQuarterNote(), 143.0, 0.01);

            const auto* hats = reloaded.getTrack (1);
            int ons = 0, offs = 0;
            double lastOn = 0.0;
            for (const auto* e : *hats)
            {
                if (e->message.isNoteOn())
                {
                    ++ons;
                    lastOn = e->message.getTimeStamp();
                    expect (e->message.getNoteNumber() >= 60);
                }
                offs += e->message.isNoteOff() ? 1 : 0;
            }
            expectEquals (ons, (int) pattern.notes.size() - 1, "muted notes are not exported");
            expectEquals (offs, ons);
            expectEquals (reloaded.getLastTimestamp(), pattern.lengthBeats() * kTicksPerBeat, "length = bars");
            expectWithinAbsoluteError (lastOn, pattern.notes.back().beat * kTicksPerBeat, 1.0);

            // first note is at beat 0 with the preset velocity
            for (const auto* e : *hats)
                if (e->message.isNoteOn())
                {
                    expectEquals ((int) e->message.getVelocity(), pattern.notes[0].vel);
                    expectEquals (e->message.getTimeStamp(), 0.0);
                    break;
                }
        }

        beginTest ("drag and drop file");
        {
            const auto f = createDragAndDropMidiFile (preset.pattern, {}, "Rolls Killa - Test/Drag");
            expect (f.existsAsFile());
            expect (f.getFileName().endsWith (".mid"));
            f.deleteFile();
        }
    }
};

static MidiExportTests midiExportTests;

} // namespace rk
