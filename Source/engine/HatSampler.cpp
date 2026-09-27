#include "HatSampler.h"
#include "HatSynth.h"

#include "SampleData.h"

#include <cmath>

namespace rk
{

HatSampler::HatSampler()
{
    loadFactorySamples();
}

static std::shared_ptr<const HatSample> readSample (std::unique_ptr<juce::InputStream> stream, const juce::String& name, juce::String& error)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (std::move (stream)));
    if (reader == nullptr)
    {
        error = "unsupported audio file";
        return nullptr;
    }

    const auto maxSeconds = 4.0;
    const auto length = (int) std::min<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * maxSeconds));
    if (length <= 0)
    {
        error = "empty audio file";
        return nullptr;
    }

    auto s = std::make_shared<HatSample>();
    s->name = name;
    s->sampleRate = reader->sampleRate;
    s->audio.setSize ((int) std::min (2u, reader->numChannels), length);
    reader->read (&s->audio, 0, length, 0, true, s->audio.getNumChannels() > 1);
    return s;
}

void HatSampler::loadFactorySamples()
{
    for (int i = 0; i < kNumFactoryHats; ++i)
    {
        const auto wanted = juce::String ("Killa_Hat_") + juce::String (i + 1).paddedLeft ('0', 2) + ".wav";
        std::shared_ptr<const HatSample> loaded;

        for (int r = 0; r < SampleData::namedResourceListSize; ++r)
        {
            if (! juce::String (SampleData::originalFilenames[r]).equalsIgnoreCase (wanted))
                continue;

            int size = 0;
            const auto* data = SampleData::getNamedResource (SampleData::namedResourceList[r], size);
            juce::String error;
            loaded = readSample (std::make_unique<juce::MemoryInputStream> (data, (size_t) size, false),
                                 "Killa Hat " + juce::String (i + 1).paddedLeft ('0', 2), error);
        }

        if (loaded == nullptr)
        {
            auto s = std::make_shared<HatSample>();
            s->name = "Killa Hat " + juce::String (i + 1).paddedLeft ('0', 2);
            s->audio = synthesizeClosedHat (i, 44100.0);
            s->sampleRate = 44100.0;
            s->synthesized = true;
            loaded = s;
        }

        slots[(size_t) i] = loaded;
    }

    publishSet();
}

juce::String HatSampler::loadCustomSample (const juce::File& file)
{
    juce::String error;
    auto stream = file.createInputStream();
    if (stream == nullptr)
        return "cannot open " + file.getFileName();

    auto s = readSample (std::move (stream), file.getFileNameWithoutExtension(), error);
    if (s == nullptr)
        return error;

    customFile = file;
    setCustomSample (s);
    return {};
}

void HatSampler::setCustomSample (std::shared_ptr<const HatSample> sample)
{
    slots[kCustomSlot] = std::move (sample);
    publishSet();
}

juce::StringArray HatSampler::getSampleNames() const
{
    juce::StringArray names;
    for (size_t i = 0; i < slots.size(); ++i)
        names.add (slots[i] != nullptr ? slots[i]->name : juce::String ("Custom WAV"));
    return names;
}

std::shared_ptr<const HatSample> HatSampler::getSample (int index) const
{
    if (index < 0 || index >= (int) slots.size())
        return nullptr;
    return slots[(size_t) index];
}

void HatSampler::publishSet()
{
    auto set = std::make_unique<HatSampleSet>();
    for (const auto& s : slots)
        set->samples.push_back (s);
    slot.publish (std::move (set));
}

//==============================================================================
void HatSampler::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    for (auto& v : voices)
        v.active = false;
}

void HatSampler::beginBlock (const HatSamplerSettings& newSettings) noexcept
{
    settings = newSettings;
    currentSet = slot.acquire();

    if (currentSet != lastSet)
    {
        // Voices may point into the previous set - drop them before it can be freed.
        for (auto& v : voices)
            v.active = false;
        lastSet = currentSet;
    }
}

void HatSampler::endBlock() noexcept
{
    slot.release();
    currentSet = nullptr;
}

