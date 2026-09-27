#pragma once

#include "CategoryProfile.h"
#include "Pattern.h"

#include <cstdint>

namespace rk
{

/**
    KILL: a deterministic variation of a pattern inside the rules of its category.

    seed 0 or amount 0 returns the pattern unchanged. Bars whose bit is set in lockedBars are never touched.
    The result keeps the preset rules: allowed roll rates, starts on the beat/1/8 (or skip grid where the
    category allows it), no overlaps.
*/
Pattern varyPattern (const Pattern& source, const CategoryProfile& profile, uint32_t seed, double amount, uint32_t lockedBars);

} // namespace rk
