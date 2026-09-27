#pragma once

#include "CategoryProfile.h"
#include "Pattern.h"
#include "Random.h"

#include <cstdint>

namespace rk
{

enum class VelocityMode { original = 0, flat, rampUp, rampDown };

/** The "fine tune" knobs. Default values leave the pattern exactly as it is. */
struct ShapeParams
{
    int speedShift = 0;         // -1 slower, 0 original, +1 faster (per roll, along 1/24 -> 1/32 -> 1/48 -> 1/64 -> 1/96)
    int density = 100;          // 0..200 % (100 = original)
    VelocityMode velMode = VelocityMode::original;
    int groove = 0;             // 0..100 % humanization
    int pitchRamp = 0;          // -12..12 semitones added across each roll
    int swing = 0;              // 0..60 %

    bool isNeutral() const noexcept
    {
        return speedShift == 0 && density == 100 && velMode == VelocityMode::original
            && groove == 0 && pitchRamp == 0 && swing == 0;
    }

    bool operator== (const ShapeParams& o) const noexcept
    {
        return speedShift == o.speedShift && density == o.density && velMode == o.velMode
            && groove == o.groove && pitchRamp == o.pitchRamp && swing == o.swing;
    }
};

/**
    Applies the knobs non-destructively to a pattern (usually the preset after KILL/edits).
    Deterministic for a given seed. Notes keep their srcTick where they map to a source note.
*/
Pattern shapePattern (const Pattern& source, const ShapeParams& params, const CategoryProfile& profile, uint32_t seed);

/** Shifts a rate index one step along the speed ladder. */
int shiftedRate (int rate, int shift) noexcept;

/** Checks whether a roll of 'length' beats can go at 'start' without touching other rolls or leaving [lo, hi). */
bool isFreeForRoll (const Pattern& p, double start, double length, double lo, double hi);

/** Picks a roll shape for a category (rate, count, velocity, pitch). */
RollShape makeRollShape (Rng& rng, const CategoryProfile& profile, double maxLength, int baseVel, bool allowPitch);

} // namespace rk
