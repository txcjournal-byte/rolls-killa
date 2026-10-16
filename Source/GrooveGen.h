#pragma once
#include <juce_core/juce_core.h>
#include <array>

// Hi-hat groove generator. A groove is up to 16 beats (4 bars); every beat is split into 4, 3 or 6 slots
// and every slot holds 0..6 hits (2+ = a roll inside that slot).
namespace GrooveGen
{
    constexpr int maxBeats = 16, maxSlots = 6, maxHits = 6;

    struct Groove
    {
        std::array<int, maxBeats> div {};
        std::array<std::array<int, maxSlots>, maxBeats> hits {};
        Groove() { div.fill (4); }
        bool operator== (const Groove& o) const { return div == o.div && hits == o.hits; }
    };

    // Roll styles (feel, not genre). MIX picks one at random on every GENERATE.
    enum Style { mix = 0, steady, endRoll, triplet, stutter, bounce, sparse, flood, chaos, numStyles };
    juce::String styleName (int style);

    // complexity 0 = plain, 1 = busy. bars = 1, 2 or 4 (bars after the first are variations of it).
    Groove generate (int style, float complexity, int bars, juce::Random& rng, int* styleUsed = nullptr);

    // Factory presets: a fixed, named set (made by the generator from fixed seeds, so they never change).
    int numPresets();
    juce::String presetName (int index);
    Groove preset (int index, int bars);

    juce::String toString (const Groove&);
    bool fromString (const juce::String&, Groove&);
}
