#pragma once
#include <JuceHeader.h>
#include "Instrument.h"
#include "BeatGrid.h"
#include "RhythmModel.h"

class RhythmScene final : public juce::Component
{
  public:
    void setPattern(RhythmPattern p) { pattern = p; }
    enum Formation { line, pointEye, horizontal, vertical, horizontalTrio, verticalTrio, infinity, sphere, lotus, nautilus, wings, ripple, heart, helix, square, cube, triangle, pyramid, hexagon, compassStar, crescent, shellFan };
    inline static const juce::StringArray formationNames{"Convergence line","Convergence point","Horizontal rail","Vertical rail","Horizontal trio","Vertical trio","Infinity","Sphere","Lotus","Nautilus","Moth wings","Ripple crown","Heart","DNA helix","Square","Cube","Triangle","Pyramid","Hexagon","Compass star","Crescent","Scallop shell"};
    static bool prefersEqualSpeed(int index)
    {
        return index==pointEye || index==horizontal || index==vertical;
    }
    void selectFormation(int index)
    {
        equalSpeed=prefersEqualSpeed(index);
        setFormation(index);
    }
    void setFormation(int index)
    {
        formation = static_cast<Formation>(juce::jlimit(0, formationNames.size()-1, index));
        pointTarget = formation == pointEye || formation >= infinity;
        repaint();
    }
    void setPointTarget(bool value) { setFormation(value ? pointEye : line); }
    void setEnvironment(int index);
    void update(double elapsed, double position);
    void strike(studio::Hit hit);
    void reset()
    {
        count = cursor = 0;
        headsReady=false;exposureCount=exposureCursor=0;exposing=capturePending=false;exposureEnd=-1;
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
    bool showForecast = true, showSyncThreads = true;
    bool showFps=false;
    bool cacheSky=true;
    float trackHeight=80;
    std::array<double,studio::layerCount> laneHit{{-10,-10,-10,-10,-10}};
    float videoOpacity=1;
    std::array<int,studio::layerCount> captionVoices{};
    bool elasticTrails = true, equalSpeed = false, autoBalance = true, cleanStage = false;
    struct BrushDial {const char* id;const char* label;float low,high,initial;const char* unit;};
    enum BrushIndex { tint, size, length, glow, haloStrength, stroke, brushCount };
    inline static constexpr BrushDial brushDials[brushCount]{
        {"hue","Node hue",-180,180,0," deg"},{"size","Node size",30,300,100,"%"},
        {"trail","Trail length",0,100,18,"%"},{"glow","Trail glow",0,100,0,"%"},
        {"halo","Node halo",0,200,100,"%"},{"stroke","Ring width",50,350,100,"%"}
    };
    using Brush=std::array<float,brushCount>;
    std::array<Brush,studio::layerCount> brushes=[]{
        std::array<Brush,studio::layerCount> result{};
        for(auto& brush:result) for(int k=0;k<brushCount;++k) brush[k]=brushDials[k].initial;
        return result;
    }();
    juce::Colour nodeColour(int layer) const {
        return juce::Colour(studio::colours[layer]).withRotatedHue(brushes[layer][tint]/360.f);
    }
    std::array<float,studio::layerCount> linePlacement{}, pointPlacement{};
    std::vector<std::pair<juce::String,float>> captions;
    void setGrid(studio::BeatGrid next)
    {
        if (grid.bodies != next.bodies) count = cursor = 0;
        if(grid.bassStart!=next.bassStart || grid.bassSeed!=next.bassSeed || grid.melodySwitch!=next.melodySwitch ||
           grid.melodyBefore!=next.melodyBefore || grid.melodyAfter!=next.melodyAfter || grid.bodies!=next.bodies || grid.divisions!=next.divisions || grid.seed!=next.seed ||
           grid.probability!=next.probability || grid.passesPerReunion!=next.passesPerReunion)
            flightWindows.fill({});
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
    std::array<float,studio::layerCount> layerDistance{100,100,100,100,100};
    float orbitScale = 100, pointCoverage = 100;
    float fanBloom=0; // Transient reunion envelope, never serialized as an edit.
    float visibleSpread() const { return fanSpread+(100.f-fanSpread)*fanBloom; }
    float timingOffset = 0, fanSpread = 65;
    int closeShot=2,shotLead=1;
    bool shotCamera=false, portraitSafe=false;
    int pinLayer=-1, pinBody=0, pinPitch=60;
    bool exposing=false, capturePending=false;
    juce::Image exposureStill;
    void beginExposure();
    void endExposure();
    std::function<int(int,int,double)> predictPitch;
    std::function<void(int,int)> onAudition;
    juce::Component::SafePointer<juce::Component> receiptOverlay;
    bool canModalEventBeSentToComponent(const juce::Component* target) override {
        return receiptOverlay && (target==receiptOverlay.getComponent() || receiptOverlay->isParentOf(target));
    }
    void mouseDown(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent&) override { pointerTarget = {}; }
    inline static const juce::StringArray environmentNames{"Eclipse Garden","Aurora Loom","Lunar Tide","Lantern Deep","Saffron Dunes","Glacier Cathedral"};
    static constexpr int environmentCount=6;
    static constexpr const char *details[environmentCount][2] = {
        {"Corona reach", "Lunar etching"}, {"Curtain lift", "Silk folds"}, {"Tide height", "Moon path"},
        {"Lantern shoal", "Ray strength"}, {"Dune relief", "Windblown sand"}, {"Ice shimmer", "Drifting frost"}};

  private:
    double fpsStart=0;
    int fpsFrames=0, measuredFps=0;
    static constexpr int nodeSlots=studio::layerCount*studio::BeatGrid::maxBodies;
    std::array<studio::BeatGrid::FlightWindow,nodeSlots> flightWindows{};
    std::array<juce::Point<float>,nodeSlots> paintedHeads{};
    bool headsReady=false;
    juce::AffineTransform shot;
    double exposureEnd=-1;
    struct ExposurePoint {juce::Point<float> point; int layer; float energy;};
    std::array<ExposurePoint,2048> exposurePoints{};
    int exposureCount=0,exposureCursor=0;
    struct Spark { juce::Point<float> head,tail;float radius;juce::Colour colour; };
    std::vector<Spark> burst;
    juce::Image burstSurface;
    void compositeBurst(juce::Graphics&,juce::Rectangle<float>);
    std::array<int,6> orbitKey{};
    float orbitSpread=-1, orbitCoverage=-1;
    double nextOrbitRefresh=0;
    juce::String styledCaption;
    juce::Point<int> captionSize;
    juce::TextLayout captionLayout;
    std::array<juce::Colour,studio::layerCount> captionPalette{};
    std::array<int,studio::layerCount> styledVoices{};
    std::array<float,studio::layerCount> orbitPlacement{};
    using GuideBounds = std::array<juce::Rectangle<float>,studio::layerCount*studio::BeatGrid::maxBodies>;
    GuideBounds previousGuides{};
    std::array<float,studio::layerCount> guideHues{};
    juce::Image guideImage;
    juce::Rectangle<int> guideViewport;
    void paintSky(juce::Graphics&, juce::Rectangle<float>);
    juce::Image skyFrame;
    juce::Rectangle<int> skySize;
    double nextSkyMs=0, skyReunion=-10,skyTime=0;
    int skyEnvironment=-1;
    juce::Point<float> pathPoint(float progress, int layer, int body, float radius = 0) const;
    void paintTarget(juce::Graphics &, juce::Rectangle<float>) const;
    void paintAtmosphere(juce::Graphics &, juce::Rectangle<float>);
    float travelProgress(float phaseOnPath,int strand) const;
    float flightRadius(double duration,int layer) const;
    void buildBackdrop();
    void paintLanternDeep(juce::Graphics&,float w,float h,float t,float bloom);
    void paintDunes(juce::Graphics&,float w,float h,float t,float bloom);
    void paintMoon(juce::Graphics&, juce::Point<float>, float);
    void paintMountains(juce::Graphics&, juce::Rectangle<float>, float);
    std::array<SceneMotion,environmentCount> sceneMotion;
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
    std::array<std::array<float, 7>, environmentCount> looks{
        {{225, 720, 80, 55, 65, 75, 48}, {174, 550, 70, 64, 70, 54, 34}, {217, 680, 75, 48, 56, 68, 42},
         {197, 420, 72, 48, 67, 68, 40}, {268, 360, 85, 48, 76, 52, 30}, {198, 220, 65, 42, 64, 48, 28}}};
    studio::BeatGrid grid{{6, 4, 2, 1, 4}, {100, 100, 100, 100, 0}, 1709};
    juce::Image backdrop;
    int environment = 0, cachedHue = -1;
    Formation formation = line;
    struct Route {
        juce::Point<float> gate;
        std::array<juce::Point<float>,65> samples;
        std::array<float,65> distance{};
        juce::Path guide;
    };
    std::array<Route,studio::layerCount*studio::BeatGrid::maxBodies> routes;
    void buildRoute(int layer,int body);
    juce::Point<float> routeGate(int rank,float lane) const;
    bool pointTarget = false;
    RhythmPattern pattern;
    std::array<studio::Hit, 256> history{};
    std::array<int,256> threadPredecessors{};
    int linkedLayers=-1,linkedCount=-1;
    void refreshThreadLinks();
    int count = 0, cursor = 0;
    double time = 0, phase = 0, reunionTime = -10;
};
