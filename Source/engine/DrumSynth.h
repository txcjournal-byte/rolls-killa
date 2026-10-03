#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <memory>

namespace rk
{

/** The eight drawers of the kit. Order = pad order and MIDI channel order (808 = channel 2 ... FX = channel 9;
    channel 1 stays the hi-hat rolls of the original Rolls Killa). */
enum class DrumType
{
    b808, kick, snare, clap, hat, openHat, perc, fx
};

constexpr int kNumDrumTypes = 8;

const char* drumTypeName (DrumType);      // "808", "KICK", ...
const char* drumTypeFolder (DrumType);    // "808s", "Kicks", ... (folders of an exported kit)
inline DrumType drumTypeFromIndex (int i) { return (DrumType) juce::jlimit (0, kNumDrumTypes - 1, i); }

/** The knobs on top of the seeded recipe. Defaults = the recipe exactly. */
struct DrumShape
{
    float tune = 0.0f;     // semitones -12..12
    float decay = 0.5f;    // 0..1, 0.5 = recipe length (0 = half, 1 = double)
    float punch = 0.5f;    // transient 0..1
    float drive = 0.0f;    // extra saturation 0..1
    float tone = 0.5f;     // dark 0 .. bright 1
    float body = 0.5f;     // noise / metal 0 .. body / tone 1 (snare, clap, perc)
};

/** A finished one-shot (mono, normalised to -0.5 dBFS). */
struct DrumSound
{
    DrumType type = DrumType::kick;
    juce::uint32 seed = 0;
    int mood = 1;                         // 0 CHILL, 1 TRAP, 2 CRAZY
    DrumShape shape;
    juce::String name;                    // "Grave Tank"
    juce::AudioBuffer<float> audio;       // 1 channel
    double sampleRate = 44100.0;
    int rootNote = 60;                    // 808 / kick / tom: the note it sounds (MIDI plays it in tune), else 60
    juce::File file;                      // set when it came from the user's WAV
    bool fromFile() const { return file != juce::File(); }
};

using DrumSoundPtr = std::shared_ptr<const DrumSound>;

/**
    Synthesizes trap one-shots. Deterministic: the same type, seed, mood and shape always give the same sound,
    so a kit is stored as a few numbers and an exported kit can be rebuilt exactly.
    Runs on the message thread (or a worker), never on the audio thread: ~1-3 ms per sound.
*/
DrumSoundPtr synthesizeDrum (DrumType type, juce::uint32 seed, int mood, const DrumShape& shape, double sampleRate);

/** The user's own WAV/AIFF as a drum (shaped with the same knobs: tune = resample, decay = fade, punch, drive, tone). */
DrumSoundPtr loadDrumFile (DrumType type, const juce::File& file, const DrumShape& shape, double sampleRate, juce::String& error);

/** Applies the knobs to a raw recording (used for the user's WAVs). */
void shapeRecording (juce::AudioBuffer<float>& audio, double sampleRate, const DrumShape& shape);

/** A short, original, brutal name for a sound ("Grave Tank"); deterministic per type and seed. */
juce::String makeDrumName (DrumType type, juce::uint32 seed);

/** "F#" etc. of a MIDI note (808 file names carry the key). */
juce::String noteNameOf (int midiNote);

} // namespace rk
