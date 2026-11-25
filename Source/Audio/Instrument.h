#pragma once
#include <array>
#include <atomic>
#include <cstdint>

namespace studio
{
enum Parameter
{
    gain,
    pitch,
    octave,
    rate,
    chance,
    decay,
    cutoff,
    lowEQ,
    highEQ,
    room,
    wet,
    delay,
    feedback,
    chorus,
    modulation,
    cloud,
    grainDensity,
    grainRange,
    grainLength,
    shimmer,
    reverse,
    damping,
    stereoWidth,
    chorusDepth,
    chorusFeedback,
    chorusCentre,
    parameterCount
};
struct Dial
{
    const char *name;
    float min, max, step, initial;
    const char *unit;
};
inline constexpr std::array<Dial, parameterCount> dials{{{"Gain", 0, 100, 1, 75, "%"},
                                                         {"Pitch", -12, 12, 1, 0, " st"},
                                                         {"Octave", -2, 2, 1, 0, ""},
                                                         {"Multiplier", 1, 4, 1, 1, "x"},
                                                         {"Note chance", 0, 100, 1, 100, "%"},
                                                         {"Release", 0.15f, 5, 0.05f, 1.5f, " s"},
                                                         {"Low-pass", 150, 16000, 1, 9000, " Hz"},
                                                         {"Low shelf", -12, 12, 0.5f, 0, " dB"},
                                                         {"High shelf", -12, 12, 0.5f, 0, " dB"},
                                                         {"Room size", 0, 100, 1, 72, "%"},
                                                         {"Reverb", 0, 100, 1, 28, "%"},
                                                         {"Echo", 0, 100, 1, 18, "%"},
                                                         {"Feedback", 0, 85, 1, 36, "%"},
                                                         {"Chorus", 0, 100, 1, 12, "%"},
                                                         {"Mod rate", 0.1f, 4, 0.1f, 0.4f, " Hz"},
                                                         {"Cloud mix", 0, 100, 1, 0, "%"},
                                                         {"Grains/sec", 1, 24, 1, 8, ""},
                                                         {"Pitch range", 0, 12, 1, 5, " st"},
                                                         {"Grain length", 30, 500, 1, 180, " ms"},
                                                         {"Shimmer", 0, 100, 1, 15, "%"},
                                                         {"Reverse", 0, 1, 1, 0, ""},
                                                         {"Damping", 0, 100, 1, 48, "%"},
                                                         {"Room width", 0, 100, 1, 85, "%"},
                                                         {"Chorus depth", 0, 100, 1, 38, "%"},
                                                         {"Chorus return", -75, 75, 1, 8, "%"},
                                                         {"Chorus delay", 2, 48, 0.5f, 14, " ms"}}};
inline constexpr std::array layers{"Melody", "Bass", "Chords", "Atmosphere", "Synth"};
inline constexpr int layerCount = static_cast<int>(layers.size());
inline constexpr std::array<const char *, 14> sounds{
    "Felt piano", "Glass keys",  "Round bass", "Velvet synth", "Air choir",   "Reed bass",
    "Sub sine",   "Copper bass", "Celesta",    "Dusk tine",    "Hollow flute", "Solar brass", "Prism lead", "Titan strings"};
inline constexpr std::array<std::uint32_t, layerCount> colours{0xff9edaff, 0xffffc48f, 0xffbb9dff, 0xff93f6d1, 0xffff9edb};
using Values = std::array<float, parameterCount>;
struct Strip
{
    std::array<std::atomic<float>, parameterCount> values{};
    std::atomic<bool> enabled{true};
    std::atomic<int> sound{0};
    std::atomic<int> pitchMode{0}, nodesPerPitch{1};
    Values snapshot() const noexcept
    {
        Values result{};
        for (size_t i = 0; i < result.size(); ++i)
            result[i] = values[i].load(std::memory_order_relaxed);
        return result;
    }
};
struct Hit
{
    double time, position;
    int layer, note;
    float velocity;
    int body{};
    bool fullReunion{};
};
} // namespace studio
