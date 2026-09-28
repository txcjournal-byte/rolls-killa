#include "engine/PatternPlayer.h"

#include <juce_core/juce_core.h>

#include <map>

namespace rk
{

class PlayerTests : public juce::UnitTest
{
public:
    PlayerTests() : juce::UnitTest ("Pattern player / host sync", "RollsKilla") {}

    struct Hit
    {
        juce::int64 sample;
        int note, vel;
    };

    static PlaybackPattern makePattern()
    {
        Pattern p;
        p.bars = 1;
        for (auto beat : { 0.0, 1.0, 2.5, 3.0, 3.125, 3.25 })
        {
            Note n;
            n.beat = beat;
            n.len = 0.06;
            n.vel = 100;
            p.notes.push_back (n);
        }
        p.notes[4].pitch = 5;
        return PlaybackPattern::fromPattern (p, 60, 140.0);
    }

    /** Runs the player like a host would and returns all events with absolute sample positions. */
    std::vector<Hit> run (PatternPlayer& player, const PlaybackPattern& pat, double bpm, double startPpq,
                          juce::int64 numSamples, int blockSize, double sr, juce::int64 sampleBase = 0)
    {
        std::vector<Hit> hits;
        std::array<PlayerEvent, PatternPlayer::kMaxEventsPerBlock> ev;
        for (juce::int64 pos = 0; pos < numSamples; pos += blockSize)
        {
            TransportState t;
            t.playing = true;
            t.bpm = bpm;
            t.ppq = startPpq + (double) pos * bpm / 60.0 / sr;
            const auto n = (int) std::min<juce::int64> (blockSize, numSamples - pos);
            const auto count = player.process (&pat, t, n, ev);
            for (int i = 0; i < count; ++i)
                hits.push_back ({ sampleBase + pos + ev[(size_t) i].sampleOffset, ev[(size_t) i].note, ev[(size_t) i].vel });
        }
        return hits;
    }

