#include "engine/DrumPatterns.h"

namespace rk
{

class DrumPatternTests : public juce::UnitTest
{
public:
    DrumPatternTests() : juce::UnitTest ("Drum patterns", "RollsKilla") {}

    void runTest() override
    {
        beginTest ("every drawer: hits inside the pattern, sorted, sane velocity and pitch");
        for (int t = 0; t < kNumDrumTypes; ++t)
        {
            const auto type = drumTypeFromIndex (t);
            if (type == DrumType::hat)
                continue;
            for (int bars : { 1, 2, 4 })
                for (int style = 0; style < 4; ++style)
                    for (float density : { 0.0f, 0.5f, 1.0f })
                        for (juce::uint32 seed = 1; seed < 6; ++seed)
                        {
                            const auto p = generateDrumPattern (type, seed, style, bars, density);
                            bool ok = true;
                            for (size_t i = 0; i < p.size(); ++i)
                            {
                                ok = ok && p[i].beat >= 0.0 && p[i].beat < bars * 4.0 && p[i].beat + p[i].len <= bars * 4.0 + 1.0e-6;
                                ok = ok && p[i].vel > 0.0f && p[i].vel <= 1.0f && std::abs (p[i].semi) <= 12;
                                ok = ok && (i == 0 || p[i - 1].beat <= p[i].beat);
                            }
                            expect (ok, juce::String (drumTypeName (type)) + " style " + juce::String (style));
                            if (type == DrumType::b808 || type == DrumType::kick || type == DrumType::snare || type == DrumType::clap)
                                expect (! p.empty(), juce::String (drumTypeName (type)) + " never empty");
                        }
        }

        beginTest ("deterministic per seed, a new seed = a new pattern");
        {
            const auto a = generateDrumPattern (DrumType::b808, 42, 1, 2, 0.6f);
            const auto b = generateDrumPattern (DrumType::b808, 42, 1, 2, 0.6f);
            expect (a.size() == b.size());
            bool same = a.size() == b.size();
            for (size_t i = 0; same && i < a.size(); ++i)
                same = juce::exactlyEqual (a[i].beat, b[i].beat) && a[i].semi == b[i].semi;
            expect (same);
            int differs = 0;
            for (juce::uint32 s = 43; s < 53; ++s)
            {
                const auto c = generateDrumPattern (DrumType::b808, s, 1, 2, 0.6f);
                differs += c.size() != a.size() ? 1 : 0;
                for (size_t i = 0; c.size() == a.size() && i < a.size(); ++i)
                    if (! juce::exactlyEqual (c[i].beat, a[i].beat) || c[i].semi != a[i].semi) { ++differs; break; }
            }
            expect (differs >= 8, "new seeds give new 808 lines");
        }

        beginTest ("trap backbeat: snares and claps hit the 3");
        for (juce::uint32 seed = 1; seed < 20; ++seed)
            for (auto type : { DrumType::snare, DrumType::clap })
            {
                const auto p = generateDrumPattern (type, seed, 0, 2, 0.5f);
                int backbeats = 0;
                for (const auto& h : p)
                    backbeats += (std::abs (std::fmod (h.beat, 4.0) - 2.0) < 1.0e-6 && h.vel > 0.9f) ? 1 : 0;
                expectEquals (backbeats, 2);
            }
    }
};

static DrumPatternTests drumPatternTests;

} // namespace rk
