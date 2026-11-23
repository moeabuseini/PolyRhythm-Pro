#pragma once
#include <JuceHeader.h>
#include "Instrument.h"
#include "BeatGrid.h"
#include "RhythmModel.h"

class RhythmScene final : public juce::Component
{
  public:
    void setPattern(RhythmPattern p) { pattern = p; }
    void setPointTarget(bool value)
    {
        pointTarget = value;
        repaint();
    }
    void setEnvironment(int index);
    void update(double elapsed, double position);
    void strike(studio::Hit hit);
    void reset()
    {
        count = cursor = 0;
        reunionTime = -10;
        laneHit.fill(-10);
        update(0, 0);
    }
    void paint(juce::Graphics &) override;
    static constexpr int maxStars = 6000;
    static constexpr size_t twinkleSetting = 6;
    int activeEnvironment() const { return environment; }
    std::array<float, 7> &settings() { return looks[environment]; }
    auto& settingsFor(int index) { return looks[index]; }
    bool showForecast = true;
    bool showFps=false;
    bool cacheSky=true;
    float trackHeight=80;
    std::array<double,studio::layerCount> laneHit{{-10,-10,-10,-10,-10}};
    bool equalSpeed = false, autoBalance = true, cleanStage = false;
    std::array<float,studio::layerCount> linePlacement{}, pointPlacement{};
    std::vector<std::pair<juce::String,float>> captions;
    void setGrid(studio::BeatGrid next)
    {
        if (grid.bodies != next.bodies) count = cursor = 0;
        grid = next;
    }
    struct SceneMotion {
        bool hueCycle = false;
        int hueSpeed = 1, camera = 1; // Camera: 0 fixed, 1 drift, 2 pointer
        float hueRange = 45, depth = 35;
    };
    SceneMotion& motion() { return sceneMotion[environment]; }
    SceneMotion& motionFor(int index) { return sceneMotion[index]; }
    bool usesPoint() const { return pointTarget; }
    float orbitScale = 100, pointCoverage = 100;
    float timingOffset = 0, fanSpread = 65;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent&) override { pointerTarget = {}; }
    static constexpr const char *details[3][2] = {
        {"Corona reach", "Lunar etching"}, {"Curtain lift", "Silk folds"}, {"Tide height", "Moon path"}};

  private:
    double fpsStart=0;
    int fpsFrames=0, measuredFps=0;
    std::array<juce::Path,studio::layerCount*studio::BeatGrid::maxBodies> orbitInk;
    std::array<int,6> orbitKey{};
    float orbitSpread=-1;
    std::array<float,studio::layerCount> orbitPlacement{};
    void paintSky(juce::Graphics&, juce::Rectangle<float>);
    juce::Image skyFrame;
    juce::Rectangle<int> skySize;
    double nextSkyMs=0, skyReunion=-10,skyTime=0;
    int skyEnvironment=-1;
    juce::Point<float> point(double cycles, int layer, int strand) const;
    juce::Point<float> pathPoint(float progress, int layer, int body, float radius = 0) const;
    void paintTarget(juce::Graphics &, juce::Rectangle<float>) const;
    void paintAtmosphere(juce::Graphics &, juce::Rectangle<float>);
    float travelProgress(float phaseOnPath,int strand) const;
    float flightRadius(double duration) const;
    void buildBackdrop();
    void paintMoon(juce::Graphics&, juce::Point<float>, float);
    void paintMountains(juce::Graphics&, juce::Rectangle<float>, float);
    std::array<SceneMotion,3> sceneMotion;
    juce::Image lunarSurface;
    std::array<juce::Image,4> ridges;
    juce::Point<float> cameraOffset, pointerTarget, pointerSmooth;
    juce::Image hueBackdrop;
    float hueBlend = 0, cachedRange = -1;
    double hueClock = 0, cameraClock = 0;
    void paintStars(juce::Graphics &, juce::Rectangle<float>);
    juce::Image distantStars;
    int cachedStars = -1;

    void paintForecast(juce::Graphics &, juce::Rectangle<float>) const;
    std::array<std::array<float, 7>, 3> looks{
        {{225, 720, 80, 55, 65, 75, 48}, {174, 550, 70, 64, 70, 54, 34}, {217, 680, 75, 48, 56, 68, 42}}};
    studio::BeatGrid grid{{6, 4, 2, 1, 4}, {100, 100, 100, 100, 0}, 1709};
    juce::Image backdrop;
    int environment = 0, cachedHue = -1;
    bool pointTarget = false;
    RhythmPattern pattern;
    std::array<studio::Hit, 256> history{};
    int count = 0, cursor = 0;
    double time = 0, phase = 0, reunionTime = -10;
};
