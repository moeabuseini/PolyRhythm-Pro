#pragma once
#include "Instrument.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <cstdint>

namespace studio
{
// All lanes repeat on an integer grid measured over two shared cycles.
struct BeatGrid
{
    std::array<int, layerCount> divisions{};
    std::array<float, layerCount> probability{};
    int seed{};
    static constexpr int maxBodies = 32;
    int bodies = 8;
    int passesPerReunion = 8;
    int phraseLength() const noexcept { return passesPerReunion * bodies; }
    // Rotate the first player each pass so recurring groove rests do not
    // always suppress the same nodes when the body count is even.
    int soloBody(int64_t step) const noexcept { return static_cast<int>((step + step / bodies) % bodies); }
    static constexpr const char *grooves[] = {"Even current", "Threefold", "Broken steps", "Braided pulse"};
    static int groove(int seed) { return (seed + 3) % 4; } // The initial seed opens on Even current.
    float accent(int layer, int64_t step) const
    {
        int group = groove(seed) == 1 ? 3 : 4;
        return ((step + layer) % group == 0) ? 1.f : 0.78f;
    }
    static unsigned hash(unsigned x) noexcept
    {
        x ^= x >> 16;
        x *= 0x7feb352du;
        x ^= x >> 15;
        x *= 0x846ca68bu;
        return x ^ (x >> 16);
    }
    unsigned key(int layer, int64_t step) const noexcept
    {
        return static_cast<unsigned>(seed) + static_cast<unsigned>(step) * 2654435761u + layer * 173u;
    }
    bool sounds(int layer, int64_t step) const noexcept
    {
        if (probability[layer] <= 0)
            return false;
        // Keep phrase downbeats and the regular common pulse audible despite chance rests.
        if (step % phraseLength() == 0 || step % (4 * divisions[layer]) == 0)
            return true;
        if (layer == 0 || layer == 1)
        {
            const auto beat = step + 2 * layer;
            switch (groove(seed))
            {
            case 1:
                if (beat % 6 == 5)
                    return false;
                break;
            case 2:
                if (beat % 4 == 1)
                    return false;
                break;
            case 3:
                if (beat % 8 != 0 && beat % 8 != 3 && beat % 8 != 6)
                    return false;
                break;
            default:
                break;
            }
        }
        return (hash(key(layer, step)) >> 8) * (100.f / 16777216.f) < probability[layer];
    }
    unsigned allBodiesMask() const noexcept { return ~0u >> (32 - bodies); }
    unsigned attackMask(int layer, int64_t step) const noexcept
    {
        if (!sounds(layer, step))
            return 0;
        return step % phraseLength() == 0 ? allBodiesMask() : (1u << soloBody(step));
    }
    struct Flight { double phase, cycles; };
    Flight flight(double cycles, int layer, int body) const noexcept
    {
        // Use absolute steps: chance and groove decisions can differ between phrases.
        // Both searches terminate at the nearest guaranteed sounding phrase boundary.
        if (probability[layer] <= 0)
            return {0.5, 0};
        const double pos = std::max(0., cycles) * divisions[layer] * 0.5;
        auto before = static_cast<int64_t>(std::floor(pos));
        auto after = before + 1;
        const unsigned bit = 1u << body;
        while ((attackMask(layer, before) & bit) == 0)
            --before;
        while ((attackMask(layer, after) & bit) == 0)
            ++after;
        return {(pos - static_cast<double>(before)) / static_cast<double>(after - before),
                2.0 * (after - before) / divisions[layer]};
    }
    double orbitPhase(double cycles, int layer, int body) const noexcept
    {
        return flight(cycles, layer, body).phase;
    }
    bool isFullReunion(int layer, int64_t step) const noexcept
    {
        // Step zero is the starting pose, not a reunion. Compare integer
        // ticks so a nearby attack never counts as a simultaneous one.
        if (step <= 0 || (bodies > 1 && step % phraseLength() != 0))
            return false;
        int active = 0;
        for (int other = 0; other < layerCount; ++other)
        {
            if (probability[other] <= 0) continue;
            ++active;
            const auto tick = step * divisions[other];
            if (tick % divisions[layer] != 0 ||
                attackMask(other, tick / divisions[layer]) != allBodiesMask())
                return false;
        }
        return active * bodies > 1;
    }
    double nextMeeting(double now, bool allBodies = false) const noexcept
    {
        int common = 0;
        for (int i = 0; i < layerCount; ++i)
        {
            if (probability[i] <= 0)
                continue;
            common = std::gcd(common, divisions[i]);
        }
        if (common == 0)
            return -1;
        if (allBodies && bodies > 1)
        {
            double period = 2.0 * phraseLength() / common;
            return (std::floor(now / period + 1.e-9) + 1) * period;
        }
        auto tick = static_cast<int64_t>(std::floor(now * common * 0.5 + 1.e-9)) + 1;
        for (;; ++tick)
        {
            bool all = true;
            for (int i = 0; i < layerCount; ++i)
                if (probability[i] > 0)
                    all &= sounds(i, tick * (divisions[i] / common));
            if (all)
                return 2.0 * tick / common;
        }
    }
};
} // namespace studio
