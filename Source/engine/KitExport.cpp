#include "KitExport.h"

#include "Pattern.h"

#include <juce_audio_formats/juce_audio_formats.h>

namespace rk
{

namespace
{
    const char* shortName (DrumType t)
    {
        static const char* n[] { "808", "Kick", "Snare", "Clap", "Hat", "Open Hat", "Perc", "FX" };
        return n[juce::jlimit (0, kNumDrumTypes - 1, (int) t)];
    }

    juce::File freshFolder (const juce::File& parent, const juce::String& name)
    {
        auto base = safeFileName (name).trim();
        if (base.isEmpty())
            base = "ROLLS KILLA KIT";
        auto dir = parent.getChildFile (base);
        for (int i = 2; dir.exists(); ++i)
            dir = parent.getChildFile (base + " " + juce::String (i));
        return dir;
    }

    juce::String soundFileName (const juce::String& kitName, DrumType t, int number, const DrumSound& s)
    {
        auto n = safeFileName (kitName) + " - " + shortName (t) + " " + juce::String (number).paddedLeft ('0', 2) + " - " + safeFileName (s.name);
        if ((t == DrumType::b808 || t == DrumType::kick) && s.rootNote != 60)
            n << " (" << noteNameOf (s.rootNote) << ")";
        return n + ".wav";
    }

    void writeReadme (const juce::File& dir, const juce::String& kitName, int wavs)
    {
        juce::String txt;
        txt << kitName << "\r\n"
            << "Made with ROLLS KILLA by TrapVST - " << wavs << " one-shots, 24-bit / 44.1 kHz.\r\n"
            << "Every sound was synthesized by the plugin (or is your own sample).\r\n";
        dir.getChildFile ("README.txt").replaceWithText (txt);
    }
}

juce::String safeFileName (const juce::String& s)
{
    return s.replaceCharacters ("\\/:*?\"<>|", "         ").trim();
}

bool writeWav24 (const juce::AudioBuffer<float>& audio, double sampleRate, const juce::File& file)
{
    file.deleteFile();
    std::unique_ptr<juce::OutputStream> stream (file.createOutputStream());
    if (stream == nullptr)
        return false;
    juce::WavAudioFormat wav;
    auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions {}.withSampleRate (sampleRate)
                                                                              .withNumChannels (audio.getNumChannels())
                                                                              .withBitsPerSample (24));
    return writer != nullptr && writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());
}

juce::MidiFile drumHitsToMidi (const std::vector<DrumHit>& hits, int rootNote, double bpm, const juce::String& trackName, int channel)
{
    juce::MidiMessageSequence seq;
    auto name = juce::MidiMessage::textMetaEvent (3, trackName);
    name.setTimeStamp (0);
    seq.addEvent (name);
    auto tempo = juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / juce::jmax (20.0, bpm)));
    tempo.setTimeStamp (0);
    seq.addEvent (tempo);
    for (const auto& h : hits)
    {
        const auto note = juce::jlimit (0, 127, rootNote + h.semi);
        const auto vel = (juce::uint8) juce::jlimit (1, 127, (int) std::lround (h.vel * 127.0f));
        seq.addEvent (juce::MidiMessage::noteOn (channel, note, vel), std::round (h.beat * kTicksPerBeat));
        seq.addEvent (juce::MidiMessage::noteOff (channel, note), std::round ((h.beat + juce::jmax (0.02, h.len)) * kTicksPerBeat));
    }
    seq.updateMatchedPairs();
    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (kTicksPerBeat);
    mf.addTrack (seq);
    return mf;
}

KitExportResult exportKit (const DrumKit& kit, const juce::File& parentDir, const std::vector<KitMidi>& midis)
{
    KitExportResult r;
    r.folder = freshFolder (parentDir, kit.name);
    if (! r.folder.createDirectory())
    {
        r.error = "Cannot create " + r.folder.getFullPathName();
        return r;
    }

    for (int t = 0; t < kNumDrumTypes; ++t)
    {
        const auto type = drumTypeFromIndex (t);
        std::vector<DrumSoundPtr> sounds;
        juce::String err;
        for (const auto& k : kit.kept)
            if (k.type == type)
                if (auto s = DrumKit::renderKept (k, kKitExportRate, err))
                    sounds.push_back (s);
        if (sounds.empty())
            if (auto s = kit.renderSlot (type, kKitExportRate, err))
                sounds.push_back (s);

        auto dir = r.folder.getChildFile (drumTypeFolder (type));
        dir.createDirectory();
        int number = 1;
        for (const auto& s : sounds)
        {
            auto snd = *s;
            for (const auto& k : kit.kept)
                if (k.type == type && k.seed == s->seed && k.name.isNotEmpty() && ! s->fromFile())
                    snd.name = k.name;
            if (writeWav24 (snd.audio, kKitExportRate, dir.getChildFile (soundFileName (kit.name, type, number++, snd))))
                ++r.wavs;
        }
    }

    if (! midis.empty())
    {
        auto dir = r.folder.getChildFile ("MIDI");
        dir.createDirectory();
        for (const auto& m : midis)
        {
            auto f = dir.getChildFile (safeFileName (kit.name + " - " + m.name) + ".mid");
            f.deleteFile();
            if (juce::FileOutputStream os (f); os.openedOk() && m.midi.writeTo (os, 1))
                ++r.midis;
        }
    }

    writeReadme (r.folder, kit.name, r.wavs);
    r.ok = r.wavs > 0;
    if (! r.ok)
        r.error = "No sound could be written";
    return r;
}

KitExportResult exportOneShotKit (DrumType type, int count, int mood, const DrumShape& shape, juce::uint32 firstSeed,
                                  const juce::String& kitName, const juce::File& parentDir)
{
    KitExportResult r;
    r.folder = freshFolder (parentDir, kitName);
    auto dir = r.folder.getChildFile (drumTypeFolder (type));
    if (! dir.createDirectory())
    {
        r.error = "Cannot create " + dir.getFullPathName();
        return r;
    }
    count = juce::jlimit (1, 500, count);
    juce::StringArray usedNames;
    for (int i = 0; i < count; ++i)
    {
        auto s = synthesizeDrum (type, firstSeed + (juce::uint32) i, mood, shape, kKitExportRate);
        auto snd = *s;
        // the name generator can repeat - one-shot kits get unique names
        for (int k = 2; usedNames.contains (snd.name); ++k)
            snd.name = s->name + " " + juce::String (k);
        usedNames.add (snd.name);
        if (writeWav24 (snd.audio, kKitExportRate, dir.getChildFile (soundFileName (kitName, type, i + 1, snd))))
            ++r.wavs;
    }
    writeReadme (r.folder, kitName, r.wavs);
    r.ok = r.wavs == count;
    if (! r.ok)
        r.error = "Only " + juce::String (r.wavs) + " of " + juce::String (count) + " sounds were written";
    return r;
}

} // namespace rk
