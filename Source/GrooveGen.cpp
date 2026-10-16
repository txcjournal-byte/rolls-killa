#include "GrooveGen.h"

namespace GrooveGen
{
namespace
{
    using Hits = std::array<int, maxSlots>;

    struct Ctx
    {
        Groove& g;
        juce::Random& rng;
        float c;
        bool chance (float p) const { return rng.nextFloat() < p; }
        int pick (std::initializer_list<int> opts) const
        {
            const int i = rng.nextInt ((int) opts.size());
            return *(opts.begin() + i);
        }

        void beat (int b, int d, Hits h)
        {
            g.div[(size_t) b] = d;
            for (int s = 0; s < maxSlots; ++s) h[(size_t) s] = s < d ? juce::jlimit (0, maxHits, h[(size_t) s]) : 0;
            g.hits[(size_t) b] = h;
        }
        void sixteenths (int b, float fill)        // 1/8 backbone, 1/16 between with probability `fill`
        {
            beat (b, 4, { 1, chance (fill) ? 1 : 0, 1, chance (fill) ? 1 : 0, 0, 0 });
        }
        int& at (int b, int s) { return g.hits[(size_t) b][(size_t) s]; }
    };

    // classic roll shapes for the last beat of a bar, mild -> wild
    void rollEnding (Ctx& x, int b, float intensity)
    {
        const int r = juce::jlimit (0, 5, (int) std::floor (intensity * 6.0f + x.rng.nextFloat() * 1.5f));
        switch (r)
        {
            case 0:  x.beat (b, 4, { 1, 1, 1, 2, 0, 0 }); break;            // 1/32 pickup
            case 1:  x.beat (b, 4, { 1, 1, 2, 2, 0, 0 }); break;            // 1/32 last half
            case 2:  x.beat (b, 6, { 1, 1, 1, 1, 1, 1, }); break;           // 1/16T run
            case 3:  x.beat (b, 4, { 1, 2, 3, 4, 0, 0 }); break;            // accelerating ramp
            case 4:  x.beat (b, 4, { 1, 0, 4, 4, 0, 0 }); break;            // 1/64 burst after a gap
            default: x.beat (b, 6, { 1, 2, 2, 2, 3, 4 }); break;            // 1/32T into 1/64 flood
        }
    }

