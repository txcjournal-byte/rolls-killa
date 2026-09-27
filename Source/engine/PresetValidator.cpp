#include "PresetValidator.h"

#include <algorithm>
#include <cmath>

namespace rk
{

juce::StringArray validatePatternInvariants (const Pattern& pattern)
{
    juce::StringArray errors;
    const auto total = pattern.lengthBeats();

    for (size_t i = 0; i < pattern.notes.size(); ++i)
    {
        const auto& n = pattern.notes[i];
        const auto where = "note " + juce::String ((int) i) + " @" + juce::String (n.beat, 4);

        if (n.vel < 1 || n.vel > 127)
            errors.add (where + ": velocity " + juce::String (n.vel) + " out of 1..127");
        if (n.beat < -kEps || n.beat >= total - kEps)
            errors.add (where + ": outside pattern length " + juce::String (total));
        if (n.len <= 0.0)
            errors.add (where + ": non-positive length");
        if (i > 0 && n.beat < pattern.notes[i - 1].beat - kEps)
            errors.add (where + ": notes not sorted");

        for (size_t j = i + 1; j < pattern.notes.size(); ++j)
        {
            const auto& m = pattern.notes[j];
            if (m.beat >= n.beat + n.len - 1.0e-6)
                break;
            if (m.pitch == n.pitch)
            {
                errors.add (where + ": overlaps next note of the same pitch");
                break;
            }
        }
    }

    return errors;
}

juce::StringArray validatePreset (const Preset& preset)
{
    auto errors = validatePatternInvariants (preset.pattern);
    const auto& profile = getCategoryProfile (preset.category);
    const auto bars = preset.pattern.bars;

    if (preset.isFactory && (bars != 1 && bars != 2 && bars != 4))
        errors.add ("bars must be 1, 2 or 4 (is " + juce::String (bars) + ")");
    if (bars < 1 || bars > 8)
        errors.add ("bars out of range");
    if (preset.pattern.notes.empty())
        errors.add ("no notes");
    if (preset.isFactory && preset.category == kUserCategory)
        errors.add ("factory preset in USER category");

    if (preset.isFactory && (preset.bpmHint < profile.bpmMin - kEps || preset.bpmHint > profile.bpmMax + kEps))
        errors.add ("bpmHint " + juce::String (preset.bpmHint) + " outside category range");

    if (! preset.pattern.notes.empty() && preset.pattern.notes.back().beat < (bars - 1) * kBeatsPerBar - kEps)
        errors.add ("last bar is empty - pattern length does not match bars");

    const auto rolls = findRolls (preset.pattern);
    const auto& notes = preset.pattern.notes;

    for (const auto& r : rolls)
    {
        const auto where = "roll @" + juce::String (r.start, 4);

        for (int k = r.first; k < r.last(); ++k)
        {
            const auto step = notes[(size_t) k + 1].beat - notes[(size_t) k].beat;
            if (rateIndexForStep (step) < 0)
                errors.add (where + ": step " + juce::String (step, 4) + " is not an allowed roll rate");
        }

        const auto onEighth = isOnGrid (r.start, 0.5);
        const auto onSkip = isOnGrid (r.start, 0.25) || isOnGrid (r.start, 1.0 / 6.0);

        if (! onEighth && ! (profile.allowSkipStarts && onSkip))
            errors.add (where + ": roll must start on a beat or an 1/8");

        if (r.mixedRates && ! profile.mixedRates)
            errors.add (where + ": mixed roll rates only allowed in MANIA");
    }

    if (profile.maxRollsPerTwoBars > 0)
    {
        if (bars < 2 && ! rolls.empty())
            errors.add ("1-bar preset in this category may not contain a roll (loops to 2 rolls per 2 bars)");

        for (int w = 0; w < bars; w += 2)
        {
            const auto inWindow = std::count_if (rolls.begin(), rolls.end(), [w] (const Roll& r) { return r.bar() == w || r.bar() == w + 1; });
            if (inWindow > profile.maxRollsPerTwoBars)
                errors.add ("too many rolls in bars " + juce::String (w + 1) + "-" + juce::String (w + 2));
        }
    }

    return errors;
}

//==============================================================================
VelocityShape classifyVelocity (const Pattern& p, const Roll& roll)
{
    const auto first = p.notes[(size_t) roll.first].vel;
    const auto last = p.notes[(size_t) roll.last()].vel;
    int lo = 127, hi = 0;
    bool nonDecreasing = true, nonIncreasing = true;

    for (int k = roll.first; k <= roll.last(); ++k)
    {
        const auto v = p.notes[(size_t) k].vel;
        lo = std::min (lo, v);
        hi = std::max (hi, v);
        if (k > roll.first)
        {
            const auto prev = p.notes[(size_t) k - 1].vel;
            if (v < prev - 2) nonDecreasing = false;
            if (v > prev + 2) nonIncreasing = false;
        }
    }

    if (hi - lo <= 8)
        return VelocityShape::flat;
    if (nonDecreasing && last > first)
        return VelocityShape::rampUp;
    if (nonIncreasing && last < first)
        return VelocityShape::rampDown;
    return VelocityShape::other;
}

LibraryStats computeStats (const std::vector<const Preset*>& presets)
{
    LibraryStats s;

    for (const auto* preset : presets)
    {
        const auto& p = preset->pattern;
        ++s.presets;
        s.bars += p.bars;
        s.notes += (int) p.notes.size();

        for (const auto& r : findRolls (p))
        {
            ++s.rolls;
            const auto rate = rateIndexForStep (r.step);
            if (rate >= 0)
                ++s.rollsByRate[(size_t) rate];

            for (int k = r.first; k < r.last(); ++k)
            {
                const auto ri = rateIndexForStep (p.notes[(size_t) k + 1].beat - p.notes[(size_t) k].beat);
                if (ri >= 0)
                    ++s.intervalsByRate[(size_t) ri];
            }

            switch (classifyVelocity (p, r))
            {
                case VelocityShape::flat:     ++s.flatRolls; break;
                case VelocityShape::rampUp:   ++s.rampUpRolls; break;
                case VelocityShape::rampDown: ++s.rampDownRolls; break;
                case VelocityShape::other:    ++s.otherVelRolls; break;
            }

            bool pitched = false;
            for (int k = r.first; k <= r.last(); ++k)
                pitched = pitched || p.notes[(size_t) k].pitch != p.notes[(size_t) r.first].pitch;
            if (pitched)
                ++s.pitchedRolls;

            if (isOnGrid (r.start, 1.0))      ++s.rollsOnBeat;
            else if (isOnGrid (r.start, 0.5)) ++s.rollsOnEighth;
            else                              ++s.rollsElsewhere;
        }
    }

    return s;
}

juce::String LibraryStats::toString() const
{
    auto pct = [this] (int v) { return juce::String (rolls > 0 ? 100.0 * v / rolls : 0.0, 1) + "%"; };
    juce::String out;
    out << "presets " << presets << ", bars " << bars << ", notes " << notes << ", rolls " << rolls
        << " (" << juce::String (rollsPerBar(), 2) << " per bar)\n";
    out << "roll rates:";
    for (int i = 0; i < numRollRates; ++i)
        out << " " << kRollRateNames[(size_t) i] << " " << pct (rollsByRate[(size_t) i]);
    out << "\nvelocity: flat " << pct (flatRolls) << ", ramp up " << pct (rampUpRolls)
        << ", ramp down " << pct (rampDownRolls) << ", other " << pct (otherVelRolls) << "\n";
    out << "pitched rolls " << pct (pitchedRolls) << "; start on beat " << pct (rollsOnBeat)
        << ", on 1/8 " << pct (rollsOnEighth) << ", elsewhere " << pct (rollsElsewhere);
    return out;
}

} // namespace rk
