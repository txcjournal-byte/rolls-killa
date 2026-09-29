#include "SmokeFx.h"

#include <algorithm>
#include <cmath>

namespace rk
{

namespace
{
    constexpr float kPi = 3.14159265358979f;
    constexpr float kInhaleEnd = 0.72f;     // pull for 72 % of the cycle, then let the smoke out
}

void SmokeFx::prepare (double sr) noexcept
{
    sampleRate = sr > 0.0 ? sr : 44100.0;
    reset();
}

void SmokeFx::reset() noexcept
{
    smoothedAmount = smoothedActive = 0.0f;
    lpL = lpR = 0.0f;
    popEnv = popGain = lastNoise = 0.0f;
    bpLow = bpBand = 0.0f;
    glow.store (0.0f, std::memory_order_relaxed);
}

float SmokeFx::inhaleCurve (double phase) noexcept
{
    const auto p = (float) (phase - std::floor (phase));
    if (p < kInhaleEnd)
    {
        const auto s = std::sin (0.5f * kPi * p / kInhaleEnd);
        return s * s;
    }
    const auto out = 1.0f - (p - kInhaleEnd) / (1.0f - kInhaleEnd);
    return out * out;
}

float SmokeFx::nextNoise() noexcept
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return (float) (rng & 0xffffff) / (float) 0x7fffff - 1.0f;
}

void SmokeFx::process (float* left, float* right, int numSamples, float amount,
                       double ppq, double beatsPerSample, bool active) noexcept
{
    amount = std::clamp (amount, 0.0f, 1.0f);
    if (amount <= 0.0f && smoothedAmount < 1.0e-4f)
    {
        smoothedAmount = 0.0f;
        lpL = lpR = 0.0f;
        glow.store (0.0f, std::memory_order_relaxed);
        return;
    }

    const auto sr = (float) sampleRate;
    const auto smooth = 1.0f - std::exp (-1.0f / (0.03f * sr));     // 30 ms parameter smoothing
    const auto activeTarget = active ? 1.0f : 0.0f;
    float lastEnv = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        smoothedAmount += (amount - smoothedAmount) * smooth;
        smoothedActive += (activeTarget - smoothedActive) * smooth;
        const auto a = smoothedAmount;

        const auto env = inhaleCurve ((ppq + i * beatsPerSample) / kCycleBeats);
        lastEnv = env;

        // ---- smoke on the hats: low-pass opens while inhaling, closes on the exhale
        const auto cutoff = 17000.0f * std::pow (0.1f, a * (1.0f - 0.75f * env));
        const auto k = 1.0f - std::exp (-2.0f * kPi * std::min (cutoff, 0.45f * sr) / sr);
        const auto drive = 1.0f + 2.5f * a;

        auto shape = [&] (float x, float& lp)
        {
            lp += (x - lp) * k;
            return std::tanh (lp * drive) / drive * (1.0f + 0.6f * a);
        };

        // ---- crackle: short pops of highpassed noise, more of them while pulling
        const auto popsPerSecond = a * (3.0f + 38.0f * env) * smoothedActive;
        if (popEnv < 0.02f && (float) (rng & 0xffff) / 65536.0f < popsPerSecond / sr * 8.0f)
        {
            const auto r = 0.5f + 0.5f * nextNoise();
            popEnv = 1.0f;
            popGain = (0.2f + 0.8f * r * r) * 0.16f;
            popDecay = std::exp (-1.0f / ((0.0006f + 0.0035f * (0.5f + 0.5f * nextNoise())) * sr));
        }
        const auto n = nextNoise();
        const auto hp = n - lastNoise;
        lastNoise = n;
        const auto crackle = hp * popEnv * popGain * a;
        popEnv *= popDecay;

        // ---- breath: band-passed noise sweeping up while inhaling (state variable filter)
        const auto f = 2.0f * std::sin (kPi * std::min (500.0f + 3200.0f * env, 0.2f * sr) / sr);
        const auto high = n - bpLow - 0.9f * bpBand;
        bpBand += f * high;
        bpLow += f * bpBand;
        const auto inhaling = std::fmod ((ppq + i * beatsPerSample) / kCycleBeats, 1.0) < kInhaleEnd ? 1.0f : 0.3f;
        const auto breath = bpBand * 0.045f * a * a * env * inhaling * smoothedActive;

        const auto add = crackle + breath;
        left[i] = shape (left[i], lpL) + add;
        if (right != nullptr)
            right[i] = shape (right[i], lpR) + add;
    }

    glow.store (smoothedAmount * (0.35f + 0.65f * lastEnv * std::max (0.3f, smoothedActive)), std::memory_order_relaxed);
}

} // namespace rk
