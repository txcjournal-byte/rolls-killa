#include "PatternShaper.h"

#include <algorithm>
#include <cmath>

namespace rk
{

int shiftedRate (int rate, int shift) noexcept
{
    return std::clamp (rate + shift, (int) rate24, (int) rate96);
}

bool isFreeForRoll (const Pattern& p, double start, double length, double lo, double hi)
{
    if (start < lo - kEps || start + length > hi + kEps)
        return false;

    const auto rolls = findRolls (p);
    std::vector<bool> inRoll (p.notes.size(), false);
    for (const auto& r : rolls)
        for (int k = r.first; k <= r.last(); ++k)
            inRoll[(size_t) k] = true;

    const auto zone = rollClearZone (start, start + length - kRollSteps[rate96]);
    const auto zoneStart = zone.first;
    const auto zoneEnd = zone.second;

    for (size_t i = 0; i < p.notes.size(); ++i)
    {
        const auto b = p.notes[i].beat;
        if (b <= zoneStart || b >= zoneEnd)
            continue;
        if (inRoll[i] || b < lo - kEps || b >= hi - kEps)
            return false;
    }

    return true;
}

RollShape makeRollShape (Rng& rng, const CategoryProfile& profile, double maxLength, int baseVel, bool allowPitch)
{
    RollShape shape;

    std::vector<double> lengths;
    for (auto l : profile.rollLengths)
        if (l <= maxLength + kEps)
            lengths.push_back (l);
    if (lengths.empty())
        lengths.push_back (std::min (maxLength, *std::min_element (profile.rollLengths.begin(), profile.rollLengths.end())));

    const auto length = lengths[(size_t) rng.range (0, (int) lengths.size() - 1)];
    const auto rate = rng.weighted (profile.rateWeights);

    if (profile.mixedRates && length >= 0.5 && rng.chance (0.6))
    {
        // Speed change inside the roll: slow start, faster end (MANIA)
        const int rates[] { rng.chance (0.5) ? (int) rate24 : (int) rate32, (int) rate64, (int) rate96 };
        const auto parts = rng.chance (0.5) ? 3 : 2;
        const auto partLen = length / parts;
        for (int i = 0; i < parts; ++i)
        {
            const auto r = rates[i == parts - 1 && parts == 2 ? 2 : i];
            shape.segments.push_back ({ r, std::max (2, (int) std::lround (partLen / kRollSteps[(size_t) r])) });
        }
    }
    else
    {
        shape.segments.push_back ({ rate, std::max (2, (int) std::lround (length / kRollSteps[(size_t) rate])) });
    }

    int count = 0;
    for (const auto& s : shape.segments)
        count += s.count;

    const auto v = std::clamp (baseVel, profile.velMin, profile.velMax);
    if (rng.chance (profile.rampChance))
    {
        const auto low = (int) std::lround (v * 0.65);
        shape.vels = rng.chance (0.7) ? std::vector<int> { low, v } : std::vector<int> { v, low };
    }
    else
    {
        shape.vels = { v };
    }

    shape.pitches = { 0 };
    if (allowPitch && ! profile.pitchSteps.empty() && rng.chance (profile.pitchChance))
    {
        const auto interval = profile.pitchSteps[(size_t) rng.range (0, (int) profile.pitchSteps.size() - 1)];
        if (rng.chance (0.5))
            shape.pitches = { 0, interval };                       // ramp
        else
        {
            shape.pitches.assign ((size_t) count, 0);              // step up half-way
            for (int i = count / 2; i < count; ++i)
                shape.pitches[(size_t) i] = interval;
        }
    }

    return shape;
}

//==============================================================================
namespace
{
    Roll* rollStartingAt (std::vector<Roll>& rolls, double start)
    {
        for (auto& r : rolls)
            if (std::abs (r.start - start) < 1.0e-6)
                return &r;
        return nullptr;
    }

