#include "PatternPlayer.h"

#include <algorithm>
#include <cmath>

namespace rk
{

PlaybackPattern PlaybackPattern::fromPattern (const Pattern& p, int rootNote, double bpmHint)
{
    PlaybackPattern out;
    out.lengthBeats = p.lengthBeats();
    out.bpmHint = bpmHint;

    for (const auto& n : p.notes)
        if (! n.muted && n.vel > 0)
            out.events.push_back ({ n.beat, n.len, std::clamp (rootNote + n.pitch, 0, 127), std::clamp (n.vel, 1, 127) });

    std::sort (out.events.begin(), out.events.end(), [] (const Event& a, const Event& b) { return a.beat < b.beat; });
    return out;
}

void PatternPlayer::prepare (double newSampleRate) noexcept
{
    sampleRate = newSampleRate;
    reset();
}

void PatternPlayer::reset() noexcept
{
    wasPlaying = false;
    numPending = 0;
    displayBeat.store (-1.0);
}

void PatternPlayer::push (std::array<PlayerEvent, kMaxEventsPerBlock>& out, int& count, PlayerEvent e) noexcept
{
    if (count < kMaxEventsPerBlock)
        out[(size_t) count++] = e;
}

int PatternPlayer::flushOffs (int sampleOffset, std::array<PlayerEvent, kMaxEventsPerBlock>& out, int count) noexcept
{
    for (int i = 0; i < numPending; ++i)
        push (out, count, { sampleOffset, pending[(size_t) i].note, 0, pending[(size_t) i].slot });
    numPending = 0;
    return count;
}

int PatternPlayer::process (const PlaybackPattern* pattern, const TransportState& t, int numSamples,
                            std::array<PlayerEvent, kMaxEventsPerBlock>& out) noexcept
{
    int count = 0;

    if (! t.playing || t.bpm <= 0.0 || numSamples <= 0)
    {
        if (wasPlaying)
            count = flushOffs (0, out, count);
        wasPlaying = false;
        displayBeat.store (-1.0, std::memory_order_relaxed);
        return count;
    }

    const auto beatsPerSample = t.bpm / 60.0 / sampleRate;
    const auto start = t.ppq;
    const auto end = start + numSamples * beatsPerSample;
    auto toOffset = [&] (double ppq) { return std::clamp ((int) std::floor ((ppq - start) / beatsPerSample + 1.0e-6), 0, numSamples - 1); };

    // Hosts do not always continue exactly where the previous block ended: after a tempo change
    // the position can be a little ahead or behind. Small differences are treated as continuous
    // playback - we carry on from where we stopped, so no note in the gap is dropped and nothing
    // is played twice. Only a real jump (loop, locate) restarts from the host position.
    const auto blockBeats = end - start;
    const auto continuityWindow = std::max (0.0625, 2.0 * blockBeats);
    const auto continuous = wasPlaying && std::abs (start - expectedPpq) <= continuityWindow;

    if (wasPlaying && ! continuous)
        count = flushOffs (0, out, count);

    const auto scanFrom = continuous ? expectedPpq : start;
    wasPlaying = true;
    expectedPpq = std::max (end, scanFrom);

    // Pending note-offs that fall into this block
    for (int i = 0; i < numPending;)
    {
        if (pending[(size_t) i].ppq < end)
        {
            push (out, count, { toOffset (pending[(size_t) i].ppq), pending[(size_t) i].note, 0, pending[(size_t) i].slot });
            pending[(size_t) i] = pending[(size_t) --numPending];
        }
        else
        {
            ++i;
        }
    }

    if (pattern != nullptr && pattern->lengthBeats > 0.0 && ! pattern->events.empty())
    {
        const auto len = pattern->lengthBeats;
        auto cycleStart = std::floor (scanFrom / len) * len;

        for (; cycleStart < end; cycleStart += len)
        {
            for (const auto& e : pattern->events)
            {
                const auto on = cycleStart + e.beat;
                if (on < scanFrom - 1.0e-9)
                    continue;
                if (on >= end)
                    break;

                const auto offset = toOffset (on);

                // Retrigger of a still-sounding note: close it first.
                for (int i = 0; i < numPending; ++i)
                    if (pending[(size_t) i].note == e.note && pending[(size_t) i].slot == e.slot)
                    {
                        push (out, count, { offset, e.note, 0, e.slot });
                        pending[(size_t) i] = pending[(size_t) --numPending];
                        break;
                    }

                push (out, count, { offset, e.note, e.vel, e.slot });

                const auto off = on + e.len;
                if (off < end)
                    push (out, count, { toOffset (off), e.note, 0, e.slot });
                else if (numPending < kMaxPendingOffs)
                    pending[(size_t) numPending++] = { off, e.note, e.slot };
                else
                    push (out, count, { numSamples - 1, e.note, 0, e.slot });
            }
        }

        auto phase = std::fmod (start, len);
        if (phase < 0.0)
            phase += len;
        displayBeat.store (phase, std::memory_order_relaxed);
    }

    // Insertion sort by offset (no allocation); at equal offsets note-offs go first so a retrigger is off -> on.
    auto before = [] (const PlayerEvent& a, const PlayerEvent& b)
    {
        if (a.sampleOffset != b.sampleOffset)
            return a.sampleOffset < b.sampleOffset;
        return a.vel == 0 && b.vel != 0;
    };

    for (int i = 1; i < count; ++i)
    {
        const auto e = out[(size_t) i];
        int j = i - 1;
        while (j >= 0 && before (e, out[(size_t) j]))
        {
            out[(size_t) j + 1] = out[(size_t) j];
            --j;
        }
        out[(size_t) j + 1] = e;
    }

    return count;
}

} // namespace rk
