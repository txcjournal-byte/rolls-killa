#pragma once

#include "Pattern.h"

#include <array>
#include <atomic>
#include <vector>

namespace rk
{

/** Immutable, audio-thread friendly version of a pattern. */
struct PlaybackPattern
{
    struct Event
    {
        double beat;
        double len;
        int note;
        int vel;
    };

    std::vector<Event> events;      // sorted by beat, muted notes removed
    double lengthBeats = 4.0;
    double bpmHint = 140.0;

    static PlaybackPattern fromPattern (const Pattern& p, int rootNote, double bpmHint);
};

struct TransportState
{
    bool playing = false;
    double ppq = 0.0;               // position of the first sample of the block, in quarter notes
    double bpm = 120.0;
};

/** A note event produced for one block. */
struct PlayerEvent
{
    int sampleOffset;
    int note;
    int vel;                        // 0 = note off
};

/**
    Turns a PlaybackPattern into sample-accurate note events, following the host transport:
    the pattern loops over its length, position jumps (loop, locate) and stops release pending notes.
    Real-time safe: fixed capacity, no allocation.
*/
class PatternPlayer
{
public:
    static constexpr int kMaxEventsPerBlock = 512;
    static constexpr int kMaxPendingOffs = 128;

    void prepare (double sampleRate) noexcept;
    void reset() noexcept;

    /** Fills 'out' with the events of this block (sorted by sample offset). Returns the event count. */
    int process (const PlaybackPattern* pattern, const TransportState& t, int numSamples,
                 std::array<PlayerEvent, kMaxEventsPerBlock>& out) noexcept;

    /** Position inside the pattern (0..length) for the UI, or -1 when stopped. */
    double getDisplayBeat() const noexcept { return displayBeat.load (std::memory_order_relaxed); }

private:
    struct PendingOff
    {
        double ppq;
        int note;
    };

    int flushOffs (int sampleOffset, std::array<PlayerEvent, kMaxEventsPerBlock>& out, int count) noexcept;
    void push (std::array<PlayerEvent, kMaxEventsPerBlock>& out, int& count, PlayerEvent e) noexcept;

    double sampleRate = 44100.0;
    bool wasPlaying = false;
    double expectedPpq = 0.0;
    std::array<PendingOff, kMaxPendingOffs> pending {};
    int numPending = 0;
    std::atomic<double> displayBeat { -1.0 };
};

} // namespace rk