    void runTest() override
    {
        const double sr = 48000.0;
        const auto pat = makePattern();

        beginTest ("events land on the right samples and loop");
        {
            PatternPlayer player;
            player.prepare (sr);
            // 120 BPM -> 24000 samples per beat, pattern = 4 beats = 96000 samples; play 2 loops
            const auto hits = run (player, pat, 120.0, 0.0, 192000, 512, sr);
            std::vector<juce::int64> ons;
            for (const auto& h : hits)
                if (h.vel > 0)
                    ons.push_back (h.sample);

            const std::vector<juce::int64> expected { 0, 24000, 60000, 72000, 75000, 78000,
                                                      96000, 120000, 156000, 168000, 171000, 174000 };
            expectEquals ((int) ons.size(), (int) expected.size());
            for (size_t i = 0; i < std::min (ons.size(), expected.size()); ++i)
                expect (std::abs (ons[i] - expected[i]) <= 1, "on " + juce::String ((int) i) + " at " + juce::String (ons[i]));

            const auto roll = std::find_if (hits.begin(), hits.end(), [] (const Hit& h) { return h.note == 65; });
            expect (roll != hits.end(), "pitched note becomes root + pitch");
        }

        beginTest ("block size does not change timing");
        {
            PatternPlayer a, b;
            a.prepare (sr);
            b.prepare (sr);
            const auto h1 = run (a, pat, 143.0, 0.0, 100000, 64, sr);
            const auto h2 = run (b, pat, 143.0, 0.0, 100000, 1024, sr);
            expectEquals ((int) h1.size(), (int) h2.size());
            for (size_t i = 0; i < std::min (h1.size(), h2.size()); ++i)
                expect (std::abs (h1[i].sample - h2[i].sample) <= 1);
        }

        beginTest ("every note-on gets a note-off (stop, loop jump)");
        {
            PatternPlayer player;
            player.prepare (sr);
            std::map<int, int> open;
            auto track = [&] (const std::vector<Hit>& hits)
            {
                for (const auto& h : hits)
                    open[h.note] += h.vel > 0 ? 1 : -1;
            };

            // play into the roll, then the host loops back to bar start mid-note
            track (run (player, pat, 120.0, 2.9, 7300, 256, sr));
            track (run (player, pat, 120.0, 0.0, 4000, 256, sr));

            // stop
            std::array<PlayerEvent, PatternPlayer::kMaxEventsPerBlock> ev;
            TransportState stopped;
            const auto count = player.process (&pat, stopped, 256, ev);
            std::vector<Hit> offs;
            for (int i = 0; i < count; ++i)
                offs.push_back ({ ev[(size_t) i].sampleOffset, ev[(size_t) i].note, ev[(size_t) i].vel });
            track (offs);

            for (const auto& [note, balance] : open)
                expectEquals (balance, 0, "note " + juce::String (note));
            expectEquals (player.getDisplayBeat(), -1.0);
        }

        beginTest ("starting in the middle of the song keeps the phase");
        {
            PatternPlayer player;
            player.prepare (sr);
            // ppq 41.0 = beat 1 of the 11th loop -> first hit at 1.0 inside the pattern (sample 0)
            const auto hits = run (player, pat, 120.0, 41.0, 30000, 512, sr);
            expect (! hits.empty() && hits.front().sample == 0 && hits.front().vel > 0);
            expectWithinAbsoluteError (player.getDisplayBeat(), 1.0 + 29696.0 / 24000.0, 0.01);
        }

        beginTest ("tempo changes with host position jitter: every note exactly once");
        {
            // Hosts recompute the position when the tempo changes; the next block can start a hair
            // before or after where the previous one ended. Notes on the block border must not be
            // lost or doubled.
            PatternPlayer player;
            player.prepare (sr);
            std::array<PlayerEvent, PatternPlayer::kMaxEventsPerBlock> ev;
            juce::Random rng (7);
            double ppq = 0.0;
            int ons = 0;
            const int block = 480;
            const double bpms[] { 140.0, 110.0, 90.0, 75.0, 140.0 };
            double hostBpm = bpms[0];
            for (int b = 0; b < 2000; ++b)
            {
                // the host reports the new tempo right away, but still advances this block at the
                // old one (tempo change inside the block) - plus a little rounding jitter
                const auto reportedBpm = bpms[(b / 30) % 5];
                const auto jitter = (rng.nextDouble() - 0.5) * 2.0e-3;
                TransportState t { true, std::max (0.0, ppq + jitter), reportedBpm };
                const auto count = player.process (&pat, t, block, ev);
                for (int i = 0; i < count; ++i)
                    ons += ev[(size_t) i].vel > 0 ? 1 : 0;
                ppq += block * hostBpm / 60.0 / sr;
                hostBpm = reportedBpm;
            }
            // expected: every pattern event whose position lies in [0, ppq_end)
            int expected = 0;
            for (double loop = 0.0; loop < ppq; loop += pat.lengthBeats)
                for (const auto& e : pat.events)
                    expected += loop + e.beat < ppq - 0.01 ? 1 : 0;
            expectWithinAbsoluteError (ons, expected, 1);
        }

        beginTest ("note in the small gap after a tempo change is not dropped (or doubled)");
        {
            PatternPlayer player;
            player.prepare (sr);
            std::array<PlayerEvent, PatternPlayer::kMaxEventsPerBlock> ev;
            auto onsAt = [&] (double ppq, double bpm, int samples)
            {
                TransportState t { true, ppq, bpm };
                const auto count = player.process (&pat, t, samples, ev);
                int ons = 0;
                for (int i = 0; i < count; ++i)
                    ons += (ev[(size_t) i].vel > 0 && ev[(size_t) i].note == 60) ? 1 : 0;
                return ons;
            };
            // block ends at 0.99 (by the tempo the host reported) ...
            const auto samplesTo099 = (int) std::lround (0.09 / (120.0 / 60.0 / sr));
            int total = onsAt (0.9, 120.0, samplesTo099);
            // ... but the host actually moved on to 1.01: the note at 1.0 lies in the gap
            total += onsAt (1.01, 90.0, 480);
            expectEquals (total, 1, "note at beat 1.0 plays exactly once");

            // and the other way round: host steps back a little -> no double note
            PatternPlayer p2;
            p2.prepare (sr);
            TransportState a { true, 0.95, 120.0 };
            int n = 0;
            auto c = p2.process (&pat, a, (int) std::lround (0.07 / (120.0 / 60.0 / sr)), ev);   // plays 1.0
            for (int i = 0; i < c; ++i) n += ev[(size_t) i].vel > 0 ? 1 : 0;
            TransportState b { true, 0.995, 90.0 };                                              // back to 0.995
            c = p2.process (&pat, b, 480, ev);
            for (int i = 0; i < c; ++i) n += ev[(size_t) i].vel > 0 ? 1 : 0;
            expectEquals (n, 1, "no double hit when the host steps back slightly");
        }

        beginTest ("tempo change follows the host position");
        {
            PatternPlayer player;
            player.prepare (sr);
            std::array<PlayerEvent, PatternPlayer::kMaxEventsPerBlock> ev;
            double ppq = 0.0;
            juce::int64 samplePos = 0;
            std::vector<juce::int64> ons;
            for (int block = 0; block < 400; ++block)
            {
                const auto bpm = block < 200 ? 120.0 : 160.0;
                TransportState t { true, ppq, bpm };
                const auto count = player.process (&pat, t, 480, ev);
                for (int i = 0; i < count; ++i)
                    if (ev[(size_t) i].vel > 0)
                        ons.push_back (samplePos + ev[(size_t) i].sampleOffset);
                ppq += 480 * bpm / 60.0 / sr;
                samplePos += 480;
            }
            // tempo switches at sample 96000 = ppq 4.0 (start of 2nd loop); the 2nd beat of loop 2
            // is then 1 beat at 160 BPM later = 18000 samples
            expect (std::find (ons.begin(), ons.end(), (juce::int64) 96000) != ons.end());
            expect (std::find (ons.begin(), ons.end(), (juce::int64) 114000) != ons.end());
        }
    }
};

static PlayerTests playerTests;

} // namespace rk
