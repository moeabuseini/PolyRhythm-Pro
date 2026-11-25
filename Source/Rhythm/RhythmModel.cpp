#include "RhythmModel.h"

#include <algorithm>
#include <cmath>

double RhythmPattern::cycleSeconds() const noexcept
{
    return static_cast<double>(voiceB) * 60.0 / bpm;
}

double RhythmPattern::phaseAt(double elapsedSeconds) const noexcept
{
    const auto cycle = cycleSeconds();
    if (cycle <= 0.0)
        return 0.0;
    return std::fmod(std::max(0.0, elapsedSeconds), cycle) / cycle;
}

RhythmPattern RhythmPattern::preset(int a, int b, double tempo) noexcept
{
    return { a, b, tempo };
}

std::string rhythmExplanation(const RhythmPattern& pattern)
{
    return std::to_string(pattern.voiceA) + " hits in A during the time of "
        + std::to_string(pattern.voiceB) + " hits in B.";
}
