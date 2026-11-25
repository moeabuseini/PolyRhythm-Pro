#pragma once
#include <JuceHeader.h>

// IDs identify the actual catalogue settings, including the active layer or sky.
class PerformanceSlider final : public juce::Slider
{
public:
    juce::StringArray resetKeys, appearanceKeys;
    void mouseDown(const juce::MouseEvent& e) override
    {
        contextGesture=e.mods.isPopupMenu();
        if(!contextGesture) Slider::mouseDown(e);
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if(!contextGesture) Slider::mouseDrag(e);
    }
    void mouseUp(const juce::MouseEvent& e) override
    {
        if(!contextGesture) Slider::mouseUp(e);
        contextGesture=false;
    }
private:
    bool contextGesture=false;
};
