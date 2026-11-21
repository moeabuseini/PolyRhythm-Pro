#pragma once

#include <JuceHeader.h>
#include "RhythmModel.h"

class RhythmScene final : public juce::Component
{
public:
    void setPattern(RhythmPattern pattern) noexcept { currentPattern = pattern; repaint(); }
    void setTransport(double phase, int cycle, bool active) noexcept;
    void paint(juce::Graphics& g) override;

private:
    RhythmPattern currentPattern;
    double transportPhase = 0.0;
    int currentCycle = 0;
    bool isActive = false;
};
