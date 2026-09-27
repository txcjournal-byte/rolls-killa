#include "RollModel.h"
#include "VariationEngine.h"

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

    markRolls (p);
    if (editableOut != nullptr)
        *editableOut = p;

    return shapePattern (p, s.shape, profile, s.seed ^ 0x5eedu);
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
