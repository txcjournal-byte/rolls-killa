#include "engine/KitExport.h"
#include "engine/KitSampler.h"

#include <juce_audio_formats/juce_audio_formats.h>

namespace rk
{

class KitTests : public juce::UnitTest
{
public:
    KitTests() : juce::UnitTest ("Drum kit", "RollsKilla") {}

    void runTest() override
    {
        const auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("rk_kit_test_" + juce::String (juce::Random::getSystemRandom().nextInt()));
        tmp.createDirectory();

        beginTest ("kit state round trip");
        {
            DrumKit kit;
            kit.name = "BLOOD KIT";
            kit.slot (DrumType::b808).seed = 999;
            kit.slot (DrumType::b808).shape.tune = 3.0f;
            kit.slot (DrumType::clap).patternOn = true;
            kit.slot (DrumType::clap).patternStyle = 2;
            expect (kit.keep (DrumType::b808, "Grave Tank"));
            expect (! kit.keep (DrumType::b808, "Grave Tank"), "no duplicates");
            DrumKit back;
            back.fromValueTree (kit.toValueTree());
            expectEquals (back.name, juce::String ("BLOOD KIT"));
            expect (back.slot (DrumType::b808).seed == 999 && juce::approximatelyEqual (back.slot (DrumType::b808).shape.tune, 3.0f));
            expect (back.slot (DrumType::clap).patternOn && back.slot (DrumType::clap).patternStyle == 2);
            expectEquals ((int) back.kept.size(), 1);
        }

        beginTest ("export kit: folders, 24-bit WAVs, MIDI, never overwrites");
        {
            DrumKit kit;
            kit.name = "TEST KIT";
            kit.keep (DrumType::b808, "One");
            kit.slot (DrumType::b808).seed = 5;
            kit.keep (DrumType::b808, "Two");
            std::vector<KitMidi> midis { { "808 Pattern", drumHitsToMidi (kit.pattern (DrumType::b808), 30, 140.0, "808") } };
            const auto r = exportKit (kit, tmp, midis);
            expect (r.ok, r.error);
            expectEquals (r.wavs, 9);   // 2 kept 808s + the current sound of the 7 other drawers
            expectEquals (r.midis, 1);
            for (int t = 0; t < kNumDrumTypes; ++t)
                expect (r.folder.getChildFile (drumTypeFolder (drumTypeFromIndex (t))).isDirectory());
            const auto wavs = r.folder.getChildFile ("808s").findChildFiles (juce::File::findFiles, false, "*.wav");
            expectEquals (wavs.size(), 2);

            juce::AudioFormatManager fm;
            fm.registerBasicFormats();
            std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (wavs[0]));
            expect (reader != nullptr && reader->bitsPerSample == 24 && juce::approximatelyEqual (reader->sampleRate, 44100.0) && reader->lengthInSamples > 1000);

            const auto again = exportKit (kit, tmp, {});
            expect (again.folder != r.folder && again.folder.getFileName() == "TEST KIT 2", again.folder.getFileName());
        }

        beginTest ("one-shot kit: N different sounds with unique names");
        {
            const auto r = exportOneShotKit (DrumType::clap, 25, 1, {}, 100, "CLAP PACK", tmp);
            expect (r.ok, r.error);
            const auto files = r.folder.getChildFile ("Claps").findChildFiles (juce::File::findFiles, false, "*.wav");
            expectEquals (files.size(), 25);
            juce::StringArray names;
            for (const auto& f : files)
                names.addIfNotAlreadyThere (f.getFileName());
            expectEquals (names.size(), 25);
        }

        beginTest ("kit sampler: plays, pitches, chokes, fades out");
        {
            KitSampler ks;
            ks.prepare (44100.0);
            ks.setSound (DrumType::b808, synthesizeDrum (DrumType::b808, 1, 1, {}, 44100.0));
            ks.setSound (DrumType::clap, synthesizeDrum (DrumType::clap, 1, 1, {}, 44100.0));
            juce::AudioBuffer<float> buf (2, 512);
            buf.clear();
            ks.beginBlock();
            ks.noteOn ((int) DrumType::b808, 30, 127);
            ks.noteOn ((int) DrumType::clap, 60, 100);
            ks.render (buf, 0, 512);
            ks.endBlock();
            expect (buf.getMagnitude (0, 512) > 0.1f);
            expectEquals (ks.getNumActiveVoices(), 2);
            ks.beginBlock();
            ks.noteOn ((int) DrumType::b808, 32, 127);   // 808 is monophonic: the old one fades
            buf.clear();
            ks.render (buf, 0, 512);
            ks.endBlock();
            expectEquals (ks.getNumActiveVoices(), 2);
            ks.beginBlock();
            ks.allNotesOff();
            buf.clear();
            ks.render (buf, 0, 512);
            ks.endBlock();
            expectEquals (ks.getNumActiveVoices(), 0);
        }

        tmp.deleteRecursively();
    }
};

static KitTests kitTests;

} // namespace rk
