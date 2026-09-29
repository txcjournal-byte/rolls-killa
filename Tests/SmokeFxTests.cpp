#include "engine/SmokeFx.h"

#include <juce_core/juce_core.h>

#include <cmath>
#include <vector>

namespace rk
{

namespace
{
    constexpr double kSmokeSr = 44100.0;
    constexpr int kSmokeLen = 44100 * 2;
}

class SmokeFxTests : public juce::UnitTest
{
public:
    SmokeFxTests() : juce::UnitTest ("PUFF smoke FX", "RollsKilla") {}

    void runTest() override
    {
        const auto sr = kSmokeSr;
        const auto n = kSmokeLen;
        const auto beatsPerSample = 140.0 / 60.0 / sr;

        auto hats = []
        {
            std::vector<float> v (kSmokeLen, 0.0f);
            for (int i = 0; i < kSmokeLen; i += 4725)          // 1/8 at 140 BPM
                for (int k = 0; k < 2000 && i + k < kSmokeLen; ++k)
                    v[(size_t) (i + k)] = 0.8f * std::exp (-k / 300.0f) * std::sin (k * 1.7f);
            return v;
        };

        beginTest ("PUFF 0 leaves the audio untouched");
        {
            SmokeFx fx;
            fx.prepare (sr);
            auto l = hats(), r = hats();
            const auto orig = hats();
            fx.process (l.data(), r.data(), n, 0.0f, 0.0, beatsPerSample, true);
            expect (l == orig && r == orig);
        }

        beginTest ("PUFF max: crackle and breath while playing, bounded output");
        {
            SmokeFx fx;
            fx.prepare (sr);
            std::vector<float> l (n, 0.0f), r (n, 0.0f);
            fx.process (l.data(), r.data(), n, 1.0f, 0.0, beatsPerSample, true);
            float peak = 0.0f;
            bool finite = true;
            for (auto s : l) { peak = std::max (peak, std::abs (s)); finite = finite && std::isfinite (s); }
            expect (finite);
            expect (peak > 0.005f, "smoke adds crackle on silence while playing");
            expect (peak < 0.5f);

            auto h = hats();
            auto h2 = hats();
            SmokeFx fx2;
            fx2.prepare (sr);
            fx2.process (h.data(), h2.data(), n, 1.0f, 0.0, beatsPerSample, true);
            float hp = 0.0f;
            for (auto s : h) { hp = std::max (hp, std::abs (s)); finite = finite && std::isfinite (s); }
            expect (finite && hp < 1.2f, "saturated hats stay below clipping");
        }

        beginTest ("no crackle while the pattern is stopped");
        {
            SmokeFx fx;
            fx.prepare (sr);
            std::vector<float> l (n, 0.0f);
            fx.process (l.data(), nullptr, n, 1.0f, 0.0, beatsPerSample, false);
            float peak = 0.0f;
            for (auto s : l) peak = std::max (peak, std::abs (s));
            expect (peak < 1.0e-6f);
        }

        beginTest ("inhale curve follows the beat");
        {
            expectWithinAbsoluteError (SmokeFx::inhaleCurve (0.0), 0.0f, 1.0e-6f);
            expect (SmokeFx::inhaleCurve (0.7) > 0.95f);
            expect (SmokeFx::inhaleCurve (0.99) < 0.05f);
            expectWithinAbsoluteError (SmokeFx::inhaleCurve (1.35), SmokeFx::inhaleCurve (0.35), 1.0e-6f);
        }
    }
};

static SmokeFxTests smokeFxTests;

} // namespace rk
