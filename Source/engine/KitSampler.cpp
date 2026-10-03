#include "KitSampler.h"

#include <cmath>

namespace rk
{

KitSampler::KitSampler()
{
    for (auto& g : gains)
        g.store (1.0f);
}

void KitSampler::setSound (DrumType type, DrumSoundPtr sound)
{
    sounds[(size_t) type] = std::move (sound);
    auto set = std::make_unique<KitSoundSet>();
    set->sounds = sounds;
    slot.publish (std::move (set));
}

void KitSampler::setGain (DrumType type, float gainDb, bool muted) noexcept
{
    gains[(size_t) type].store (muted ? 0.0f : juce::Decibels::decibelsToGain (gainDb), std::memory_order_relaxed);
}

void KitSampler::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    for (auto& v : voices)
        v.active = false;
}

void KitSampler::beginBlock() noexcept
{
    currentSet = slot.acquire();
    if (currentSet != lastSet)
    {
        // a drawer got a new sound: voices still playing the replaced one must stop now
        // (the old set may be freed by the message thread as soon as we release it)
        for (auto& v : voices)
            if (v.active && (currentSet == nullptr || currentSet->sounds[(size_t) v.type].get() != v.sound))
                v.active = false;
        lastSet = currentSet;
    }
}

void KitSampler::endBlock() noexcept
{
    currentSet = nullptr;
    slot.release();
}

void KitSampler::startFade (Voice& v) noexcept
{
    if (v.active && v.fadeStep <= 0.0f)
        v.fadeStep = 1.0f / (float) juce::jmax (1.0, 0.003 * sampleRate);
}

void KitSampler::choke (int typeIndex) noexcept
{
    for (auto& v : voices)
        if (v.type == typeIndex)
            startFade (v);
}

void KitSampler::audition (int typeIndex, int velocity) noexcept
{
    if (currentSet != nullptr && typeIndex >= 0 && typeIndex < kNumDrumTypes)
        if (const auto* s = currentSet->sounds[(size_t) typeIndex].get())
            noteOn (typeIndex, s->rootNote, velocity);
}

void KitSampler::allNotesOff() noexcept
{
    for (auto& v : voices)
        startFade (v);
}

void KitSampler::noteOn (int typeIndex, int midiNote, int velocity) noexcept
{
    if (currentSet == nullptr || velocity <= 0 || typeIndex < 0 || typeIndex >= kNumDrumTypes)
        return;
    const auto* s = currentSet->sounds[(size_t) typeIndex].get();
    if (s == nullptr || s->audio.getNumSamples() == 0)
        return;

    const auto type = (DrumType) typeIndex;
    if (type == DrumType::b808 || type == DrumType::kick || type == DrumType::openHat)
        choke (typeIndex);

    // free voice, else the oldest
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

    auto& v = *target;
    v.sound = s;
    v.type = typeIndex;
    v.pos = 0.0;
    v.increment = s->sampleRate / sampleRate * std::pow (2.0, (midiNote - s->rootNote) / 12.0);
    const auto vel = (float) velocity / 127.0f;
    v.gain = vel * std::sqrt (vel);
    v.fade = 1.0f;
    v.fadeStep = 0.0f;
    v.age = ++ageCounter;
    v.active = true;
}

void KitSampler::render (juce::AudioBuffer<float>& buffer, int startSample, int numSamples) noexcept
{
    if (currentSet == nullptr)
        return;
    const auto outChans = buffer.getNumChannels();
    for (auto& v : voices)
    {
        if (! v.active)
            continue;
        const auto& audio = v.sound->audio;
        const auto len = audio.getNumSamples();
        const auto srcChans = audio.getNumChannels();
        const auto slotGain = gains[(size_t) v.type].load (std::memory_order_relaxed);
        for (int i = 0; i < numSamples; ++i)
        {
            const auto idx = (int) v.pos;
            if (idx >= len - 1 || v.fade <= 0.0f)
            {
                v.active = false;
                break;
            }
            const auto frac = (float) (v.pos - idx);
            const auto g = v.gain * v.fade * slotGain;
            for (int ch = 0; ch < outChans; ++ch)
            {
                const auto* d = audio.getReadPointer (juce::jmin (ch, srcChans - 1));
                buffer.addSample (ch, startSample + i, (d[idx] + (d[idx + 1] - d[idx]) * frac) * g);
            }
            v.pos += v.increment;
            if (v.fadeStep > 0.0f)
                v.fade -= v.fadeStep;
        }
    }
}

int KitSampler::getNumActiveVoices() const noexcept
{
    int n = 0;
    for (const auto& v : voices)
        n += v.active ? 1 : 0;
    return n;
}

} // namespace rk
