#pragma once

#include "LockFreeSlot.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include <array>

namespace rk
{

struct HatSample
{
    juce::String name;
    juce::AudioBuffer<float> audio;     // 1 or 2 channels
    double sampleRate = 44100.0;
    bool synthesized = false;
};

/** Immutable set of samples handed to the audio thread. */
struct HatSampleSet
{
    std::vector<std::shared_ptr<const HatSample>> samples;
};

/** Settings read by the audio thread each block. */
struct HatSamplerSettings
{
    int sampleIndex = 0;
    float tuneSemitones = 0.0f;
    float decayMs = 300.0f;         // -60 dB point of the extra decay envelope; >= kDecayOffMs = off
    bool choke = true;
    float volumeDb = 0.0f;
};

/**
    Closed hi-hat sampler with choke. Real-time safe: render() and noteOn() never allocate or lock.
    With choke on, a new note fades the previous one out in ~2 ms so fast rolls stay tight.
*/
class HatSampler
{
public:
    static constexpr int kMaxVoices = 16;
    static constexpr int kRootNote = 60;
    static constexpr float kDecayOffMs = 1000.0f;
    static constexpr double kChokeFadeMs = 2.0;

    HatSampler();

    // ---- message thread -----------------------------------------------------
    /** Loads embedded Killa hats (or synthesized placeholders) into slots 0..13. */
    void loadFactorySamples();
    /** Loads a WAV/AIFF into the custom slot (index 14). Returns an error message or empty. */
    juce::String loadCustomSample (const juce::File& file);
    void setCustomSample (std::shared_ptr<const HatSample> sample);
    juce::StringArray getSampleNames() const;
    std::shared_ptr<const HatSample> getSample (int index) const;
    juce::File getCustomSampleFile() const { return customFile; }

    static constexpr int kNumFactoryHats = 14;
    static constexpr int kCustomSlot = kNumFactoryHats;

    // ---- audio thread -------------------------------------------------------
    void prepare (double sampleRate);
    void beginBlock (const HatSamplerSettings& settings) noexcept;   // call once per block
    void noteOn (int midiNote, int velocity) noexcept;
    void allNotesOff() noexcept;                                     // quick fade
    void render (juce::AudioBuffer<float>& buffer, int startSample, int numSamples) noexcept;
    void endBlock() noexcept;

    int getNumActiveVoices() const noexcept;

private:
    struct Voice
    {
        const HatSample* sample = nullptr;
        double pos = 0.0, increment = 1.0;
        float gain = 0.0f;
        float env = 1.0f, envCoef = 1.0f;
        float fade = 1.0f, fadeStep = 0.0f;     // choke / release fade
        uint32_t age = 0;
        bool active = false;
    };

    void publishSet();

    LockFreeSlot<HatSampleSet> slot;
    std::array<std::shared_ptr<const HatSample>, kNumFactoryHats + 1> slots;
    juce::File customFile;

    // audio thread state
    const HatSampleSet* currentSet = nullptr;
    const HatSampleSet* lastSet = nullptr;
    HatSamplerSettings settings;
    std::array<Voice, kMaxVoices> voices;
    double sampleRate = 44100.0;
    uint32_t ageCounter = 0;
};

} // namespace rk
