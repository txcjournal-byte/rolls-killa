#include "RollModel.h"

namespace rk
{

RollModel::RollModel (RollLibrary& lib) : library (lib) {}

const Preset& RollModel::getPreset() const
{
    return library.getPreset (settings.presetIndex);
}

bool RollModel::update (const ModelSettings& newSettings, bool force)
{
    if (valid && ! force && newSettings == settings)
        return false;

    settings = newSettings;
    settings.presetIndex = juce::jlimit (0, std::max (0, library.getNumPresets() - 1), settings.presetIndex);
    valid = true;

    if (library.getNumPresets() == 0)
    {
        shaped = {};
        return true;
    }

    shaped = tileToBars (getPreset().pattern, settings.bars);
    return true;
}

std::unique_ptr<PlaybackPattern> RollModel::makePlayback (int rootNote) const
{
    const auto bpm = library.getNumPresets() > 0 ? getPreset().bpmHint : 140.0;
    return std::make_unique<PlaybackPattern> (PlaybackPattern::fromPattern (shaped, rootNote, bpm));
}

} // namespace rk