    void applyDensity (Pattern& p, double density, const CategoryProfile& profile, uint32_t seed)
    {
        if (std::abs (density - 1.0) < 1.0e-3)
            return;

        const auto grid = detectBaseGrid (p);
        const auto baseVel = detectBaseVelocity (p);
        auto rolls = findRolls (p);

        if (density < 1.0)
        {
            // Remove rolls: random order, but the phrase-ending fill goes last.
            std::vector<double> starts;
            for (size_t i = 0; i + 1 < rolls.size(); ++i)
                starts.push_back (rolls[i].start);
            Rng rng (seed ^ 0xD0D0u);
            rng.shuffle (starts);
            if (! rolls.empty())
                starts.push_back (rolls.back().start);

            const auto toRemove = (int) std::lround ((1.0 - density) * (double) rolls.size());
            for (int i = 0; i < toRemove && i < (int) starts.size(); ++i)
            {
                auto current = findRolls (p);
                if (auto* r = rollStartingAt (current, starts[(size_t) i]))
                    removeRoll (p, *r, grid, baseVel);
            }
            return;
        }

        // Add short rolls on free 1/8 positions.
        const auto toAdd = (int) std::lround ((density - 1.0) * p.bars * std::max (0.5f, profile.rollsPerBar));
        std::vector<double> candidates;
        for (double pos = 0.0; pos < p.lengthBeats() - 0.25; pos += 0.5)
            candidates.push_back (pos);

        Rng rng (seed ^ 0xADD5u);
        rng.shuffle (candidates);

        int added = 0;
        for (auto pos : candidates)
        {
            if (added >= toAdd)
                break;

            const auto bar = std::floor (pos / kBeatsPerBar) * kBeatsPerBar;
            auto shape = makeRollShape (rng, profile, 0.5, baseVel, false);
            if (! isFreeForRoll (p, pos, shapeLength (shape), bar, bar + kBeatsPerBar))
                continue;

            insertRoll (p, pos, shape);
            ++added;
        }
    }

    void applySpeed (Pattern& p, int shift)
    {
        auto rolls = findRolls (p);
        bool anyOverride = false;
        for (const auto& r : rolls)
            anyOverride = anyOverride || p.notes[(size_t) r.first].rateOverride >= 0;

        if (shift == 0 && ! anyOverride)
            return;

        struct Job { double start; RollShape shape; int srcTick; int rateOverride; };
        std::vector<Job> jobs;

        for (const auto& r : rolls)
        {
            const auto& first = p.notes[(size_t) r.first];
            auto shape = shapeOfRoll (p, r);
            const auto duration = shapeLength (shape);

            if (first.rateOverride >= 0)
            {
                const auto rate = std::clamp (first.rateOverride, 0, numRollRates - 1);
                shape.segments = { { rate, std::max (2, (int) std::lround (duration / kRollSteps[(size_t) rate])) } };
            }
            else if (shift != 0)
            {
                for (auto& seg : shape.segments)
                {
                    const auto segDur = seg.count * kRollSteps[(size_t) seg.rate];
                    seg.rate = shiftedRate (seg.rate, shift);
                    seg.count = std::max (2, (int) std::lround (segDur / kRollSteps[(size_t) seg.rate]));
                }
            }
            else
            {
                continue;
            }

            jobs.push_back ({ r.start, shape, first.srcTick, first.rateOverride });
        }

        for (auto it = jobs.rbegin(); it != jobs.rend(); ++it)
        {
            auto current = findRolls (p);
            auto* r = rollStartingAt (current, it->start);
            if (r == nullptr)
                continue;

            p.notes.erase (p.notes.begin() + r->first, p.notes.begin() + r->last() + 1);
            insertRoll (p, it->start, it->shape);

            for (auto& n : p.notes)
                if (std::abs (n.beat - it->start) < 1.0e-6)
                {
                    n.srcTick = it->srcTick;
                    n.rateOverride = it->rateOverride;
                    break;
                }
        }
    }

    void applyVelocityMode (Pattern& p, VelocityMode mode)
    {
        if (mode == VelocityMode::original)
            return;

        for (const auto& r : findRolls (p))
        {
            int maxVel = 1;
            double sum = 0.0;
            for (int k = r.first; k <= r.last(); ++k)
            {
                maxVel = std::max (maxVel, p.notes[(size_t) k].vel);
                sum += p.notes[(size_t) k].vel;
            }

            const auto mean = (int) std::lround (sum / r.count);
            const auto low = (int) std::lround (maxVel * 0.6);

            for (int k = r.first; k <= r.last(); ++k)
            {
                const auto t = r.count > 1 ? (double) (k - r.first) / (r.count - 1) : 1.0;
                auto& v = p.notes[(size_t) k].vel;
                if (mode == VelocityMode::flat)          v = mean;
                else if (mode == VelocityMode::rampUp)   v = (int) std::lround (low + (maxVel - low) * t);
                else                                     v = (int) std::lround (maxVel - (maxVel - low) * t);
            }
        }
    }

