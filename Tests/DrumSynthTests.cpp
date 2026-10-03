#include "engine/DrumSynth.h"

#include <cmath>

namespace rk
{

class DrumSynthTests : public juce::UnitTest
{
public:
    DrumSynthTests() : juce::UnitTest ("Drum synth", "RollsKilla") {}

    static double centroid (const juce::AudioBuffer<float>& b, double fs)
    {
        // power-weighted spectral centroid of the first 4096 samples (Hann window)
        const int n = juce::jmin (4096, b.getNumSamples());
        double num = 0.0, den = 0.0;
        for (int k = 1; k < n / 2; k += 4)
        {
            double re = 0.0, im = 0.0;
            for (int i = 0; i < n; ++i)
            {
                const auto w = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * i / (n - 1));
                const auto ph = juce::MathConstants<double>::twoPi * k * i / n;
                re += b.getSample (0, i) * w * std::cos (ph);
                im -= b.getSample (0, i) * w * std::sin (ph);
            }
            const auto pw = re * re + im * im;
            num += pw * k * fs / n;
            den += pw;
        }
        return den > 0.0 ? num / den : 0.0;
    }

    void runTest() override
    {
        constexpr double fs = 44100.0;

        beginTest ("every type renders a clean, normalised one-shot");
        for (int t = 0; t < kNumDrumTypes; ++t)
            for (juce::uint32 seed = 1; seed < 40; seed += 7)
                for (int mood = 0; mood < 3; ++mood)
                {
                    const auto s = synthesizeDrum (drumTypeFromIndex (t), seed, mood, {}, fs);
                    expect (s != nullptr && s->audio.getNumSamples() > 100);
                    bool finite = true;
                    for (int i = 0; i < s->audio.getNumSamples(); ++i)
                        finite = finite && std::isfinite (s->audio.getSample (0, i));
                    expect (finite, drumTypeName (drumTypeFromIndex (t)));
                    const auto peak = s->audio.getMagnitude (0, s->audio.getNumSamples());
                    expect (peak > 0.9f && peak < 0.95f, juce::String (drumTypeName (drumTypeFromIndex (t))) + " peak " + juce::String (peak));
                    expect (std::abs (s->audio.getSample (0, s->audio.getNumSamples() - 1)) < 0.01f, "ends at silence");
                    expect (s->name.isNotEmpty());
                }

        beginTest ("deterministic: the same seed gives the same sound");
        for (int t = 0; t < kNumDrumTypes; ++t)
        {
            const auto a = synthesizeDrum (drumTypeFromIndex (t), 1234, 1, {}, fs);
            const auto b = synthesizeDrum (drumTypeFromIndex (t), 1234, 1, {}, fs);
            const auto c = synthesizeDrum (drumTypeFromIndex (t), 1235, 1, {}, fs);
            bool same = a->audio.getNumSamples() == b->audio.getNumSamples();
            for (int i = 0; same && i < a->audio.getNumSamples(); ++i)
                same = juce::exactlyEqual (a->audio.getSample (0, i), b->audio.getSample (0, i));
            expect (same && a->name == b->name);
            bool differs = a->audio.getNumSamples() != c->audio.getNumSamples();
            for (int i = 0; ! differs && i < a->audio.getNumSamples(); ++i)
                differs = ! juce::exactlyEqual (a->audio.getSample (0, i), c->audio.getSample (0, i));
            expect (differs, "another seed = another sound");
        }

        beginTest ("trap ranges: 808 long and low, hats short and bright, open hats longer");
        for (juce::uint32 seed = 3; seed < 60; seed += 11)
        {
            const auto b808 = synthesizeDrum (DrumType::b808, seed, 1, {}, fs);
            const auto len808 = b808->audio.getNumSamples() / fs;
            expect (len808 > 0.4 && len808 < 4.2, "808 length " + juce::String (len808));
            expect (b808->rootNote >= 20 && b808->rootNote <= 40, "808 root " + juce::String (b808->rootNote));
            expect (centroid (b808->audio, fs) < 1500.0, "808 is dark " + juce::String (centroid (b808->audio, fs)));

            const auto hat = synthesizeDrum (DrumType::hat, seed, 1, {}, fs);
            const auto open = synthesizeDrum (DrumType::openHat, seed, 1, {}, fs);
            expect (hat->audio.getNumSamples() / fs < 0.5, "closed hat short");
            expect (open->audio.getNumSamples() > hat->audio.getNumSamples(), "open hat longer");
            expect (centroid (hat->audio, fs) > 5000.0, "hat bright " + juce::String (centroid (hat->audio, fs)));

            const auto kick = synthesizeDrum (DrumType::kick, seed, 1, {}, fs);
            expect (kick->audio.getNumSamples() / fs < 1.5 && kick->audio.getNumSamples() < b808->audio.getNumSamples(), "kick shorter than 808");
        }

        beginTest ("knobs: decay makes it longer, tune moves the 808 note");
        {
            DrumShape longer;
            longer.decay = 1.0f;
            DrumShape up;
            up.tune = 5.0f;
            const auto base = synthesizeDrum (DrumType::b808, 77, 1, {}, fs);
            expect (synthesizeDrum (DrumType::b808, 77, 1, longer, fs)->audio.getNumSamples() > base->audio.getNumSamples());
            expectEquals (synthesizeDrum (DrumType::b808, 77, 1, up, fs)->rootNote, base->rootNote + 5);
        }
    }
};

static DrumSynthTests drumSynthTests;

} // namespace rk
