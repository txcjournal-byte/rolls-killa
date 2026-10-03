#include "DrumSynth.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <array>
#include <cmath>

namespace rk
{

namespace
{
    constexpr double kTwoPi = juce::MathConstants<double>::twoPi;

    //==============================================================================
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

        void set (double nb0, double nb1, double nb2, double a0, double na1, double na2)
        {
            b0 = nb0 / a0; b1 = nb1 / a0; b2 = nb2 / a0; a1 = na1 / a0; a2 = na2 / a0;
        }

        void highPass (double fc, double q, double fs)
        {
            const auto w = kTwoPi * juce::jlimit (10.0, fs * 0.45, fc) / fs, alpha = std::sin (w) / (2.0 * q), c = std::cos (w);
            set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + alpha, -2 * c, 1 - alpha);
        }

        void lowPass (double fc, double q, double fs)
        {
            const auto w = kTwoPi * juce::jlimit (10.0, fs * 0.45, fc) / fs, alpha = std::sin (w) / (2.0 * q), c = std::cos (w);
            set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + alpha, -2 * c, 1 - alpha);
        }

        void bandPass (double fc, double q, double fs)
        {
            const auto w = kTwoPi * juce::jlimit (10.0, fs * 0.45, fc) / fs, alpha = std::sin (w) / (2.0 * q), c = std::cos (w);
            set (alpha, 0, -alpha, 1 + alpha, -2 * c, 1 - alpha);
        }

        double process (double x)
        {
            const auto y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
    };

    /** Seeded dice for one sound. */
    struct Dice
    {
        explicit Dice (juce::int64 seed) : r (seed) {}
        float uni() { return r.nextFloat(); }
        float range (float a, float b) { return a + (b - a) * r.nextFloat(); }
        int pick (int n) { return r.nextInt (juce::jmax (1, n)); }
        bool chance (float p) { return r.nextFloat() < p; }
        float noise() { return r.nextFloat() * 2.0f - 1.0f; }
        juce::Random r;
    };

    double midiToHz (double note) { return 440.0 * std::pow (2.0, (note - 69.0) / 12.0); }
    int hzToMidi (double hz) { return (int) std::lround (69.0 + 12.0 * std::log2 (juce::jmax (1.0, hz) / 440.0)); }
    float decayScale (const DrumShape& s) { return std::pow (2.0f, (s.decay - 0.5f) * 2.0f); }   // 0.5 .. 2
    float tuneRatio (const DrumShape& s) { return std::pow (2.0f, s.tune / 12.0f); }

    juce::AudioBuffer<float> makeBuffer (double seconds, double fs)
    {
        juce::AudioBuffer<float> b (1, juce::jmax (16, (int) std::ceil (seconds * fs)));
        b.clear();
        return b;
    }

    /** Tiny Schroeder room for claps, snares and FX tails (offline). */
    void addRoom (juce::AudioBuffer<float>& b, double fs, float wet, float seconds)
    {
        if (wet <= 0.0f)
            return;
        const auto n = b.getNumSamples();
        const auto tail = (int) (seconds * fs);
        juce::AudioBuffer<float> out (1, n + tail);
        out.clear();
        out.copyFrom (0, 0, b, 0, 0, n);
        const int combMs[] { 29, 37, 41, 44 };
        std::vector<float> wetSig ((size_t) (n + tail), 0.0f);
        for (auto ms : combMs)
        {
            const auto d = juce::jmax (1, (int) (ms * 0.001 * fs));
            const auto g = std::pow (0.001, (double) d / (seconds * fs));
            std::vector<float> line ((size_t) d, 0.0f);
            int p = 0;
            float lp = 0.0f;
            for (int i = 0; i < n + tail; ++i)
            {
                const auto x = i < n ? b.getSample (0, i) : 0.0f;
                const auto y = line[(size_t) p];
                lp += (y - lp) * 0.45f;                       // damping
                line[(size_t) p] = x + lp * (float) g;
                p = (p + 1) % d;
                wetSig[(size_t) i] += y * 0.25f;
            }
        }
        for (int ms : { 5, 2 })                              // diffusion
        {
            const auto d = juce::jmax (1, (int) (ms * 0.001 * fs));
            std::vector<float> line ((size_t) d, 0.0f);
            int p = 0;
            for (auto& s : wetSig)
            {
                const auto buf = line[(size_t) p];
                const auto y = -0.6f * s + buf;
                line[(size_t) p] = s + 0.6f * y;
                p = (p + 1) % d;
                s = y;
            }
        }
        for (int i = 0; i < n + tail; ++i)
            out.addSample (0, i, wetSig[(size_t) i] * wet);
        b = std::move (out);
    }

    /** Knobs + mastering: punch, drive, tone, DC, trim, -0.5 dBFS, tiny fade. */
    void finish (juce::AudioBuffer<float>& b, double fs, const DrumShape& shape, float baseDrive)
    {
        const auto n = b.getNumSamples();
        const auto chans = b.getNumChannels();
        const auto punchAmt = (shape.punch - 0.5f) * 1.6f;            // -0.8 .. +0.8
        const auto punchLen = (int) (0.018 * fs);
        const auto drive = 1.0f + 9.0f * juce::jlimit (0.0f, 1.5f, baseDrive + shape.drive);
        const auto norm = std::tanh (drive);
        const auto tilt = (shape.tone - 0.5f) * 2.0f;                 // -1 .. 1
        const auto lpK = (float) (1.0 - std::exp (-kTwoPi * 1800.0 / fs));
        const auto dcK = (float) (1.0 - std::exp (-kTwoPi * 18.0 / fs));

        for (int ch = 0; ch < chans; ++ch)
        {
            auto* d = b.getWritePointer (ch);
            float lp = 0.0f, dc = 0.0f;
            for (int i = 0; i < n; ++i)
            {
                auto x = d[i];
                if (i < punchLen)
                    x *= 1.0f + punchAmt * (1.0f - (float) i / (float) punchLen);
                x = std::tanh (x * drive) / norm;
                lp += (x - lp) * lpK;
                x += tilt * (tilt > 0.0f ? 1.2f : 0.85f) * (x - lp);
                dc += (x - dc) * dcK;
                d[i] = x - dc * (i > (int) (0.004 * fs) ? 1.0f : (float) i / (float) (0.004 * fs));
            }
        }

        // trim the silent end (-70 dB), keep at least 20 ms
        const auto peak = b.getMagnitude (0, n);
        if (peak <= 0.0f)
            return;
        const auto floor = peak * 0.000316f;
        int last = n - 1;
        for (; last > (int) (0.02 * fs); --last)
        {
            bool loud = false;
            for (int ch = 0; ch < chans; ++ch)
                loud = loud || std::abs (b.getSample (ch, last)) > floor;
            if (loud)
                break;
        }
        b.setSize (chans, juce::jmin (n, last + (int) (0.005 * fs)), true, true, false);

        b.applyGain (0.944f / juce::jmax (1.0e-6f, b.getMagnitude (0, b.getNumSamples())));
        const auto fade = juce::jmin (b.getNumSamples() / 4, (int) (0.003 * fs));
        b.applyGainRamp (b.getNumSamples() - fade, fade, 1.0f, 0.0f);
    }

    float moodDrive (Dice& d, int mood, float clean, float dirty)
    {
        const auto pDirty = mood == 0 ? 0.12f : mood == 2 ? 0.75f : 0.35f;
        return d.chance (pDirty) ? dirty * d.range (0.6f, 1.0f) : clean * d.range (0.4f, 1.0f);
    }

    //==============================================================================
    void render808 (Dice& d, juce::AudioBuffer<float>& out, double fs, int mood, const DrumShape& s, int& root, float& drive)
    {
        const auto note = 25 + d.pick (7);                          // C#0 .. G0 (35-49 Hz)
        const auto f0 = midiToHz (note) * tuneRatio (s);
        root = juce::jlimit (0, 127, note + (int) std::lround (s.tune));
        const auto sweep = std::pow (2.0, d.range (mood == 2 ? 16.0f : 7.0f, mood == 2 ? 30.0f : 19.0f) / 12.0);
        const auto sweepTau = d.range (0.012f, 0.04f);
        const auto len = juce::jlimit (0.35f, 4.0f, d.range (0.9f, 2.3f) * decayScale (s));
        const auto curve = d.range (1.3f, 2.6f);
        const auto h2 = d.range (0.0f, 0.18f) + (mood == 2 ? 0.1f : 0.0f);
        const auto clickAmt = d.range (0.15f, 0.5f) * s.punch * 2.0f;
        drive = moodDrive (d, mood, 0.12f, 0.75f);

        out = makeBuffer (len + 0.02, fs);
        auto* o = out.getWritePointer (0);
        double phase = 0.0;
        Biquad click;
        click.highPass (2500.0, 0.7, fs);
        for (int i = 0; i < out.getNumSamples(); ++i)
        {
            const auto t = i / fs;
            const auto f = f0 * (1.0 + (sweep - 1.0) * std::exp (-t / sweepTau));
            phase += kTwoPi * f / fs;
            const auto x = juce::jlimit (0.0, 1.0, t / len);
            const auto amp = std::pow (1.0 - x, curve) * juce::jmin (1.0, t / 0.002);
            auto y = (std::sin (phase) + h2 * std::sin (2.0 * phase)) * amp;
            if (t < 0.006)
                y += click.process (d.noise()) * clickAmt * (1.0 - t / 0.006);
            o[i] = (float) y;
        }
    }

    void renderKick (Dice& d, juce::AudioBuffer<float>& out, double fs, int mood, const DrumShape& s, int& root, float& drive)
    {
        const auto f0 = d.range (44.0f, 62.0f) * tuneRatio (s);
        root = juce::jlimit (0, 127, hzToMidi (f0));
        const auto fStart = f0 * std::pow (2.0, d.range (1.6f, mood == 2 ? 3.2f : 2.6f));
        const auto tau = d.range (0.012f, 0.032f);
        const auto len = juce::jlimit (0.12f, 1.4f, d.range (0.24f, 0.55f) * decayScale (s));
        const auto clickAmt = d.range (0.25f, 0.7f) * s.punch * 2.0f;
        drive = moodDrive (d, mood, 0.25f, 0.8f);

        out = makeBuffer (len + 0.02, fs);
        auto* o = out.getWritePointer (0);
        Biquad click;
        click.bandPass (d.range (2800.0f, 5200.0f), 0.9, fs);
        double phase = 0.0;
        for (int i = 0; i < out.getNumSamples(); ++i)
        {
            const auto t = i / fs;
            const auto f = f0 + (fStart - f0) * std::exp (-t / tau);
            phase += kTwoPi * f / fs;
            const auto amp = std::exp (-t / (len * 0.28)) * juce::jmin (1.0, t / 0.0008);
            auto y = std::sin (phase) * amp;
            if (t < 0.005)
                y += click.process (d.noise()) * clickAmt * 2.0 * (1.0 - t / 0.005);
            o[i] = (float) y;
        }
    }

    void renderSnare (Dice& d, juce::AudioBuffer<float>& out, double fs, int mood, const DrumShape& s, float& drive)
    {
        const auto f1 = d.range (165.0f, 245.0f) * tuneRatio (s);
        const auto f2 = f1 * d.range (1.45f, 1.75f);
        const auto toneDecay = d.range (0.05f, 0.13f) * decayScale (s);
        const auto noiseDecay = d.range (0.1f, 0.28f) * decayScale (s);
        const auto len = juce::jmax (toneDecay, noiseDecay) * 4.0f + 0.02f;
        const auto toneGain = 0.35f + 0.9f * s.body, noiseGain = 1.25f - 0.75f * s.body;
        drive = moodDrive (d, mood, 0.25f, 0.7f);

        out = makeBuffer (len, fs);
        auto* o = out.getWritePointer (0);
        Biquad hp, lp;
        hp.highPass (d.range (700.0f, 1800.0f) * (0.7f + 0.6f * s.tone), 0.7, fs);
        lp.lowPass (d.range (6000.0f, 11000.0f) * (0.6f + 0.8f * s.tone), 0.7, fs);
        double p1 = 0.0, p2 = 0.0;
        for (int i = 0; i < out.getNumSamples(); ++i)
        {
            const auto t = i / fs;
            const auto drop = 1.0 + 0.25 * std::exp (-t / 0.012);
            p1 += kTwoPi * f1 * drop / fs;
            p2 += kTwoPi * f2 * drop / fs;
            const auto tone = (std::sin (p1) + 0.6 * std::sin (p2)) * std::exp (-t / toneDecay) * toneGain;
            const auto nz = lp.process (hp.process (d.noise())) * std::exp (-t / noiseDecay) * noiseGain * 1.6;
            o[i] = (float) ((tone + nz) * juce::jmin (1.0, t / 0.0006));
        }
        if (d.chance (mood == 0 ? 0.5f : 0.3f))
            addRoom (out, fs, d.range (0.08f, 0.22f), d.range (0.25f, 0.6f));
    }

    void renderClap (Dice& d, juce::AudioBuffer<float>& out, double fs, int mood, const DrumShape& s, float& drive)
    {
        const auto bursts = 3 + d.pick (3);
        const auto spacing = d.range (0.007f, 0.013f);
        const auto tail = d.range (0.12f, 0.3f) * decayScale (s);
        const auto tailStart = spacing * (float) (bursts - 1);
        const auto len = tailStart + tail * 5.0f;
        drive = moodDrive (d, mood, 0.2f, 0.65f);

        out = makeBuffer (len, fs);
        auto* o = out.getWritePointer (0);
        Biquad bp, hp, lp;
        const auto centre = d.range (1000.0f, 2100.0f) * (0.75f + 0.5f * s.tone) * (1.15f - 0.3f * s.body);
        bp.bandPass (centre, d.range (0.9f, 1.6f), fs);
        hp.highPass (450.0, 0.7, fs);
        lp.lowPass (centre * 3.5, 0.6, fs);
        std::array<float, 6> jitter {};
        for (auto& j : jitter)
            j = d.range (-0.0015f, 0.0015f);
        for (int i = 0; i < out.getNumSamples(); ++i)
        {
            const auto t = (float) (i / fs);
            float env = 0.0f;
            for (int k = 0; k < bursts; ++k)
            {
                const auto t0 = spacing * (float) k + (k > 0 ? jitter[(size_t) k] : 0.0f);
                if (t >= t0)
                    env = juce::jmax (env, std::exp (-(t - t0) / (k == bursts - 1 ? tail : 0.0045f)) * (k == bursts - 1 ? 1.0f : 0.85f));
            }
            const auto x = d.noise();
            o[i] = (float) (lp.process (hp.process (bp.process (x) * 2.5 + x * 0.15 * (1.0f - s.body))) * env);
        }
        if (d.chance (mood == 2 ? 0.4f : 0.6f))
            addRoom (out, fs, d.range (0.12f, 0.32f), d.range (0.3f, 0.8f));
    }

    void renderHat (Dice& d, juce::AudioBuffer<float>& out, double fs, int mood, const DrumShape& s, bool open, float& drive)
    {
        static constexpr double ratios[] { 1.0, 1.483, 1.8, 2.546, 2.63, 3.897 };
        const auto base = d.range (190.0f, 340.0f) * tuneRatio (s);
        const auto decay = (open ? d.range (0.22f, 0.55f) : d.range (0.028f, 0.095f)) * decayScale (s);
        const auto len = decay * (open ? 3.2f : 4.0f) + 0.01f;
        const auto metal = 0.25f + 0.7f * s.body;   // body = more metal, less noise
        drive = moodDrive (d, mood, 0.08f, 0.4f);

        out = makeBuffer (len, fs);
        auto* o = out.getWritePointer (0);
        Biquad hp, bp;
        hp.highPass (d.range (5800.0f, 7800.0f) * (0.8f + 0.4f * s.tone), 0.75, fs);
        bp.bandPass (d.range (8000.0f, 10500.0f) * (0.85f + 0.3f * s.tone), 0.8, fs);
        std::array<double, 6> ph {};
        for (auto& p : ph)
            p = d.uni();
        for (int i = 0; i < out.getNumSamples(); ++i)
        {
            const auto t = i / fs;
            double sq = 0.0;
            for (size_t k = 0; k < 6; ++k)
            {
                ph[k] += base * ratios[k] / fs;
                ph[k] -= std::floor (ph[k]);
                sq += ph[k] < 0.5 ? 1.0 : -1.0;
            }
            const auto x = sq / 6.0 * metal + d.noise() * (1.0 - metal);
            const auto filtered = hp.process (x) * 0.6 + bp.process (x) * 1.4;
            const auto env = std::exp (-t / decay) * juce::jmin (1.0, t / (open ? 0.002 : 0.0006));
            o[i] = (float) (filtered * env);
        }
    }

    juce::String renderPerc (Dice& d, juce::AudioBuffer<float>& out, double fs, int mood, const DrumShape& s, int& root, float& drive)
    {
        const auto kind = d.pick (5);
        const auto ds = decayScale (s), tr = tuneRatio (s);
        drive = moodDrive (d, mood, 0.15f, 0.5f);
        Biquad f1, f2;
        double p1 = 0.0, p2 = 0.0;

        auto build = [&] (float seconds, auto&& sampleAt)
        {
            out = makeBuffer (seconds, fs);
            auto* o = out.getWritePointer (0);
            for (int i = 0; i < out.getNumSamples(); ++i)
                o[i] = (float) sampleAt (i / fs);
        };

        switch (kind)
        {
            case 0:   // rim
            {
                const auto f = d.range (420.0f, 880.0f) * tr, dec = d.range (0.02f, 0.05f) * ds;
                f1.bandPass (d.range (2500.0f, 4500.0f), 1.2, fs);
                build (dec * 5.0f, [&] (double t)
                {
                    p1 += kTwoPi * f / fs;
                    return (std::sin (p1) * s.body * 1.4 + f1.process (d.noise()) * (1.3 - s.body)) * std::exp (-t / dec);
                });
                return "Rim";
            }
            case 1:   // tom / conga
            {
                const auto f = d.range (120.0f, 360.0f) * tr, dec = d.range (0.1f, 0.3f) * ds;
                const auto drop = std::pow (2.0, d.range (3.0f, 8.0f) / 12.0);
                root = hzToMidi (f);
                build (dec * 5.0f, [&] (double t)
                {
                    p1 += kTwoPi * f * (1.0 + (drop - 1.0) * std::exp (-t / 0.02)) / fs;
                    return (std::sin (p1) + d.noise() * 0.05 * (1.0 - s.body)) * std::exp (-t / dec) * juce::jmin (1.0, t / 0.0008);
                });
                return d.chance (0.5f) ? "Tom" : "Conga";
            }
            case 2:   // shaker
            {
                const auto dec = d.range (0.05f, 0.11f) * ds, att = d.range (0.006f, 0.016f);
                f1.bandPass (d.range (5000.0f, 9000.0f) * (0.8f + 0.4f * s.tone), 1.0, fs);
                build (att + dec * 5.0f, [&] (double t)
                {
                    const auto env = t < att ? t / att : std::exp (-(t - att) / dec);
                    return f1.process (d.noise()) * 2.0 * env;
                });
                return "Shaker";
            }
            case 3:   // bell
            {
                const auto f = d.range (520.0f, 640.0f) * tr, ratio = d.range (1.45f, 1.52f), dec = d.range (0.15f, 0.4f) * ds;
                f1.bandPass (d.range (1200.0f, 2600.0f), 0.8, fs);
                root = hzToMidi (f);
                build (dec * 5.0f, [&] (double t)
                {
                    p1 += f / fs; p1 -= std::floor (p1);
                    p2 += f * ratio / fs; p2 -= std::floor (p2);
                    const auto sq = (p1 < 0.5 ? 1.0 : -1.0) + (p2 < 0.5 ? 0.8 : -0.8);
                    return f1.process (sq) * std::exp (-t / dec) * 1.4;
                });
                return "Bell";
            }
            default:  // wood block
            {
                const auto f = d.range (700.0f, 1300.0f) * tr, dec = d.range (0.025f, 0.065f) * ds;
                build (dec * 5.0f, [&] (double t)
                {
                    p1 += kTwoPi * f / fs;
                    p2 += kTwoPi * f * 2.71 / fs;
                    return (std::sin (p1) + 0.4 * std::sin (p2) * std::exp (-t / (dec * 0.4))) * std::exp (-t / dec);
                });
                return "Block";
            }
        }
    }

    juce::String renderFx (Dice& d, juce::AudioBuffer<float>& out, double fs, int mood, const DrumShape& s, float& drive)
    {
        const auto kind = d.pick (5);
        const auto ds = decayScale (s), tr = tuneRatio (s);
        drive = moodDrive (d, mood, 0.1f, 0.5f);
        Biquad f1;

        switch (kind)
        {
            case 0:   // riser
            {
                const auto len = d.range (1.2f, 2.4f) * ds;
                out = makeBuffer (len, fs);
                auto* o = out.getWritePointer (0);
                double ph = 0.0;
                for (int i = 0; i < out.getNumSamples(); ++i)
                {
                    const auto x = i / (fs * len);
                    if (i % 32 == 0)
                        f1.bandPass (300.0 * std::pow (30.0, x) * tr, 2.5, fs);
                    ph += 120.0 * std::pow (8.0, x) * tr / fs; ph -= std::floor (ph);
                    const auto saw = (ph * 2.0 - 1.0) * 0.25 * s.body;
                    o[i] = (float) ((f1.process (d.noise()) * 2.5 + saw) * x * x * juce::jmin (1.0, (1.0 - x) * 40.0));
                }
                return "Rise";
            }
            case 1:   // impact
            {
                const auto len = d.range (1.4f, 2.6f) * ds;
                out = makeBuffer (len, fs);
                auto* o = out.getWritePointer (0);
                double ph = 0.0;
                for (int i = 0; i < out.getNumSamples(); ++i)
                {
                    const auto t = i / fs;
                    if (i % 32 == 0)
                        f1.lowPass (200.0 + 6000.0 * std::exp (-t / 0.15), 0.7, fs);
                    ph += kTwoPi * (28.0 + 55.0 * std::exp (-t / 0.25)) * tr / fs;
                    o[i] = (float) ((std::sin (ph) * 1.2 + f1.process (d.noise()) * 0.9) * std::exp (-t / (len * 0.25)));
                }
                addRoom (out, fs, 0.3f, 1.2f);
                return "Impact";
            }
            case 2:   // reverse crash
            {
                const auto len = d.range (1.2f, 2.0f) * ds;
                out = makeBuffer (len, fs);
                auto* o = out.getWritePointer (0);
                Biquad hp;
                hp.highPass (d.range (3000.0f, 5500.0f), 0.7, fs);
                for (int i = 0; i < out.getNumSamples(); ++i)
                    o[i] = (float) (hp.process (d.noise()) * std::exp (-(i / fs) / (len * 0.3)));
                out.reverse (0, out.getNumSamples());
                return "Reverse";
            }
            case 3:   // downlifter
            {
                const auto len = d.range (0.9f, 1.8f) * ds;
                out = makeBuffer (len, fs);
                auto* o = out.getWritePointer (0);
                for (int i = 0; i < out.getNumSamples(); ++i)
                {
                    const auto x = i / (fs * len);
                    if (i % 32 == 0)
                        f1.bandPass (8000.0 * std::pow (0.03, x) * tr, 2.2, fs);
                    o[i] = (float) (f1.process (d.noise()) * 2.5 * (1.0 - x) * juce::jmin (1.0, x * 60.0));
                }
                return "Drop";
            }
            default:  // zap
            {
                const auto len = d.range (0.12f, 0.32f) * ds;
                out = makeBuffer (len + 0.02, fs);
                auto* o = out.getWritePointer (0);
                double ph = 0.0;
                for (int i = 0; i < out.getNumSamples(); ++i)
                {
                    const auto t = i / fs;
                    const auto x = juce::jmin (1.0, t / len);
                    ph += kTwoPi * (120.0 + 2900.0 * std::pow (1.0 - x, 3.0)) * tr * (1.0 + 0.04 * std::sin (t * 260.0)) / fs;
                    o[i] = (float) (std::sin (ph) * (1.0 - x));
                }
                return "Zap";
            }
        }
    }

    const juce::StringArray& adjectives()
    {
        static const juce::StringArray a { "Grave", "Rust", "Blood", "Toxic", "Crypt", "Iron", "Venom", "Feral", "Hex", "Ghoul",
                                           "Riot", "Ash", "Bone", "Chrome", "Voodoo", "Doom", "Savage", "Lethal", "Cursed", "Molten",
                                           "Black", "Dead", "Wicked", "Brutal", "Sinister", "Hollow", "Reaper", "Night", "Smoke", "Acid",
                                           "Rabid", "Coffin", "Static", "Satan", "Morgue", "Nitro", "Psycho", "Lunar", "Plague", "Ruin" };
        return a;
    }
}

