#pragma once

#include "DrumSynth.h"
#include "LockFreeSlot.h"

#include <array>
#include <atomic>

namespace rk
{

/** The sounds of the eight drawers, handed to the audio thread as one immutable set. */
struct KitSoundSet
{
    std::array<DrumSoundPtr, kNumDrumTypes> sounds;
};

/**
    Plays the kit drawers (the hi-hat rolls keep their own HatSampler). Real-time safe: noteOn/render never
    allocate or lock. Chokes: 808, kick and open hat are monophonic (a new hit fades the last one in ~3 ms),
    a closed hat chokes the open hat (call choke (openHat)). Pitch per note like a sampler (note - root).
*/
class KitSampler
{
public:
    static constexpr int kMaxVoices = 32;

    KitSampler();

    // ---- message thread ----
    void setSound (DrumType type, DrumSoundPtr sound);
    DrumSoundPtr getSound (DrumType type) const { return sounds[(size_t) type]; }
    void setGain (DrumType type, float gainDb, bool muted) noexcept;

    // ---- audio thread ----
    void prepare (double sampleRate);
    void beginBlock() noexcept;
    void noteOn (int typeIndex, int midiNote, int velocity) noexcept;
    void choke (int typeIndex) noexcept;
    /** Plays the drawer's sound at its own pitch (pad click). */
    void audition (int typeIndex, int velocity) noexcept;
    void allNotesOff() noexcept;
    void render (juce::AudioBuffer<float>& buffer, int startSample, int numSamples) noexcept;
    void endBlock() noexcept;
    int getNumActiveVoices() const noexcept;

private:
    struct Voice
    {
        const DrumSound* sound = nullptr;
        int type = 0;
        double pos = 0.0, increment = 1.0;
        float gain = 0.0f, fade = 1.0f, fadeStep = 0.0f;
        uint32_t age = 0;
        bool active = false;
    };

    void startFade (Voice& v) noexcept;

    LockFreeSlot<KitSoundSet> slot;
    std::array<DrumSoundPtr, kNumDrumTypes> sounds;
    std::array<std::atomic<float>, kNumDrumTypes> gains {};

    const KitSoundSet* currentSet = nullptr;
    const KitSoundSet* lastSet = nullptr;
    std::array<Voice, kMaxVoices> voices;
    double sampleRate = 44100.0;
    uint32_t ageCounter = 0;
};

} // namespace rk
