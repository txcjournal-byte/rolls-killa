#pragma once

#include "DrumKit.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace rk
{

struct KitExportResult
{
    bool ok = false;
    juce::File folder;
    int wavs = 0, midis = 0;
    juce::String error;
};

/** A MIDI pattern that goes into the kit's MIDI folder. */
struct KitMidi
{
    juce::String name;      // "808 Pattern"
    juce::MidiFile midi;
};

constexpr double kKitExportRate = 44100.0;

/**
    Writes a drum kit folder:  <parent>/<kit name>/808s, Kicks, Snares, Claps, Hi-Hats, Open Hats, Percussion, FX, MIDI
    (24-bit 44.1 kHz WAVs). A drawer exports its kept sounds, or its current sound when nothing was kept.
    Never overwrites: an existing folder gets " 2", " 3" ...
*/
KitExportResult exportKit (const DrumKit& kit, const juce::File& parentDir, const std::vector<KitMidi>& midis);

/** A one-shot kit: <count> different sounds of one type (seeds firstSeed, firstSeed + 1, ...). */
KitExportResult exportOneShotKit (DrumType type, int count, int mood, const DrumShape& shape, juce::uint32 firstSeed,
                                  const juce::String& kitName, const juce::File& parentDir);

/** A drawer's pattern as a MIDI file (tempo + one track, notes = root + semi). */
juce::MidiFile drumHitsToMidi (const std::vector<DrumHit>& hits, int rootNote, double bpm, const juce::String& trackName, int channel = 1);

/** File-system safe name ("Grave Tank (F#)" stays, "a/b:c" -> "a b c"). */
juce::String safeFileName (const juce::String&);

bool writeWav24 (const juce::AudioBuffer<float>& audio, double sampleRate, const juce::File& file);

} // namespace rk
