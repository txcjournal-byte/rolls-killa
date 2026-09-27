#pragma once

#include "RollLibrary.h"

namespace rk
{

/** Checks a preset against the rules from section 3.4 of the spec. Returns an empty array when valid. */
juce::StringArray validatePreset (const Preset& preset);

/** Checks the generic pattern invariants (velocity range, overlaps, length). */
juce::StringArray validatePatternInvariants (const Pattern& pattern);

struct LibraryStats
{
    int presets = 0;
    int bars = 0;
    int rolls = 0;
    int notes = 0;
    std::array<int, numRollRates> rollsByRate {};   // by the rate of the first interval
    std::array<int, numRollRates> intervalsByRate {};
    int flatRolls = 0, rampUpRolls = 0, rampDownRolls = 0, otherVelRolls = 0;
    int pitchedRolls = 0;
    int rollsOnBeat = 0, rollsOnEighth = 0, rollsElsewhere = 0;

    double tripletRollShare() const noexcept { return rolls > 0 ? (double) rollsByRate[rate24] / rolls : 0.0; }
    double rollsPerBar() const noexcept { return bars > 0 ? (double) rolls / bars : 0.0; }
    double flatVelocityShare() const noexcept { return rolls > 0 ? (double) flatRolls / rolls : 0.0; }
    juce::String toString() const;
};

enum class VelocityShape { flat, rampUp, rampDown, other };
VelocityShape classifyVelocity (const Pattern& p, const Roll& roll);

LibraryStats computeStats (const std::vector<const Preset*>& presets);

} // namespace rk
