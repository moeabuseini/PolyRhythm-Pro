#pragma once
#include "BeatGrid.h"

namespace studio
{
enum PitchMode { ascending, descending, seededOrder, ensemble };
enum ChordShape { seventh, triad, firstInversion, suspendedSecond, fourths };
enum ChordTouch { single, block, rolled };

// Scale degrees (zero-based), not fixed semitone offsets: every shape
// stays in the score's mode, even when the seed changes its root.
inline int chordDegree(int index, int shape)
{
    constexpr int shapes[][4] = {{0,2,4,6}, {0,2,4,0}, {2,4,7,0}, {0,1,4,0}, {0,3,6,0}};
    const int size = shape == seventh ? 4 : 3;
    return shapes[shape][index % size] + 7 * (index / size);
}

inline int scaleInterval(int degree)
{
    constexpr int dorian[]{0, 2, 3, 5, 7, 9, 10};
    return dorian[degree % 7] + 12 * (degree / 7);
}

inline int nodeRoot(int seed) { return static_cast<int>(BeatGrid::hash(static_cast<unsigned>(seed)) % 4); }

// A node's pitch depends on its identity and the controls, never on its hit
// number. The pitch map in the inspector and the synthesizer share this code.
inline int nodePitch(int layer, int body, int count, int copies, int mode, int seed, int transpose,
                     int shape = seventh, int companion = 0)
{
    constexpr int registers[]{62, 38, 50, 62, 62};
    constexpr int bassDegrees[]{0, 2, 3, 4, 6};
    const int groups = (count + copies - 1) / copies;
    int slot = body / copies;
    const auto key = BeatGrid::hash(static_cast<unsigned>(seed));
    if (mode == descending) slot = groups - 1 - slot;
    if (mode == seededOrder)
    {
        int stride = 1 + static_cast<int>(key % groups);
        while (std::gcd(stride, groups) != 1) ++stride;
        slot = (slot * stride + static_cast<int>((key >> 8) % groups)) % groups;
    }
    auto interval = [&](int index)
    {
        int degree = index;
        if (layer == 2) degree = chordDegree(index, shape);
        if (layer == 1) degree = bassDegrees[index % 5] + 7 * (index / 5);
        return scaleInterval(degree + nodeRoot(seed) + (layer == 3 ? 4 : 0));
    };
    int base = registers[layer] + transpose;
    // Move the whole voicing by octaves at the register limits. Clipping
    // individual pitches here would silently duplicate the highest nodes.
    while (base + interval(groups - 1) > 105) base -= 12;
    while (base + interval(0) < 21) base += 12;
    int note = base + interval(slot + companion);
    while (note > 105) note -= 12;
    return note;
}
}
