#include "HatLibrary.h"
#include <BinaryData.h>

static void toMono (juce::AudioFormatReader& reader, HatSample& out, double maxSeconds)
{
    auto len = (int) juce::jmin<juce::int64> (reader.lengthInSamples, (juce::int64) (maxSeconds * reader.sampleRate));
    juce::AudioBuffer<float> tmp ((int) juce::jmax (1u, reader.numChannels), juce::jmax (1, len));
    tmp.clear();
    reader.read (&tmp, 0, len, 0, true, true);
    out.audio.setSize (1, juce::jmax (1, len));
    out.audio.clear();
    for (int ch = 0; ch < tmp.getNumChannels(); ++ch)
        out.audio.addFrom (0, 0, tmp, ch, 0, len, 1.0f / (float) tmp.getNumChannels());
    out.sampleRate = reader.sampleRate;
}

const HatLibrary& HatLibrary::get()
{
    static HatLibrary lib;
    return lib;
}

HatLibrary::HatLibrary()
{
    juce::WavAudioFormat wav;
    struct Entry { juce::String file; const char* data; int size; };
    std::vector<Entry> entries;

    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        juce::String original (BinaryData::originalFilenames[i]);
        if (! original.endsWithIgnoreCase (".wav"))
            continue;
        int size = 0;
        auto* data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], size);
        if (data != nullptr && size > 0)
            entries.push_back ({ original, data, size });
    }

    // order: bank order, then number inside the bank
    auto banks = bankNames();
    std::sort (entries.begin(), entries.end(), [&] (const Entry& a, const Entry& b)
    {
        auto ba = banks.indexOf (a.file.upToFirstOccurrenceOf ("_", false, false).toUpperCase());
        auto bb = banks.indexOf (b.file.upToFirstOccurrenceOf ("_", false, false).toUpperCase());
        if (ba != bb) return ba < bb;
        return a.file < b.file;
    });

    for (auto& e : entries)
    {
        auto stream = std::make_unique<juce::MemoryInputStream> (e.data, (size_t) e.size, false);
        std::unique_ptr<juce::AudioFormatReader> reader (wav.createReaderFor (stream.release(), true));
        if (reader == nullptr)
            continue;
        auto h = std::make_unique<HatSample>();
        toMono (*reader, *h, 2.0);
        auto base = e.file.upToLastOccurrenceOf (".", false, false);       // basic_01
        h->bank = base.upToFirstOccurrenceOf ("_", false, false).toUpperCase();
        h->name = h->bank + " " + base.fromFirstOccurrenceOf ("_", false, false);
        hats.push_back (std::move (h));
    }

    if (hats.empty())   // never leave the engine without a sound
    {
        auto h = std::make_unique<HatSample>();
        h->name = "CLICK"; h->bank = "BASIC";
        h->audio.setSize (1, 64); h->audio.clear(); h->audio.setSample (0, 0, 1.0f);
        hats.push_back (std::move (h));
    }
}

bool HatLibrary::loadFile (const juce::File& file, HatSample& out, double maxSeconds)
{
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples <= 0)
        return false;
    toMono (*reader, out, maxSeconds);
    // normalise to -1 dBFS so a quiet user sample still rolls at a usable level
    auto peak = out.audio.getMagnitude (0, 0, out.audio.getNumSamples());
    if (peak > 0.0001f)
        out.audio.applyGain (0.89f / peak);
    out.name = file.getFileNameWithoutExtension().toUpperCase();
    out.bank = "YOUR WAV";
    return true;
}
