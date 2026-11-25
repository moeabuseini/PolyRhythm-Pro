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

inline constexpr std::array<RhythmPreset, 24> rhythmPresets {{
    { "3 : 2", 3, 2 },
    { "4 : 3", 4, 3 },
    { "5 : 4", 5, 4 },
    { "2 : 3", 2, 3 },
    { "5 : 2", 5, 2 },
    { "5 : 3", 5, 3 },
    { "6 : 5", 6, 5 },
    { "7 : 2", 7, 2 },
    { "7 : 3", 7, 3 },
    { "7 : 4", 7, 4 },
    { "7 : 5", 7, 5 },
    { "7 : 6", 7, 6 },
    { "8 : 3", 8, 3 },
    { "8 : 5", 8, 5 },
    { "8 : 7", 8, 7 },
    { "9 : 2", 9, 2 },
    { "9 : 4", 9, 4 },
    { "9 : 5", 9, 5 },
    { "9 : 7", 9, 7 },
    { "9 : 8", 9, 8 },
    { "4 : 4", 4, 4 },
    { "6 : 4", 6, 4 },
    { "3 : 5", 3, 5 },
    { "4 : 7", 4, 7 }
}};

std::string rhythmExplanation(const RhythmPattern& pattern);
