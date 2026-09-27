#include "MidiExport.h"

namespace rk
{

juce::MidiFile createMidiFile (const Pattern& pattern, const MidiExportOptions& o)
{
    juce::MidiFile file;
    file.setTicksPerQuarterNote (o.ppq);

    const auto toTicks = [&] (double beat) { return (double) juce::roundToInt (beat * o.ppq); };
    const auto endTick = toTicks (pattern.lengthBeats());

    juce::MidiMessageSequence meta;
    auto tempo = juce::MidiMessage::tempoMetaEvent (juce::roundToInt (60000000.0 / juce::jmax (1.0, o.bpm)));
    tempo.setTimeStamp (0.0);
    meta.addEvent (tempo);
    auto timeSig = juce::MidiMessage::timeSignatureMetaEvent (4, 4);
    timeSig.setTimeStamp (0.0);
    meta.addEvent (timeSig);
    auto metaEnd = juce::MidiMessage::endOfTrack();
    metaEnd.setTimeStamp (endTick);
    meta.addEvent (metaEnd);

    juce::MidiMessageSequence hats;
    auto name = juce::MidiMessage::textMetaEvent (3, o.trackName);
    name.setTimeStamp (0.0);
    hats.addEvent (name);

    for (const auto& n : pattern.notes)
    {
        if (n.muted)
            continue;

        const auto note = juce::jlimit (0, 127, o.rootNote + n.pitch);
        const auto on = toTicks (n.beat);
        const auto off = juce::jmax (on + 1.0, juce::jmin (endTick, toTicks (n.beat + n.len)));

        auto mOn = juce::MidiMessage::noteOn (o.channel, note, (juce::uint8) juce::jlimit (1, 127, n.vel));
        mOn.setTimeStamp (on);
        auto mOff = juce::MidiMessage::noteOff (o.channel, note);
        mOff.setTimeStamp (off);
        hats.addEvent (mOn);
        hats.addEvent (mOff);
    }

    hats.updateMatchedPairs();
    auto end = juce::MidiMessage::endOfTrack();
    end.setTimeStamp (endTick);
    hats.addEvent (end);
    hats.sort();

    file.addTrack (meta);
    file.addTrack (hats);
    return file;
}

bool writeMidiFile (const Pattern& pattern, const MidiExportOptions& options, const juce::File& file)
{
    const auto midi = createMidiFile (pattern, options);
    file.deleteFile();
    juce::FileOutputStream out (file);
    if (! out.openedOk())
        return false;
    return midi.writeTo (out, 1);
}

juce::File createDragAndDropMidiFile (const Pattern& pattern, const MidiExportOptions& options, const juce::String& baseName)
{
    const auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Rolls Killa MIDI");
    dir.createDirectory();

    // Keep the temp folder small: drop files older than a day.
    for (const auto& old : dir.findChildFiles (juce::File::findFiles, false, "*.mid"))
        if (old.getLastModificationTime() < juce::Time::getCurrentTime() - juce::RelativeTime::days (1))
            old.deleteFile();

    const auto file = dir.getChildFile (juce::File::createLegalFileName (baseName) + ".mid");
    return writeMidiFile (pattern, options, file) ? file : juce::File();
}

} // namespace rk
