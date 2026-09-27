#pragma once

#include <cstdint>
#include <vector>

namespace rk
{

/** Small deterministic RNG (splitmix64). Same seed -> same numbers on every platform/compiler. */
class Rng
{
public:
    explicit Rng (uint64_t seed) noexcept : state (seed * 0x9E3779B97F4A7C15ull + 0x632BE59BD9B4E019ull) {}

    uint64_t next() noexcept
    {
        uint64_t z = (state += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }

    double uniform() noexcept { return (double) (next() >> 11) * (1.0 / 9007199254740992.0); }
    bool chance (double p) noexcept { return uniform() < p; }
    int range (int lo, int hiInclusive) noexcept { return lo + (int) (next() % (uint64_t) (hiInclusive - lo + 1)); }

    template <typename Weights>
    int weighted (const Weights& w) noexcept
    {
        double total = 0.0;
        for (auto x : w) total += (double) x;
        if (total <= 0.0) return 0;
        auto r = uniform() * total;
        int i = 0;
        for (auto x : w)
        {
            if (r < (double) x) return i;
            r -= (double) x;
            ++i;
        }
        return i - 1;
    }

    template <typename T>
    void shuffle (std::vector<T>& v) noexcept
    {
        for (size_t i = v.size(); i > 1; --i)
            std::swap (v[i - 1], v[(size_t) (next() % i)]);
    }

private:
    uint64_t state;
};

/** Stateless hash -> [0, 1), used for per-note humanization that must not depend on note order. */
inline double hash01 (uint64_t a, uint64_t b) noexcept
{
    Rng r (a * 0x100000001B3ull ^ (b + 0x51ED27ull));
    return r.uniform();
}

} // namespace rk
