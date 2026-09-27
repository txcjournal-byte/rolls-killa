#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace rk
{

constexpr int kBeatsPerBar = 4;
constexpr int kTicksPerBeat = 960;        // resolution used for edit keys and MIDI export
constexpr double kRollThreshold = 0.25;   // intervals shorter than a 1/16 belong to a roll
constexpr double kEps = 1.0e-4;

// Allowed roll rates, slowest to fastest. Index is used everywhere as "rate index".
enum RollRate : int { rate24 = 0, rate32, rate48, rate64, rate96, numRollRates };

constexpr std::array<double, numRollRates> kRollSteps { 1.0 / 6.0, 1.0 / 8.0, 1.0 / 12.0, 1.0 / 16.0, 1.0 / 24.0 };
constexpr std::array<const char*, numRollRates> kRollRateNames { "1/24", "1/32", "1/48", "1/64", "1/96" };

/** Returns the rate index for a step length in beats, or -1 if it is not an allowed roll rate. */
int rateIndexForStep (double stepBeats) noexcept;

struct Note
{
    double beat = 0.0;      // position in quarter-note beats from pattern start
    double len  = 0.1;      // length in beats
    int vel     = 100;      // 1..127
    int pitch   = 0;        // semitone offset from the root note
    bool muted  = false;

    // Bookkeeping (not stored in preset files)
    int srcTick = -1;       // tick of the source note this came from (edit key), -1 = generated
    int rateOverride = -1;  // on the first note of a roll: forced rate index, -1 = none
};

struct Pattern
{
    std::vector<Note> notes;
    int bars = 1;

    double lengthBeats() const noexcept { return (double) bars * kBeatsPerBar; }
    void sort();
    /** Clamps velocities and fixes note lengths so that no note overlaps the next one. */
    void tidy();
};

/** A roll is a run of consecutive notes whose spacing is shorter than a 1/16. */
struct Roll
{
    int first = 0;          // index of first note (pattern must be sorted)
    int count = 0;          // number of notes in the roll
    double start = 0.0;     // beat of first note
    double step = 0.0;      // first interval
    double length = 0.0;    // sum of intervals + last step (= "how long the roll sounds")
    bool mixedRates = false;

    int last() const noexcept { return first + count - 1; }
    int bar() const noexcept { return (int) (start / kBeatsPerBar + kEps); }
};

std::vector<Roll> findRolls (const Pattern& sortedPattern);

/** Most common spacing between non-roll notes (0.5 for a 1/8 grid, 0.25 for 1/16, ...). */
double detectBaseGrid (const Pattern& sortedPattern);

/** Median velocity of the non-roll notes (used when re-filling the grid). */
int detectBaseVelocity (const Pattern& sortedPattern);

int beatToTick (double beat) noexcept;
bool isOnGrid (double beat, double grid) noexcept;

/** Removes a roll and fills its span with plain grid hits. */
void removeRoll (Pattern& p, const Roll& roll, double grid, int baseVel);

/** One uniform-rate stretch of a roll. */
struct RollSegment
{
    int rate = rate24;
    int count = 2;
};

struct RollShape
{
    std::vector<RollSegment> segments;
    std::vector<int> vels;     // per note (resampled if sizes differ)
    std::vector<int> pitches;  // per note (resampled if sizes differ)
};

/** Inserts a roll at 'start', removing whatever notes it would collide with. Returns the note count. */
int insertRoll (Pattern& p, double start, const RollShape& shape);

/** Extracts the shape of an existing roll. */
RollShape shapeOfRoll (const Pattern& p, const Roll& roll);

/** Duration of a roll shape in beats. */
double shapeLength (const RollShape& shape) noexcept;

/** Resamples a per-note curve to a new number of points (linear, rounded). */
std::vector<int> resampleCurve (const std::vector<int>& values, int newCount);

/** Tiles a preset of N bars to a target number of bars, keeping the fill at the end of the phrase. */
Pattern tileToBars (const Pattern& preset, int targetBars);

} // namespace rk