    void genBar (Ctx& x, int first, int style)
    {
        const float c = x.c;
        auto b = [first] (int i) { return first + i; };
        switch (style)
        {
            case steady:
                for (int i = 0; i < 4; ++i) x.sixteenths (b (i), 0.55f + 0.45f * c);
                if (x.chance (0.45f + 0.4f * c)) x.at (b (3), 3) = 2;
                if (c > 0.6f && x.chance (0.5f)) x.at (b (1), 3) = 2;
                break;

            case endRoll:
                for (int i = 0; i < 3; ++i) x.sixteenths (b (i), 0.75f + 0.25f * c);
                rollEnding (x, b (3), c);
                if (x.chance (0.25f + 0.4f * c)) x.at (b (1), 3) = x.pick ({ 2, 2, 3 });   // pickup into beat 3
                break;

            case triplet:
            {
                for (int i = 0; i < 4; ++i) x.sixteenths (b (i), 0.6f + 0.3f * c);
                const int runs = 1 + (int) std::round (c * 1.6f);
                for (int k = 0; k < runs; ++k)
                {
                    const int i = k == 0 ? x.pick ({ 1, 3, 3 }) : x.pick ({ 1, 2, 3 });
                    if (c > 0.55f && x.chance (0.5f)) x.beat (b (i), 6, { 1, 1, 2, 1, 2, 2 });    // 1/32T rolls inside
                    else if (x.chance (0.3f))         x.beat (b (i), 3, { 1, 1, 1, 0, 0, 0 });    // 1/8T
                    else                              x.beat (b (i), 6, { 1, 1, 1, 1, 1, 1 });    // 1/16T run
                }
                break;
            }

            case stutter:
                for (int i = 0; i < 4; ++i)
                {
                    x.beat (b (i), 4, { 1, 0, 1, 0, 0, 0 });
                    for (int s : { 1, 3 })
                        if (x.chance (0.35f + 0.4f * c)) x.at (b (i), s) = 2;
                    if (i > 0 && x.chance (0.12f + 0.2f * c)) x.at (b (i), 2) = 0;
                }
                x.at (b (x.pick ({ 1, 3 })), 3) = c > 0.5f ? 4 : 3;
                break;

            case bounce:
            {
                static const Hits shapes[] = { { 1, 0, 1, 1, 0, 0 }, { 1, 1, 0, 1, 0, 0 }, { 1, 0, 0, 1, 0, 0 },
                                               { 0, 1, 1, 0, 0, 0 }, { 1, 0, 1, 0, 0, 0 } };
                for (int i = 0; i < 4; ++i)
                {
                    auto h = shapes[x.rng.nextInt (i == 0 ? 3 : 5)];
                    if (i == 0) h[0] = 1;
                    x.beat (b (i), 4, h);
                    if (x.chance (0.2f + 0.45f * c))
                    {
                        const int s = x.rng.nextInt (4);
                        if (x.at (b (i), s) > 0) x.at (b (i), s) = x.pick ({ 2, 2, 3 });
                    }
                }
                break;
            }

            case sparse:
                for (int i = 0; i < 4; ++i)
                {
                    x.beat (b (i), 4, { 1, 0, x.chance (0.8f) ? 1 : 0, 0, 0, 0 });
                }
                x.at (b (x.pick ({ 1, 3 })), x.pick ({ 2, 3 })) = x.pick ({ 3, 4, 4, 6 });
                if (c > 0.5f) x.at (b (2), 3) = 2;
                break;

            case flood:
                for (int i = 0; i < 4; ++i)
                {
                    if (x.chance (0.25f + 0.25f * c)) { x.beat (b (i), 6, { 1, 2, 2, 2, 2, 3 }); continue; }
                    x.beat (b (i), 4, { 1, 2, x.pick ({ 2, 1 }), x.pick ({ 2, 3, 4 }), 0, 0 });
                }
                x.at (b (0), 0) = 1;
                break;

            case chaos:
            default:
                for (int i = 0; i < 4; ++i)
                {
                    const int d = x.pick ({ 4, 4, 3, 6 });
                    Hits h {};
                    for (int s = 0; s < d; ++s)
                        h[(size_t) s] = x.chance (0.15f + 0.15f * (1.0f - c)) ? 0 : 1 + (x.chance (0.35f + 0.4f * c) ? x.rng.nextInt (2 + (int) (c * 4.0f)) : 0);
                    x.beat (b (i), d, h);
                }
                x.at (b (0), 0) = juce::jmax (1, x.at (b (0), 0));
                break;
        }
    }
}

juce::String styleName (int style)
{
    static const char* names[] = { "MIX", "STEADY", "END ROLL", "TRIPLET", "STUTTER", "BOUNCE", "SPARSE", "FLOOD", "CHAOS" };
    return names[juce::jlimit (0, numStyles - 1, style)];
}

Groove generate (int style, float complexity, int bars, juce::Random& rng, int* styleUsed)
{
    Groove g;
    if (style <= mix || style >= numStyles) style = 1 + rng.nextInt (numStyles - 1);
    if (styleUsed != nullptr) *styleUsed = style;
    Ctx x { g, rng, juce::jlimit (0.0f, 1.0f, complexity) };
    bars = bars >= 4 ? 4 : (bars >= 2 ? 2 : 1);

    genBar (x, 0, style);
    for (int bar = 1; bar < bars; ++bar)
    {
        // later bars repeat bar 1 and change the ending: bars 2 and 4 get a bigger fill
        for (int i = 0; i < 4; ++i)
        {
            g.div[(size_t) (bar * 4 + i)] = g.div[(size_t) i];
            g.hits[(size_t) (bar * 4 + i)] = g.hits[(size_t) i];
        }
        const bool fill = bar == 1 || bar == 3;
        if (fill) rollEnding (x, bar * 4 + 3, juce::jmin (1.0f, x.c + (bar == 3 ? 0.35f : 0.15f)));
        else if (x.chance (0.6f))
        {
            Ctx y { g, rng, x.c * 0.8f };
            genBar (y, bar * 4, style);
        }
    }
    return g;
}

int numPresets() { return (numStyles - 1) * 6; }

juce::String presetName (int index)
{
    index = juce::jlimit (0, numPresets() - 1, index);
    return styleName (1 + index / 6) + " " + juce::String (index % 6 + 1).paddedLeft ('0', 2);
}

Groove preset (int index, int bars)
{
    index = juce::jlimit (0, numPresets() - 1, index);
    const int style = 1 + index / 6, n = index % 6;
    juce::Random rng ((juce::int64) (7001 + style * 131 + n * 17));
    return generate (style, 0.15f + 0.14f * (float) n, bars, rng);
}

juce::String toString (const Groove& g)
{
    juce::String s;
    for (int b = 0; b < maxBeats; ++b)
    {
        s << g.div[(size_t) b];
        for (int k = 0; k < maxSlots; ++k) s << g.hits[(size_t) b][(size_t) k];
    }
    return s;
}

bool fromString (const juce::String& s, Groove& g)
{
    if (s.length() != maxBeats * (1 + maxSlots)) return false;
    Groove out;
    int p = 0;
    for (int b = 0; b < maxBeats; ++b)
    {
        const int d = s[p++] - '0';
        if (d != 3 && d != 4 && d != 6) return false;
        out.div[(size_t) b] = d;
        for (int k = 0; k < maxSlots; ++k)
        {
            const int v = s[p++] - '0';
            if (v < 0 || v > maxHits) return false;
            out.hits[(size_t) b][(size_t) k] = k < d ? v : 0;
        }
    }
    g = out;
    return true;
}
}
