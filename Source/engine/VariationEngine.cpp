#include "VariationEngine.h"
#include "PatternShaper.h"
#include "Random.h"

#include <algorithm>
#include <cmath>

namespace rk
{

namespace
{
    bool isLocked (uint32_t mask, int bar) noexcept { return bar >= 0 && bar < 32 && (mask & (1u << bar)) != 0; }

    std::vector<Roll> rollsInBar (const Pattern& p, int bar)
    {
        std::vector<Roll> out;
        for (const auto& r : findRolls (p))
            if (r.bar() == bar)
                out.push_back (r);
        return out;
    }

    Roll* rollAt (std::vector<Roll>& rolls, double start)
    {
        for (auto& r : rolls)
            if (std::abs (r.start - start) < 1.0e-6)
                return &r;
        return nullptr;
    }

    std::vector<double> startCandidates (const CategoryProfile& profile, int bar, Rng& rng)
    {
        std::vector<double> c;
        const auto grid = profile.allowSkipStarts ? 0.25 : 0.5;
        for (double pos = 0.0; pos < kBeatsPerBar - 0.2; pos += grid)
            c.push_back (bar * kBeatsPerBar + pos);
        rng.shuffle (c);
        return c;
    }

    /** Re-inserts a roll with a new shape at 'start' (after removing the old one). Returns false if no room. */
    bool placeRoll (Pattern& p, double start, const RollShape& shape, int bar)
    {
        const auto lo = bar * (double) kBeatsPerBar;
        if (! isFreeForRoll (p, start, shapeLength (shape), lo, lo + kBeatsPerBar))
            return false;
        insertRoll (p, start, shape);
        return true;
    }

    void varyBar (Pattern& p, const CategoryProfile& profile, Rng& rng, double amount, int bar, bool isPhraseEnd,
                  double grid, int baseVel)
    {
        // 1) mutate existing rolls
        auto rolls = rollsInBar (p, bar);
        std::vector<double> starts;
        for (const auto& r : rolls)
            starts.push_back (r.start);

        for (auto start : starts)
        {
            auto current = findRolls (p);
            auto* r = rollAt (current, start);
            if (r == nullptr)
                continue;

            const auto original = shapeOfRoll (p, *r);
            auto shape = original;
            bool changed = false;

            if (rng.chance (amount * 0.15) && ! isPhraseEnd)
            {
                removeRoll (p, *r, grid, baseVel);
                continue;                                         // drop the roll
            }

            if (rng.chance (amount * 0.55))
            {
                // new speed for the whole roll, same duration
                const auto duration = shapeLength (shape);
                auto rate = rng.weighted (profile.rateWeights);
                if (shape.segments.size() == 1 && rate == shape.segments[0].rate)
                    rate = rng.weighted (profile.rateWeights);
                shape.segments = { { rate, std::max (2, (int) std::lround (duration / kRollSteps[(size_t) rate])) } };
                changed = true;
            }

            if (rng.chance (amount * 0.35 * profile.pitchChance) && ! profile.pitchSteps.empty())
            {
                const auto interval = profile.pitchSteps[(size_t) rng.range (0, (int) profile.pitchSteps.size() - 1)];
                shape.pitches = rng.chance (0.5) ? std::vector<int> { 0, interval } : std::vector<int> { 0, 0, interval, interval };
                changed = true;
            }

            if (rng.chance (amount * profile.rampChance * 0.6))
            {
                const auto v = std::clamp (shape.vels.empty() ? baseVel : shape.vels.front(), profile.velMin, profile.velMax);
                const auto low = (int) std::lround (v * 0.65);
                shape.vels = rng.chance (0.7) ? std::vector<int> { low, v } : std::vector<int> { v, low };
                changed = true;
            }

            const auto move = rng.chance (amount * 0.35);
            if (! changed && ! move)
                continue;

            removeRoll (p, *r, grid, baseVel);

            bool placed = false;
            if (move)
                for (auto pos : startCandidates (profile, bar, rng))
                    if (std::abs (pos - start) > 1.0e-6 && placeRoll (p, pos, shape, bar))
                    {
                        placed = true;
                        break;
                    }

            if (! placed)
                placed = placeRoll (p, start, shape, bar);

            if (! placed)
                insertRoll (p, start, original);   // new shape doesn't fit: put the original back
        }

        // 2) maybe add a roll (phrase end prefers a longer fill late in the bar)
        const auto current = (int) rollsInBar (p, bar).size();
        const auto limit = profile.maxRollsPerTwoBars > 0 ? profile.maxRollsPerTwoBars : 3;
        if (current < limit && rng.chance (amount * std::min (1.0, (double) profile.rollsPerBar) * (isPhraseEnd ? 0.8 : 0.45)))
        {
            auto candidates = startCandidates (profile, bar, rng);
            if (isPhraseEnd)
                std::stable_sort (candidates.begin(), candidates.end(), [] (double a, double b)
                                  { return std::fmod (a, (double) kBeatsPerBar) > std::fmod (b, (double) kBeatsPerBar); });

            const auto maxLen = isPhraseEnd ? 1.5 : 0.75;
            auto shape = makeRollShape (rng, profile, maxLen, baseVel, true);
            for (auto pos : candidates)
                if (placeRoll (p, pos, shape, bar))
                    break;
        }
    }

    /** PRIMITIVUS & co: never more rolls than the category allows per two bars. */
    void enforceRollLimit (Pattern& p, const CategoryProfile& profile, uint32_t lockedBars, double grid, int baseVel)
    {
        if (profile.maxRollsPerTwoBars <= 0)
            return;

        for (int w = 0; w < p.bars; w += 2)
        {
            for (;;)
            {
                auto rolls = findRolls (p);
                std::vector<Roll> inWindow;
                for (const auto& r : rolls)
                    if ((r.bar() == w || r.bar() == w + 1))
                        inWindow.push_back (r);

                if ((int) inWindow.size() <= profile.maxRollsPerTwoBars)
                    break;

                // remove the first unlocked roll of the window
                auto victim = std::find_if (inWindow.begin(), inWindow.end(), [&] (const Roll& r) { return ! isLocked (lockedBars, r.bar()); });
                if (victim == inWindow.end())
                    break;
                removeRoll (p, *victim, grid, baseVel);
            }
        }
    }
}

Pattern varyPattern (const Pattern& source, const CategoryProfile& profile, uint32_t seed, double amount, uint32_t lockedBars)
{
    Pattern p = source;
    p.sort();

    if (seed == 0 || amount <= 0.0)
        return p;

    amount = std::clamp (amount, 0.0, 1.0);
    const auto grid = detectBaseGrid (p);
    const auto baseVel = detectBaseVelocity (p);

    for (int bar = 0; bar < p.bars; ++bar)
    {
        if (isLocked (lockedBars, bar))
            continue;

        Rng rng ((uint64_t) seed * 131u + (uint64_t) bar);
        const auto phraseEnd = bar == p.bars - 1 || (bar % 4) == 3;
        varyBar (p, profile, rng, amount, bar, phraseEnd, grid, baseVel);
    }

    enforceRollLimit (p, profile, lockedBars, grid, baseVel);
    p.tidy();
    return p;
}

} // namespace rk
