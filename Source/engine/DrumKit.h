#pragma once

#include "DrumPatterns.h"
#include "DrumSynth.h"

#include <juce_data_structures/juce_data_structures.h>

#include <array>
#include <vector>

namespace rk
{

/** One drawer: which sound (seed / mood / knobs, or the user's file) and its pattern. Just numbers - the
    sound and pattern are rebuilt deterministically, so a project stores a whole kit in a few bytes. */
struct KitSlot
{
    juce::uint32 seed = 1;
    int mood = 1;
    DrumShape shape;
    juce::String file;           // the user's WAV (empty = synthesized)
    float volumeDb = 0.0f;
    bool muted = false;

    bool patternOn = false;      // the drawer plays its pattern with the song
    juce::uint32 patternSeed = 1;
    int patternStyle = 0;
    int patternBars = 2;
    float patternDensity = 0.5f;
};

/** A sound the producer kept for the exported kit. */
struct KeptSound
{
    DrumType type = DrumType::kick;
    juce::uint32 seed = 1;
    int mood = 1;
    DrumShape shape;
    juce::String file;
    juce::String name;
};

class DrumKit
{
public:
    DrumKit();

    std::array<KitSlot, kNumDrumTypes> slots;
    std::vector<KeptSound> kept;
    juce::String name { "ROLLS KILLA KIT" };

    KitSlot& slot (DrumType t) { return slots[(size_t) t]; }
    const KitSlot& slot (DrumType t) const { return slots[(size_t) t]; }

    /** The drawer's current sound (synthesized or the user's file, with the knobs). */
    DrumSoundPtr renderSlot (DrumType t, double sampleRate, juce::String& error) const;
    static DrumSoundPtr renderKept (const KeptSound& k, double sampleRate, juce::String& error);

    /** The drawer's pattern (empty for the hi-hat - that one is the rolls engine). */
    std::vector<DrumHit> pattern (DrumType t) const;

    /** Adds the drawer's current sound to the kit list (no duplicates). Returns false if it was there already. */
    bool keep (DrumType t, const juce::String& soundName);
    int numKept (DrumType t) const;

    juce::ValueTree toValueTree() const;
    void fromValueTree (const juce::ValueTree&);

    static constexpr const char* kTreeType = "KIT";
};

} // namespace rk
