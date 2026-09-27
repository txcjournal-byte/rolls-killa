#pragma once

#include "PatternPlayer.h"
#include "RollLibrary.h"

namespace rk
{

/** Everything that decides the pattern that plays. Plain data, compared to detect changes. */
struct ModelSettings
{
    int presetIndex = 0;
    int bars = 4;

    bool operator== (const ModelSettings& o) const noexcept
    {
        return presetIndex == o.presetIndex && bars == o.bars;
    }
    bool operator!= (const ModelSettings& o) const noexcept { return ! (*this == o); }
};

/**
    Message-thread model: preset -> tiled to bars -> (variation, edits, shaping) -> playable pattern.
*/
class RollModel
{
public:
    explicit RollModel (RollLibrary& library);

    /** Recomputes the pattern if the settings changed. Returns true when the result changed. */
    bool update (const ModelSettings& settings, bool force = false);

    const ModelSettings& getSettings() const noexcept { return settings; }
    const Preset& getPreset() const;
    const Pattern& getPattern() const noexcept { return shaped; }

    std::unique_ptr<PlaybackPattern> makePlayback (int rootNote) const;

    RollLibrary& getLibrary() noexcept { return library; }

private:
    RollLibrary& library;
    ModelSettings settings;
    bool valid = false;
    Pattern shaped;
};

} // namespace rk
