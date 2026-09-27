#include "HatSynth.h"

#include <cmath>

namespace rk
{

namespace
{
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

        static Biquad highPass (double fc, double q, double fs)
        {
            const auto w = juce::MathConstants<double>::twoPi * fc / fs;
            const auto alpha = std::sin (w) / (2.0 * q);
            const auto c = std::cos (w);
            const auto a0 = 1.0 + alpha;
            Biquad f;
            f.b0 = (1.0 + c) / 2.0 / a0;
            f.b1 = -(1.0 + c) / a0;
            f.b2 = (1.0 + c) / 2.0 / a0;
            f.a1 = -2.0 * c / a0;
            f.a2 = (1.0 - alpha) / a0;
            return f;
        }

        static Biquad lowPass (double fc, double q, double fs)
        {
            const auto w = juce::MathConstants<double>::twoPi * fc / fs;
            const auto alpha = std::sin (w) / (2.0 * q);
            const auto c = std::cos (w);
            const auto a0 = 1.0 + alpha;
            Biquad f;
            f.b0 = (1.0 - c) / 2.0 / a0;
            f.b1 = (1.0 - c) / a0;
            f.b2 = (1.0 - c) / 2.0 / a0;
            f.a1 = -2.0 * c / a0;
            f.a2 = (1.0 - alpha) / a0;
            return f;
        }

        static Biquad peak (double fc, double q, double gainDb, double fs)
        {
            const auto A = std::pow (10.0, gainDb / 40.0);
            const auto w = juce::MathConstants<double>::twoPi * fc / fs;
            const auto alpha = std::sin (w) / (2.0 * q);
            const auto c = std::cos (w);
            const auto a0 = 1.0 + alpha / A;
            Biquad f;
            f.b0 = (1.0 + alpha * A) / a0;
            f.b1 = -2.0 * c / a0;
            f.b2 = (1.0 - alpha * A) / a0;
            f.a1 = -2.0 * c / a0;
            f.a2 = (1.0 - alpha / A) / a0;
            return f;
        }

        double process (double x)
        {
            const auto y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
    };
}

juce::AudioBuffer<float> synthesizeClosedHat (int variant, double fs)
{
    variant = juce::jlimit (0, 13, variant);

    // Variants go from tight/dark to long/bright, roughly matching the Killa kit (75-185 ms, 7.7-10 kHz).
    const auto t = variant / 13.0;
    const auto decayMs = 38.0 + 60.0 * std::fmod (t * 2.7 + 0.15, 1.0) + 25.0 * t;
    const auto lengthMs = decayMs * 2.2 + 10.0;
    const auto hpHz = 5200.0 + 1800.0 * std::fmod (t * 1.9 + 0.3, 1.0);
    const auto lpHz = 9500.0 + 3000.0 * std::fmod (t * 2.3 + 0.1, 1.0);
    const auto presenceHz = 7800.0 + 1800.0 * t;
    const auto metal = 0.35 + 0.4 * std::fmod (t * 3.3, 1.0);
    const auto detune = 1.0 + 0.06 * (std::fmod (t * 5.1, 1.0) - 0.5);

    const auto numSamples = (int) (lengthMs * 0.001 * fs);
    juce::AudioBuffer<float> out (1, numSamples);

    // Classic six-oscillator metallic cluster + noise
    const double freqs[] { 205.3, 304.4, 369.6, 522.7, 540.0, 800.0 };
    double phases[6] {};
    uint32_t rng = 0x9e3779b9u * (uint32_t) (variant + 1);

    auto hp1 = Biquad::highPass (hpHz, 0.7, fs);
    auto hp2 = Biquad::highPass (hpHz * 0.9, 0.6, fs);
    auto pk = Biquad::peak (presenceHz, 1.2, 5.0, fs);
    auto lp1 = Biquad::lowPass (lpHz, 0.7, fs);
    auto lp2 = Biquad::lowPass (lpHz * 1.2, 0.6, fs);

    const auto tau = decayMs * 0.001 / 6.9;       // -60 dB at decayMs
    auto* d = out.getWritePointer (0);
    double peakLevel = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        const auto time = i / fs;

        double sq = 0.0;
        for (int o = 0; o < 6; ++o)
        {
            phases[o] += freqs[o] * detune * 1.47 / fs;
            phases[o] -= std::floor (phases[o]);
            sq += phases[o] < 0.5 ? 1.0 : -1.0;
        }

        rng = rng * 1664525u + 1013904223u;
        const auto noise = ((rng >> 8) / 8388608.0) - 1.0;

        auto x = metal * sq / 6.0 + (1.0 - metal) * noise;
        x = lp2.process (lp1.process (pk.process (hp2.process (hp1.process (x)))));

        const auto attack = std::min (1.0, time / 0.0004);
        const auto env = attack * std::exp (-time / tau);
        const auto fadeOut = std::min (1.0, (numSamples - i) / (0.002 * fs));
        const auto y = x * env * fadeOut;

        d[i] = (float) y;
        peakLevel = std::max (peakLevel, std::abs (y));
    }

    if (peakLevel > 0.0)
        out.applyGain ((float) (0.89 / peakLevel));      // -1 dBFS

    return out;
}

} // namespace rk
