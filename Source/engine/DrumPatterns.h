#pragma once

#include "DrumSynth.h"

#include <vector>

namespace rk
{

/** One hit of a drum pattern (beats, velocity 0..1, semitones from the sound's root note, length in beats). */
struct DrumHit
{
    double beat = 0.0;
    float vel = 0.8f;
    int semi = 0;
    double len = 0.1;
};

/**
    Pattern generators for the kit drawers (808 lines, kicks, snares, claps, open hats, percs, FX).
    The hi-hat drawer uses the Rolls Killa roll engine instead. Deterministic per seed;
    every hit stays inside the pattern. Based on the KEYS KILLA drum generators.
*/
std::vector<DrumHit> generateDrumPattern (DrumType type, juce::uint32 seed, int style, int bars, float density);

/** The four styles of a drawer ("SIMPLE", "SLIDES", ...). */
const juce::StringArray& drumPatternStyles (DrumType type);

} // namespace rk
