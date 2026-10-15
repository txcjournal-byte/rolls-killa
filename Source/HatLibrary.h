#pragma once
#include <juce_audio_formats/juce_audio_formats.h>

// The 120 factory hats compiled into the plugin (assets/hats/*.wav), grouped in 6 banks of 20.
// Loaded once, shared by every plugin instance, read-only after loading.
struct HatSample
{
    juce::String name;      // "BASIC 01"
    juce::String bank;      // "BASIC"
    juce::AudioBuffer<float> audio; // mono
    double sampleRate = 44100.0;
};

class HatLibrary
{
public:
    static const HatLibrary& get();

    int size() const noexcept { return (int) hats.size(); }
    const HatSample& hat (int index) const noexcept { return *hats[(size_t) juce::jlimit (0, size() - 1, index)]; }
    static juce::StringArray bankNames() { return { "BASIC", "CRISP", "DIRTY", "LOFI", "EXTREME", "WEIRD" }; }
    static constexpr int hatsPerBank = 20;

    // Loads any audio file into a mono buffer (user WAV / AIFF / FLAC / MP3). Returns false if unreadable.
    static bool loadFile (const juce::File& file, HatSample& out, double maxSeconds = 4.0);

private:
    HatLibrary();
    std::vector<std::unique_ptr<HatSample>> hats;
};
