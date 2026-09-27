#pragma once

#include "PatternPlayer.h"
#include "PatternShaper.h"
#include "RollLibrary.h"

#include <map>

namespace rk
{

/** A manual edit made in the Roll Visualizer, keyed by the note's tick position. */
struct NoteEdit
{
    bool muted = false;
    int vel = -1;           // -1 = keep
    int rate = -1;          // on a roll's first note: forced rate index, -1 = keep

    bool operator== (const NoteEdit& o) const noexcept { return muted == o.muted && vel == o.vel && rate == o.rate; }
    bool isEmpty() const noexcept { return ! muted && vel < 0 && rate < 0; }
};

using NoteEdits = std::map<int, NoteEdit>;

/** Everything that decides the pattern that plays. Plain data, compared to detect changes. */
struct ModelSettings
{
    int presetIndex = 0;
    int bars = 4;
    uint32_t seed = 0;          // 0 = original preset
    int variation = 50;         // 0..100 %
    uint32_t lockedBars = 0;
    ShapeParams shape;
    NoteEdits edits;

    bool operator== (const ModelSettings& o) const noexcept
    {
        return presetIndex == o.presetIndex && bars == o.bars && seed == o.seed && variation == o.variation
            && lockedBars == o.lockedBars && shape == o.shape && edits == o.edits;
    }
    bool operator!= (const ModelSettings& o) const noexcept { return ! (*this == o); }
};

/**
    Message-thread model:
        preset -> tiled to N bars -> KILL variation (seed, amount, locks) -> visualizer edits -> knobs (PatternShaper)
*/
class RollModel
{
public:
    explicit RollModel (RollLibrary& library);

    /** Recomputes the pattern if the settings changed. Returns true when the result changed. */
    bool update (const ModelSettings& settings, bool force = false);

    const ModelSettings& getSettings() const noexcept { return settings; }
    const Preset& getPreset() const;
    const CategoryProfile& getProfile() const;

    /** Pattern after KILL + edits, before the knobs (edit keys refer to this). */
    const Pattern& getEditablePattern() const noexcept { return edited; }
    /** Final pattern that plays. */
    const Pattern& getPattern() const noexcept { return shaped; }

    std::unique_ptr<PlaybackPattern> makePlayback (int rootNote) const;

    RollLibrary& getLibrary() noexcept { return library; }

    /** Builds the whole chain for the given settings without touching the model (tests, export). */
    static Pattern build (const RollLibrary& library, const ModelSettings& settings, Pattern* editableOut = nullptr);

private:
    RollLibrary& library;
    ModelSettings settings;
    bool valid = false;
    Pattern edited, shaped;
};

} // namespace rk
