#pragma once

#include <array>
#include <string>

struct RhythmPattern
{
    int voiceA = 3;
    int voiceB = 2;
    double bpm = 96.0;

    double cycleSeconds() const noexcept;
    double phaseAt(double elapsedSeconds) const noexcept;
    static RhythmPattern preset(int voiceA, int voiceB, double bpm = 96.0) noexcept;
};

struct RhythmPreset
{
    const char* name;
    int voiceA;
    int voiceB;
};

inline constexpr std::array<RhythmPreset, 3> rhythmPresets {{
    { "3 : 2", 3, 2 },
    { "4 : 3", 4, 3 },
    { "5 : 4", 5, 4 }
}};

std::string rhythmExplanation(const RhythmPattern& pattern);