//==============================================================================
const char* drumTypeName (DrumType t)
{
    static const char* n[] { "808", "KICK", "SNARE", "CLAP", "HI-HAT", "OPEN HAT", "PERC", "FX" };
    return n[juce::jlimit (0, kNumDrumTypes - 1, (int) t)];
}

const char* drumTypeFolder (DrumType t)
{
    static const char* n[] { "808s", "Kicks", "Snares", "Claps", "Hi-Hats", "Open Hats", "Percussion", "FX" };
    return n[juce::jlimit (0, kNumDrumTypes - 1, (int) t)];
}

juce::String noteNameOf (int midiNote)
{
    static const char* n[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return n[((midiNote % 12) + 12) % 12];
}

juce::String makeDrumName (DrumType type, juce::uint32 seed)
{
    static const juce::StringArray nouns[] {
        { "Tank", "Quake", "Bully", "Monster", "Rumble", "Grizzly", "Tremor", "Bulldozer", "Earth", "Boom", "Slab", "Titan" },
        { "Knock", "Punch", "Fist", "Hammer", "Stomp", "Slam", "Brick", "Thump", "Club", "Boot" },
        { "Crack", "Snap", "Whip", "Shot", "Slap", "Lash", "Strike", "Smack" },
        { "Clap", "Palms", "Smack", "Crowd", "Gang", "Hands", "Applause", "Slap" },
        { "Tick", "Spark", "Chain", "Needle", "Razor", "Sizzle", "Blade", "Tsk" },
        { "Hiss", "Wash", "Spray", "Breath", "Steam", "Splash" },
        { "Perc" }, { "FX" } };
    juce::Random r ((juce::int64) seed * 2654435761LL + (int) type * 977);
    const auto& adj = adjectives();
    const auto& list = nouns[juce::jlimit (0, kNumDrumTypes - 1, (int) type)];
    return adj[r.nextInt (adj.size())] + " " + list[r.nextInt (list.size())];
}

DrumSoundPtr synthesizeDrum (DrumType type, juce::uint32 seed, int mood, const DrumShape& shape, double fs)
{
    Dice d ((juce::int64) seed * 7919 + (int) type * 131 + juce::jlimit (0, 2, mood) * 17 + 1);
    auto snd = std::make_shared<DrumSound>();
    snd->type = type;
    snd->seed = seed;
    snd->mood = mood;
    snd->shape = shape;
    snd->sampleRate = fs;
    snd->name = makeDrumName (type, seed);

    float drive = 0.0f;
    int root = 60;
    switch (type)
    {
        case DrumType::b808:    render808 (d, snd->audio, fs, mood, shape, root, drive); break;
        case DrumType::kick:    renderKick (d, snd->audio, fs, mood, shape, root, drive); break;
        case DrumType::snare:   renderSnare (d, snd->audio, fs, mood, shape, drive); break;
        case DrumType::clap:    renderClap (d, snd->audio, fs, mood, shape, drive); break;
        case DrumType::hat:     renderHat (d, snd->audio, fs, mood, shape, false, drive); break;
        case DrumType::openHat: renderHat (d, snd->audio, fs, mood, shape, true, drive); break;
        case DrumType::perc:
        {
            const auto kind = renderPerc (d, snd->audio, fs, mood, shape, root, drive);
            snd->name = snd->name.upToFirstOccurrenceOf (" ", false, false) + " " + kind;
            break;
        }
        case DrumType::fx:
        {
            const auto kind = renderFx (d, snd->audio, fs, mood, shape, drive);
            snd->name = snd->name.upToFirstOccurrenceOf (" ", false, false) + " " + kind;
            break;
        }
    }
    snd->rootNote = root;
    finish (snd->audio, fs, shape, drive);
    return snd;
}

void shapeRecording (juce::AudioBuffer<float>& audio, double fs, const DrumShape& shape)
{
    // tune = resample (like a sampler), decay < 0.5 = shorter (fade), then the same knobs as the synth
    if (std::abs (shape.tune) > 0.01f)
    {
        const auto ratio = tuneRatio (shape);
        const auto outLen = juce::jmax (16, (int) (audio.getNumSamples() / ratio));
        juce::AudioBuffer<float> out (audio.getNumChannels(), outLen);
        for (int ch = 0; ch < audio.getNumChannels(); ++ch)
        {
            juce::LagrangeInterpolator interp;
            interp.process (ratio, audio.getReadPointer (ch), out.getWritePointer (ch), outLen, audio.getNumSamples(), 0);
        }
        audio = std::move (out);
    }
    if (shape.decay < 0.5f)
    {
        const auto keep = juce::jmax (0.05f, shape.decay * 2.0f);
        const auto n = juce::jmax (16, (int) (audio.getNumSamples() * keep));
        audio.setSize (audio.getNumChannels(), n, true, true, false);
        audio.applyGainRamp (n / 3, n - n / 3, 1.0f, 0.0f);
    }
    finish (audio, fs, shape, 0.0f);
}

namespace
{
    int detectRoot (const juce::AudioBuffer<float>& b, double fs)
    {
        // 808 note: count zero crossings of the low-passed body between 60 and 400 ms
        Biquad lp;
        lp.lowPass (160.0, 0.7, fs);
        const auto from = (int) (0.06 * fs), to = juce::jmin (b.getNumSamples(), (int) (0.4 * fs));
        if (to - from < (int) (0.05 * fs))
            return 60;
        int crossings = 0;
        double prev = 0.0;
        for (int i = 0; i < to; ++i)
        {
            const auto y = lp.process (b.getSample (0, i));
            if (i >= from && prev <= 0.0 && y > 0.0)
                ++crossings;
            prev = y;
        }
        const auto hz = crossings / ((to - from) / fs);
        return hz > 20.0 && hz < 200.0 ? hzToMidi (hz) : 60;
    }
}

DrumSoundPtr loadDrumFile (DrumType type, const juce::File& file, const DrumShape& shape, double fs, juce::String& error)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr)
    {
        error = "Cannot read " + file.getFileName();
        return nullptr;
    }
    const auto maxLen = (juce::int64) (10.0 * reader->sampleRate);
    const auto len = (int) juce::jmin (reader->lengthInSamples, maxLen);
    const auto chans = (int) juce::jlimit (1u, 2u, reader->numChannels);
    juce::AudioBuffer<float> raw (chans, juce::jmax (16, len));
    raw.clear();
    reader->read (&raw, 0, len, 0, true, chans > 1);

    juce::AudioBuffer<float> audio;
    if (std::abs (reader->sampleRate - fs) > 1.0)
    {
        const auto ratio = reader->sampleRate / fs;
        const auto outLen = juce::jmax (16, (int) (raw.getNumSamples() / ratio));
        audio.setSize (chans, outLen);
        for (int ch = 0; ch < chans; ++ch)
        {
            juce::LagrangeInterpolator interp;
            interp.process (ratio, raw.getReadPointer (ch), audio.getWritePointer (ch), outLen, raw.getNumSamples(), 0);
        }
    }
    else
    {
        audio = std::move (raw);
    }

    auto snd = std::make_shared<DrumSound>();
    snd->type = type;
    snd->shape = shape;
    snd->sampleRate = fs;
    snd->file = file;
    snd->name = file.getFileNameWithoutExtension();
    snd->rootNote = (type == DrumType::b808 || type == DrumType::kick) ? detectRoot (audio, fs) : 60;
    if (snd->rootNote != 60)
        snd->rootNote = juce::jlimit (0, 127, snd->rootNote + (int) std::lround (shape.tune));
    shapeRecording (audio, fs, shape);
    snd->audio = std::move (audio);
    return snd;
}

} // namespace rk
