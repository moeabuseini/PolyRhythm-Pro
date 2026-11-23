#pragma once
#include <JuceHeader.h>
#include <vector>
#include <functional>
#include <cmath>

// A single catalogue drives capture, recall and constrained randomisation.
// Stored IDs are independent of the text displayed in the interface.
struct StudioSetting
{
    juce::String id, section, label, unit;
    double low, high, step;
    juce::StringArray choices;
    std::function<double()> read;
    std::function<void(double)> write;

    double snap(double x) const
    {
        return juce::jlimit(low, high, low + step * std::round((juce::jlimit(low, high, x) - low) / step));
    }
    double draw(juce::Random& rng, double from, double to) const
    {
        int first = juce::roundToInt((snap(from) - low) / step);
        int last = juce::roundToInt((snap(to) - low) / step);
        return snap(low + step * (first + rng.nextInt(1 + last - first)));
    }
};
struct RandomRule { bool enabled{}; double low{}, high{}; unsigned allowed=~0u; };
using StudioState = juce::ValueTree;
inline StudioState captureStudio(const std::vector<StudioSetting>& catalogue)
{
    StudioState result("Patch");
    for (const auto& setting : catalogue)
        result.setProperty(setting.id, setting.read(), nullptr);
    return result;
}
inline juce::File studioFolder()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("PolyRhythm Pro");
}
inline bool storeStudioFile(const juce::File& file, const StudioState& state)
{
    if (file.getParentDirectory().createDirectory().failed()) return false;
    juce::TemporaryFile output(file);
    auto xml = state.createXml();
    return xml != nullptr && xml->writeTo(output.getFile()) && output.overwriteTargetFileWithTemporary();
}
inline StudioState readStudioFile(const juce::File& file)
{
    if (auto xml = juce::parseXML(file)) return StudioState::fromXml(*xml);
    return {};
}
