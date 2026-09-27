#include "Pattern.h"

#include <algorithm>
#include <cmath>
#include <map>

namespace rk
{

int rateIndexForStep (double stepBeats) noexcept
{
    for (int i = 0; i < numRollRates; ++i)
        if (std::abs (stepBeats - kRollSteps[(size_t) i]) < 1.0e-3)
            return i;

    return -1;
}

static int nearestRate (double stepBeats) noexcept
{
    int best = 0;
    double bestDist = 1.0e9;

    for (int i = 0; i < numRollRates; ++i)
    {
        const auto d = std::abs (std::log (stepBeats / kRollSteps[(size_t) i]));
        if (d < bestDist) { bestDist = d; best = i; }
    }

    return best;
}

int beatToTick (double beat) noexcept
{
    return (int) std::lround (beat * kTicksPerBeat);
}

bool isOnGrid (double beat, double grid) noexcept
{
    const auto r = beat / grid;
    return std::abs (r - std::round (r)) < 1.0e-3;
}

//==============================================================================
void Pattern::sort()
{
    std::stable_sort (notes.begin(), notes.end(), [] (const Note& a, const Note& b) { return a.beat < b.beat; });
}

void Pattern::tidy()
{
    sort();

    const auto total = lengthBeats();
    std::vector<Note> out;
    out.reserve (notes.size());

    for (const auto& n : notes)
    {
        if (n.beat < -kEps || n.beat >= total - kEps)
            continue;

        if (! out.empty() && std::abs (out.back().beat - n.beat) < 1.0e-6)
        {
            if (n.vel > out.back().vel)
                out.back() = n;
            continue;
        }

        out.push_back (n);
        out.back().beat = std::max (0.0, n.beat);
    }

    for (size_t i = 0; i < out.size(); ++i)
    {
        auto& n = out[i];
        n.vel = std::clamp (n.vel, 1, 127);
        const auto limit = (i + 1 < out.size() ? out[i + 1].beat : total) - n.beat;
        n.len = std::clamp (n.len, 0.01, std::max (0.01, limit * 0.95));
    }

    notes = std::move (out);
}

//==============================================================================
std::vector<Roll> findRolls (const Pattern& p)
{
    std::vector<Roll> rolls;
    const auto& n = p.notes;
    const int count = (int) n.size();

    for (int i = 0; i < count;)
    {
        // A roll never continues over a bar line: a roll that lands on the downbeat resolves there.
        int j = i;
        while (j + 1 < count && n[(size_t) j + 1].beat - n[(size_t) j].beat < kRollThreshold - kEps
               && ! isOnGrid (n[(size_t) j + 1].beat, (double) kBeatsPerBar))
            ++j;

        if (j > i)
        {
            Roll r;
            r.first = i;
            r.count = j - i + 1;
            r.start = n[(size_t) i].beat;
            r.step = n[(size_t) i + 1].beat - n[(size_t) i].beat;
            const auto lastStep = n[(size_t) j].beat - n[(size_t) j - 1].beat;
            r.length = n[(size_t) j].beat - r.start + lastStep;

            for (int k = i + 1; k < j; ++k)
                if (std::abs ((n[(size_t) k + 1].beat - n[(size_t) k].beat) - r.step) > 1.0e-3)
                    r.mixedRates = true;

            rolls.push_back (r);
        }

        i = j + 1;
    }

    return rolls;
}

void markRolls (Pattern& p)
{
    for (auto& n : p.notes)
        n.rollId = -1;

    const auto rolls = findRolls (p);
    for (size_t r = 0; r < rolls.size(); ++r)
        for (int k = rolls[r].first; k <= rolls[r].last(); ++k)
            p.notes[(size_t) k].rollId = (int) r;
}

static std::vector<bool> rollMembership (const Pattern& p, const std::vector<Roll>& rolls)
{
    std::vector<bool> inRoll (p.notes.size(), false);
    for (const auto& r : rolls)
        for (int k = r.first; k <= r.last(); ++k)
            inRoll[(size_t) k] = true;
    return inRoll;
}

double detectBaseGrid (const Pattern& p)
{
    const auto rolls = findRolls (p);
    const auto inRoll = rollMembership (p, rolls);
    const double candidates[] { 0.25, 1.0 / 3.0, 0.5, 1.0 };
    int votes[4] {};

    for (size_t i = 0; i + 1 < p.notes.size(); ++i)
    {
        if (inRoll[i] && inRoll[i + 1])
            continue;

        const auto d = p.notes[i + 1].beat - p.notes[i].beat;
        for (int c = 0; c < 4; ++c)
            if (std::abs (d - candidates[c]) < 1.0e-3)
                ++votes[c];
    }

    const auto best = (int) (std::max_element (std::begin (votes), std::end (votes)) - std::begin (votes));
    return votes[best] > 0 ? candidates[best] : 0.5;
}

int detectBaseVelocity (const Pattern& p)
{
    const auto rolls = findRolls (p);
    const auto inRoll = rollMembership (p, rolls);
    std::vector<int> v;

    for (size_t i = 0; i < p.notes.size(); ++i)
        if (! inRoll[i])
            v.push_back (p.notes[i].vel);

    if (v.empty())
        return 100;

    std::nth_element (v.begin(), v.begin() + (long) v.size() / 2, v.end());
    return v[v.size() / 2];
}

//==============================================================================
void removeRoll (Pattern& p, const Roll& roll, double grid, int baseVel)
{
    const auto lastIndex = (size_t) roll.last();
    const auto end = lastIndex + 1 < p.notes.size() ? p.notes[lastIndex + 1].beat : p.lengthBeats();
    const auto prev = roll.first > 0 ? p.notes[(size_t) roll.first - 1].beat : -1.0;

    p.notes.erase (p.notes.begin() + roll.first, p.notes.begin() + (long) lastIndex + 1);

    auto pos = std::ceil (roll.start / grid - 1.0e-6) * grid;
    for (; pos < end - kEps; pos += grid)
    {
        if (prev >= 0.0 && pos - prev < kRollThreshold - kEps)
            continue;
        if (end - pos < kRollThreshold - kEps)
            continue;

        Note n;
        n.beat = pos;
        n.len = 0.1;
        n.vel = baseVel;
        n.srcTick = -1;
        p.notes.push_back (n);
    }

    p.tidy();
}

double shapeLength (const RollShape& shape) noexcept
{
    double len = 0.0;
    for (const auto& s : shape.segments)
        len += s.count * kRollSteps[(size_t) s.rate];
    return len;
}

std::vector<int> resampleCurve (const std::vector<int>& values, int newCount)
{
    std::vector<int> out ((size_t) std::max (0, newCount), 0);

    if (values.empty() || newCount <= 0)
        return out;

    if ((int) values.size() == newCount)
        return values;

    if (values.size() == 1 || newCount == 1)
    {
        std::fill (out.begin(), out.end(), values.front());
        return out;
    }

    for (int i = 0; i < newCount; ++i)
    {
        const auto pos = (double) i * (double) (values.size() - 1) / (double) (newCount - 1);
        const auto i0 = (size_t) pos;
        const auto i1 = std::min (i0 + 1, values.size() - 1);
        const auto frac = pos - (double) i0;
        out[(size_t) i] = (int) std::lround (values[i0] + (values[i1] - values[i0]) * frac);
    }

    return out;
}

std::pair<double, double> rollClearZone (double start, double lastNote) noexcept
{
    // Notes closer than a 1/16 would merge with the roll - but never reach over a bar line.
    const auto barStart = std::floor (start / kBeatsPerBar + kEps) * kBeatsPerBar;
    const auto nextBar = (std::floor (lastNote / kBeatsPerBar + kEps) + 1.0) * kBeatsPerBar;
    return { std::max (start - kRollThreshold + kEps, barStart - kEps),
             std::min (lastNote + kRollThreshold - kEps, nextBar - kEps) };
}

int insertRoll (Pattern& p, double start, const RollShape& shape)
{
    std::vector<double> times;
    auto t = start;

    for (const auto& seg : shape.segments)
        for (int k = 0; k < seg.count; ++k)
        {
            if (t < p.lengthBeats() - kEps)
                times.push_back (t);
            t += kRollSteps[(size_t) seg.rate];
        }

    if (times.size() < 2)
        return 0;

    const auto zone = rollClearZone (start, times.back());
    const auto zoneStart = zone.first;
    const auto zoneEnd = zone.second;

    p.notes.erase (std::remove_if (p.notes.begin(), p.notes.end(),
                                   [&] (const Note& n) { return n.beat > zoneStart && n.beat < zoneEnd; }),
                   p.notes.end());

    const auto count = (int) times.size();
    const auto vels = resampleCurve (shape.vels.empty() ? std::vector<int> { 100 } : shape.vels, count);
    const auto pitches = resampleCurve (shape.pitches.empty() ? std::vector<int> { 0 } : shape.pitches, count);

    for (int i = 0; i < count; ++i)
    {
        Note n;
        n.beat = times[(size_t) i];
        n.len = 0.05;
        n.vel = vels[(size_t) i];
        n.pitch = pitches[(size_t) i];
        n.srcTick = -1;
        p.notes.push_back (n);
    }

    p.tidy();
    return count;
}

RollShape shapeOfRoll (const Pattern& p, const Roll& roll)
{
    RollShape shape;
    std::vector<int> noteRates;

    for (int k = roll.first; k <= roll.last(); ++k)
    {
        const auto& n = p.notes[(size_t) k];
        shape.vels.push_back (n.vel);
        shape.pitches.push_back (n.pitch);

        const auto a = k < roll.last() ? k : k - 1;
        const auto step = p.notes[(size_t) a + 1].beat - p.notes[(size_t) a].beat;
        const auto rate = rateIndexForStep (step);
        noteRates.push_back (rate >= 0 ? rate : nearestRate (step));
    }

    for (auto r : noteRates)
    {
        if (shape.segments.empty() || shape.segments.back().rate != r)
            shape.segments.push_back ({ r, 0 });
        ++shape.segments.back().count;
    }

    return shape;
}

//==============================================================================
Pattern tileToBars (const Pattern& preset, int targetBars)
{
    Pattern src = preset;
    src.sort();

    if (targetBars <= 0 || targetBars == src.bars)
        return src;

    const auto rolls = findRolls (src);
    std::vector<int> noteBar (src.notes.size());

    for (size_t i = 0; i < src.notes.size(); ++i)
        noteBar[i] = std::clamp ((int) (src.notes[i].beat / kBeatsPerBar + kEps), 0, src.bars - 1);

    for (const auto& r : rolls)
        for (int k = r.first; k <= r.last(); ++k)
            noteBar[(size_t) k] = std::clamp (r.bar(), 0, src.bars - 1);

    auto sourceBarFor = [&] (int k)
    {
        if (targetBars == 1)
            return 0;
        if (k == targetBars - 1)
            return src.bars - 1;           // the fill stays at the end of the phrase
        return k % src.bars;
    };

    Pattern out;
    out.bars = targetBars;

    for (int k = 0; k < targetBars; ++k)
    {
        const auto sb = sourceBarFor (k);
        const auto offset = (double) (k - sb) * kBeatsPerBar;

        for (size_t i = 0; i < src.notes.size(); ++i)
            if (noteBar[i] == sb)
            {
                auto n = src.notes[i];
                n.beat += offset;
                out.notes.push_back (n);
            }
    }

    out.tidy();
    return out;
}

} // namespace rk
