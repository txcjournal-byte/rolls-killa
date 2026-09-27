#pragma once

#include "Pattern.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace rk
{

struct MidiExportOptions
{
    double bpm = 140.0;
    int rootNote = 60;
    int channel = 1;
    juce::String trackName = "Rolls Killa";
    int ppq = kTicksPerBeat;
};

/** Builds a type-1 MIDI file: track 0 = tempo/time signature, track 1 = the hats. Muted notes are skipped. */
juce::MidiFile createMidiFile (const Pattern& pattern, const MidiExportOptions& options);

bool writeMidiFile (const Pattern& pattern, const MidiExportOptions& options, const juce::File& file);

/** Writes the pattern to a temp .mid for drag & drop into a DAW. Returns the file (or an invalid File). */
juce::File createDragAndDropMidiFile (const Pattern& pattern, const MidiExportOptions& options, const juce::String& baseName);

} // namespace rk
