#include "RollModel.h"
#include "VariationEngine.h"

#include <algorithm>

namespace rk
{

RollModel::RollModel (RollLibrary& lib) : library (lib) {}

const Preset& RollModel::getPreset() const
{
    return library.getPreset (settings.presetIndex);
}

const CategoryProfile& RollModel::getProfile() const
{
    return getCategoryProfile (getPreset().category);
}

void RollModel::applyRemovals (Pattern& p, const NoteEdits& edits, bool sourceNotes)
{
    if (edits.empty())
        return;

    auto keyOf = [sourceNotes] (const Note& n) { return sourceNotes ? n.srcTick : (n.srcTick < 0 ? beatToTick (n.beat) : -1); };
    auto find = [&edits] (int key) -> const NoteEdit* { const auto it = edits.find (key); return it != edits.end() ? &it->second : nullptr; };

    // whole rolls first (refill the grid so the groove keeps going)
    const auto grid = detectBaseGrid (p);
    const auto baseVel = detectBaseVelocity (p);
    for (bool again = true; again;)
    {
        again = false;
        for (const auto& r : findRolls (p))
        {
            const auto key = keyOf (p.notes[(size_t) r.first]);
            if (const auto* e = find (key); key >= 0 && e != nullptr && e->removeRoll)
            {
                removeRoll (p, r, grid, baseVel);
                again = true;
                break;
            }
        }
    }

    // notes the knobs generated can be muted / re-velocitied by position too
    if (! sourceNotes)
        for (auto& n : p.notes)
            if (n.srcTick < 0)
                if (const auto* e = find (beatToTick (n.beat)))
                {
                    n.muted = n.muted || e->muted;
                    if (e->vel > 0)
                        n.vel = juce::jlimit (1, 127, e->vel);
                }

    // single notes
    p.notes.erase (std::remove_if (p.notes.begin(), p.notes.end(), [&] (const Note& n)
                                   {
                                       const auto key = keyOf (n);
                                       const auto* e = key >= 0 ? find (key) : nullptr;
                                       return e != nullptr && e->deleted;
                                   }),
                   p.notes.end());
    p.tidy();
}

Pattern RollModel::build (const RollLibrary& library, const ModelSettings& s, Pattern* editableOut)
{
    if (library.getNumPresets() == 0)
        return {};

    const auto& preset = library.getPreset (s.presetIndex);
    const auto& profile = getCategoryProfile (preset.category);

    auto p = tileToBars (preset.pattern, s.bars);
    p = varyPattern (p, profile, s.seed, s.variation / 100.0, s.lockedBars);

    for (auto& n : p.notes)
    {
        n.srcTick = beatToTick (n.beat);
        n.rateOverride = -1;

        const auto it = s.edits.find (n.srcTick);
        if (it == s.edits.end())
            continue;

        n.muted = it->second.muted;
        if (it->second.vel > 0)
            n.vel = juce::jlimit (1, 127, it->second.vel);
        if (it->second.rate >= 0)
            n.rateOverride = it->second.rate;
    }

    // Removals keyed by source notes (before the knobs)
    applyRemovals (p, s.edits, true);
    markRolls (p);
    if (editableOut != nullptr)
        *editableOut = p;

    auto shaped = shapePattern (p, s.shape, profile, s.seed ^ 0x5eedu);

    // Removals of notes the knobs generated (e.g. a roll re-timed by Roll Speed), keyed by their position
    applyRemovals (shaped, s.edits, false);
    markRolls (shaped);
    return shaped;
}

bool RollModel::update (const ModelSettings& newSettings, bool force)
{
    if (valid && ! force && newSettings == settings)
        return false;

    settings = newSettings;
    settings.presetIndex = juce::jlimit (0, std::max (0, library.getNumPresets() - 1), settings.presetIndex);
    valid = true;

    shaped = build (library, settings, &edited);
    return true;
}

std::unique_ptr<PlaybackPattern> RollModel::makePlayback (int rootNote) const
{
    const auto bpm = library.getNumPresets() > 0 ? getPreset().bpmHint : 140.0;
    return std::make_unique<PlaybackPattern> (PlaybackPattern::fromPattern (shaped, rootNote, bpm));
}

} // namespace rk