    void applyPitchRamp (Pattern& p, int semis)
    {
        if (semis == 0)
            return;

        for (const auto& r : findRolls (p))
            for (int k = r.first; k <= r.last(); ++k)
            {
                const auto t = r.count > 1 ? (double) (k - r.first) / (r.count - 1) : 1.0;
                p.notes[(size_t) k].pitch += (int) std::lround (semis * t);
            }
    }

    void applyGroove (Pattern& p, double groove, uint32_t seed)
    {
        if (groove <= 0.0)
            return;

        const auto rolls = findRolls (p);
        std::vector<bool> inRoll (p.notes.size(), false);
        for (const auto& r : rolls)
            for (int k = r.first; k <= r.last(); ++k)
                inRoll[(size_t) k] = true;

        for (size_t i = 0; i < p.notes.size(); ++i)
        {
            auto& n = p.notes[i];
            double delta = 0.0;

            if (isOnGrid (n.beat, 1.0))                                 delta += 10.0 * groove;   // push the beat
            else if (! isOnGrid (n.beat, 0.5) && isOnGrid (n.beat, 0.25)) delta -= 8.0 * groove;  // relax the "e"/"a"

            const auto jitter = (hash01 (seed, (uint64_t) beatToTick (n.beat)) * 2.0 - 1.0) * (inRoll[i] ? 6.0 : 12.0);
            delta += jitter * groove;
            n.vel = std::clamp ((int) std::lround (n.vel + delta), 1, 127);
        }
    }

    double swingOffset (double beat, double swing, double grid)
    {
        const auto amount = swing / 0.6;
        if (grid >= 0.5 - kEps)
            return (! isOnGrid (beat, 1.0) && isOnGrid (beat, 0.5)) ? amount * (1.0 / 6.0) : 0.0;
        return (! isOnGrid (beat, 0.5) && isOnGrid (beat, 0.25)) ? amount * (1.0 / 12.0) : 0.0;
    }

    void applySwing (Pattern& p, double swing)
    {
        if (swing <= 0.0)
            return;

        const auto grid = detectBaseGrid (p);
        const auto rolls = findRolls (p);
        std::vector<int> rollOf (p.notes.size(), -1);
        for (size_t ri = 0; ri < rolls.size(); ++ri)
            for (int k = rolls[ri].first; k <= rolls[ri].last(); ++k)
                rollOf[(size_t) k] = (int) ri;

        std::vector<double> shift (p.notes.size(), 0.0);

        for (size_t i = 0; i < p.notes.size(); ++i)
            if (rollOf[i] < 0)
                shift[i] = swingOffset (p.notes[i].beat, swing, grid);

        for (const auto& r : rolls)
        {
            auto s = swingOffset (r.start, swing, grid);
            const auto nextIndex = (size_t) r.last() + 1;
            const auto next = nextIndex < p.notes.size() ? p.notes[nextIndex].beat + shift[nextIndex] : p.lengthBeats();
            if (p.notes[(size_t) r.last()].beat + s > next - kRollThreshold)
                s = 0.0;   // no room - keep the roll where it is
            for (int k = r.first; k <= r.last(); ++k)
                shift[(size_t) k] = s;
        }

        for (size_t i = 0; i < p.notes.size(); ++i)
            p.notes[i].beat += shift[i];
    }
}

Pattern shapePattern (const Pattern& source, const ShapeParams& params, const CategoryProfile& profile, uint32_t seed)
{
    Pattern p = source;
    p.sort();

    bool hasOverrides = false;
    for (const auto& n : p.notes)
        hasOverrides = hasOverrides || n.rateOverride >= 0;

    if (params.isNeutral() && ! hasOverrides)
    {
        markRolls (p);
        return p;
    }

    applyDensity (p, params.density / 100.0, profile, seed);
    applySpeed (p, params.speedShift);
    applyVelocityMode (p, params.velMode);
    applyPitchRamp (p, params.pitchRamp);
    applyGroove (p, params.groove / 100.0, seed);
    p.tidy();
    markRolls (p);
    applySwing (p, params.swing / 100.0);
    p.tidy();
    return p;
}

} // namespace rk
