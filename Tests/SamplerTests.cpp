#include "engine/HatSampler.h"
#include "engine/HatSynth.h"

#include <cmath>

namespace rk
{

class SamplerTests : public juce::UnitTest
{
public:
    SamplerTests() : juce::UnitTest ("Hat sampler", "RollsKilla") {}

    static double spectralCentroid (const juce::AudioBuffer<float>& b, double fs)
    {
        const int n = std::min (2048, b.getNumSamples());
        const auto* d = b.getReadPointer (0);
        double num = 0.0, den = 0.0;
        for (int k = 1; k < n / 2; k += 4)
        {
            double re = 0.0, im = 0.0;
            for (int i = 0; i < n; ++i)
            {
                const auto w = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * i / (n - 1));
                const auto ph = juce::MathConstants<double>::twoPi * k * i / n;
                re += d[i] * w * std::cos (ph);
                im -= d[i] * w * std::sin (ph);
            }
            const auto mag = std::sqrt (re * re + im * im);
            num += mag * k * fs / n;
            den += mag;
        }
        return den > 0.0 ? num / den : 0.0;
    }

    static double decayTimeMs (const juce::AudioBuffer<float>& b, double fs, float dbDown)
    {
        const auto peak = b.getMagnitude (0, 0, b.getNumSamples());
        const auto threshold = peak * juce::Decibels::decibelsToGain (dbDown);
        int last = 0;
        for (int i = 0; i < b.getNumSamples(); ++i)
            if (std::abs (b.getSample (0, i)) >= threshold)
                last = i;
        return last * 1000.0 / fs;
    }

    void runTest() override
    {
        beginTest ("placeholder hats sound like closed hats");
        for (int v = 0; v < HatSampler::kNumFactoryHats; ++v)
        {
            const auto hat = synthesizeClosedHat (v, 44100.0);
            const auto lengthMs = hat.getNumSamples() * 1000.0 / 44100.0;
            const auto decay30 = decayTimeMs (hat, 44100.0, -30.0f);
            const auto centroid = spectralCentroid (hat, 44100.0);

            expectGreaterOrEqual (lengthMs, 75.0);
            expectLessOrEqual (lengthMs, 400.0);
            expectGreaterOrEqual (decay30, 20.0);
            expectLessOrEqual (decay30, 140.0);
            expectGreaterOrEqual (centroid, 6500.0, "centroid " + juce::String (centroid));
            expectLessOrEqual (centroid, 12000.0, "centroid " + juce::String (centroid));
            expectWithinAbsoluteError (hat.getMagnitude (0, 0, hat.getNumSamples()), 0.89f, 0.01f);
        }

        beginTest ("sample slots");
        HatSampler sampler;
        expectEquals (sampler.getSampleNames().size(), HatSampler::kNumFactoryHats + 1);
        expectEquals (sampler.getSampleNames()[2], juce::String ("Killa Hat 03"));

        sampler.prepare (48000.0);
        HatSamplerSettings settings;
        juce::AudioBuffer<float> out (2, 4800);

        beginTest ("choke cuts the previous hit within 3 ms");
        {
            settings.choke = true;
            out.clear();
            sampler.beginBlock (settings);
            sampler.noteOn (60, 127);
            sampler.render (out, 0, 100);
            expectEquals (sampler.getNumActiveVoices(), 1);
            sampler.noteOn (60, 127);
            expectEquals (sampler.getNumActiveVoices(), 2);
            sampler.render (out, 100, (int) (0.003 * 48000));
            expectEquals (sampler.getNumActiveVoices(), 1);
            sampler.allNotesOff();
            sampler.render (out, 400, 400);
            expectEquals (sampler.getNumActiveVoices(), 0);
            sampler.endBlock();
        }

        beginTest ("without choke hits overlap");
        {
            settings.choke = false;
            sampler.beginBlock (settings);
            sampler.noteOn (60, 127);
            sampler.render (out, 0, 100);
            sampler.noteOn (60, 127);
            sampler.render (out, 100, 200);
            expectEquals (sampler.getNumActiveVoices(), 2);
            sampler.allNotesOff();
            sampler.render (out, 300, 400);
            sampler.endBlock();
            settings.choke = true;
        }

        auto renderHit = [&] (int note, int vel, HatSamplerSettings s, int& lengthSamples)
        {
            juce::AudioBuffer<float> b (2, 48000);
            b.clear();
            sampler.beginBlock (s);
            sampler.noteOn (note, vel);
            lengthSamples = 0;
            for (int pos = 0; pos < b.getNumSamples() && sampler.getNumActiveVoices() > 0; pos += 256)
            {
                sampler.render (b, pos, std::min (256, b.getNumSamples() - pos));
                lengthSamples = pos + 256;
            }
            sampler.endBlock();
            return b;
        };

        beginTest ("pitch: +12 semitones plays twice as fast");
        {
            int base = 0, up = 0;
            renderHit (60, 127, settings, base);
            renderHit (72, 127, settings, up);
            expectWithinAbsoluteError ((double) up / base, 0.5, 0.05);
        }

        beginTest ("velocity, volume and decay change the level");
        {
            int len = 0;
            const auto loud = renderHit (60, 127, settings, len).getMagnitude (0, 0, 48000);
            const auto soft = renderHit (60, 64, settings, len).getMagnitude (0, 0, 48000);
            expectLessThan (soft, loud * 0.6f);

            auto quiet = settings;
            quiet.volumeDb = -12.0f;
            expectWithinAbsoluteError (renderHit (60, 127, quiet, len).getMagnitude (0, 0, 48000), loud * 0.251f, loud * 0.02f);

            auto shortDecay = settings;
            shortDecay.decayMs = 30.0f;
            const auto a = renderHit (60, 127, settings, len);
            const auto b = renderHit (60, 127, shortDecay, len);
            expectLessThan (b.getRMSLevel (0, 2400, 2400), a.getRMSLevel (0, 2400, 2400) * 0.5f);
        }

        beginTest ("custom sample slot");
        {
            auto s = std::make_shared<HatSample>();
            s->name = "Test";
            s->audio.setSize (1, 1000);
            s->audio.clear();
            s->audio.setSample (0, 0, 1.0f);
            sampler.setCustomSample (s);
            expectEquals (sampler.getSampleNames()[HatSampler::kCustomSlot], juce::String ("Test"));

            auto custom = settings;
            custom.sampleIndex = HatSampler::kCustomSlot;
            int len = 0;
            const auto b = renderHit (60, 127, custom, len);
            expectWithinAbsoluteError (b.getSample (0, 0), 1.0f, 1.0e-3f);
        }
    }
};

static SamplerTests samplerTests;

} // namespace rk
