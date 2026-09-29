#pragma once

#include <atomic>
#include <cstdint>

namespace rk
{

/**
    PUFF - the blunt effect of Rolls Killa Mini. Synced to the beat, one "pull" every 2 beats:
      - inhale: a quiet breath (band-passed noise sweeping up) and the ember crackling harder,
      - smoke: the hats go through a low-pass that opens while inhaling and closes on the exhale,
        plus soft saturation (hazy, squashed transients),
      - crackle: random short pops of burning weed.
    amount 0 = bypass (the buffer is not touched). Real-time safe: no allocation, no locks.
*/
class SmokeFx
{
public:
    static constexpr double kCycleBeats = 2.0;

    void prepare (double sampleRate) noexcept;
    void reset() noexcept;

    /** amount 0..1; ppq = position of the first sample; active = the pattern is sounding
        (crackle and breath only while it plays, the smoke filter always follows 'amount'). */
    void process (float* left, float* right, int numSamples, float amount,
                  double ppq, double beatsPerSample, bool active) noexcept;

    /** 0..1: how hard the ember glows right now (UI). */
    float getGlow() const noexcept { return glow.load (std::memory_order_relaxed); }

    /** Inhale envelope over one cycle (0..1 phase) - 0 at the start, 1 at the end of the pull. */
    static float inhaleCurve (double phase) noexcept;

private:
    float nextNoise() noexcept;

    double sampleRate = 44100.0;
    float smoothedAmount = 0.0f;
    float smoothedActive = 0.0f;
    float lpL = 0.0f, lpR = 0.0f;
    float popEnv = 0.0f, popDecay = 0.0f, popGain = 0.0f, lastNoise = 0.0f;
    float bpLow = 0.0f, bpBand = 0.0f;
    std::uint32_t rng = 0x1234567u;
    std::atomic<float> glow { 0.0f };
};

} // namespace rk