void HatSampler::noteOn (int midiNote, int velocity) noexcept
{
    if (currentSet == nullptr || velocity <= 0)
        return;

    const auto index = juce::jlimit (0, (int) currentSet->samples.size() - 1, settings.sampleIndex);
    const HatSample* s = currentSet->samples[(size_t) index].get();
    if (s == nullptr)
        s = currentSet->samples[0].get();
    if (s == nullptr || s->audio.getNumSamples() == 0)
        return;

    const auto fadeSamples = (float) (kChokeFadeMs * 0.001 * sampleRate);

    if (settings.choke)
        for (auto& v : voices)
            if (v.active && v.fadeStep <= 0.0f)
                v.fadeStep = 1.0f / std::max (1.0f, fadeSamples);

    // Pick a free voice, otherwise steal the oldest one.
    Voice* target = nullptr;
    for (auto& v : voices)
        if (! v.active) { target = &v; break; }

    if (target == nullptr)
    {
        target = &voices[0];
        for (auto& v : voices)
            if (v.age < target->age)
                target = &v;
    }

    const auto semis = (double) (midiNote - kRootNote) + settings.tuneSemitones;
    const auto vel01 = juce::jlimit (0.0f, 1.0f, (float) velocity / 127.0f);

    Voice v;
    v.sample = s;
    v.pos = 0.0;
    v.increment = s->sampleRate / sampleRate * std::pow (2.0, semis / 12.0);
    v.gain = std::pow (vel01, 1.4f) * juce::Decibels::decibelsToGain (settings.volumeDb);
    v.env = 1.0f;
    v.envCoef = settings.decayMs >= kDecayOffMs
                  ? 1.0f
                  : (float) std::exp (-6.9 / (std::max (5.0f, settings.decayMs) * 0.001 * sampleRate));
    v.fade = 1.0f;
    v.fadeStep = 0.0f;
    v.age = ++ageCounter;
    v.active = true;
    *target = v;
}

void HatSampler::allNotesOff() noexcept
{
    const auto fadeSamples = (float) (kChokeFadeMs * 0.001 * sampleRate);
    for (auto& v : voices)
        if (v.active && v.fadeStep <= 0.0f)
            v.fadeStep = 1.0f / std::max (1.0f, fadeSamples);
}

int HatSampler::getNumActiveVoices() const noexcept
{
    int n = 0;
    for (const auto& v : voices)
        n += v.active ? 1 : 0;
    return n;
}

static inline float hermite (const float* d, int n, double pos) noexcept
{
    const auto i = (int) pos;
    const auto f = (float) (pos - i);
    auto at = [d, n] (int k) { return k >= 0 && k < n ? d[k] : 0.0f; };
    const auto xm1 = at (i - 1), x0 = at (i), x1 = at (i + 1), x2 = at (i + 2);
    const auto c1 = 0.5f * (x1 - xm1);
    const auto c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
    const auto c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
    return ((c3 * f + c2) * f + c1) * f + x0;
}

void HatSampler::render (juce::AudioBuffer<float>& buffer, int startSample, int numSamples) noexcept
{
    if (numSamples <= 0)
        return;

    const auto outChannels = buffer.getNumChannels();

    for (auto& v : voices)
    {
        if (! v.active)
            continue;

        const auto& audio = v.sample->audio;
        const auto len = audio.getNumSamples();
        const auto* left = audio.getReadPointer (0);
        const auto* right = audio.getNumChannels() > 1 ? audio.getReadPointer (1) : left;

        for (int i = 0; i < numSamples; ++i)
        {
            if (v.pos >= len || v.fade <= 0.0f)
            {
                v.active = false;
                break;
            }

            const auto g = v.gain * v.env * v.fade;
            const auto l = hermite (left, len, v.pos) * g;
            const auto r = right == left ? l : hermite (right, len, v.pos) * g;

            buffer.addSample (0, startSample + i, l);
            if (outChannels > 1)
                buffer.addSample (1, startSample + i, r);

            v.pos += v.increment;
            v.env *= v.envCoef;
            if (v.fadeStep > 0.0f)
                v.fade -= v.fadeStep;
        }
    }
}

} // namespace rk
