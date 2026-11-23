#pragma once

#include <JuceHeader.h>
#include <functional>
#include "RhythmModel.h"
#include "LayerInspector.h"
#include "TransportAudioEngine.h"
#include "GlassTheme.h"
#include "SettingsSheet.h"
#include "VideoEditor.h"

class MainComponent final : public juce::Component,
                            public juce::AudioSource,
                            private juce::Timer
{
public:
    explicit MainComponent(juce::File settingsFolder = studioFolder());
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

private:
    void timerCallback() override;
    void animateStage();
    std::unique_ptr<juce::VBlankAttachment> displaySync;
    double lastVblankMs=-1000;
    std::vector<studio::Hit> displayHits;
    void updatePatternFromControls();
    void syncTransportLabel();
    void jumpTo(double cycles);
    void generateRhythm();
    void makeCatalogue();
    void recall(const StudioState&, bool restart);
    void randomize(bool everything);
    void showSettings(bool defaults);
    void showPresets();
    void savePreset();
    StudioState factoryPatch(int index);
    void chooseStartupPreset();
    void loadFactory(int index);
    void toggleFullscreen();
    void menuFor(int);
    void openVideo();
    void previewVideo();
    void updateVideo(double elapsed,bool reunion);
    void finishVideo();
    void populateVideos();
    std::unique_ptr<VideoEditor> video;
    std::vector<StudioState> sequences;
    struct LiveCue {size_t index; int frame,length; double from,to; bool complete=false;};
    std::vector<LiveCue> timeline;
    struct Narration { double start, end; juce::String text; };
    std::vector<Narration> narration;
    bool videoReunion = false;
    StudioState beforeVideo;
    double beforePosition=0, videoEnd=0, reunionEnd=-1;
    bool filming=false, wasPlaying=false;
    juce::TextButton createVideo{"Create video"};
    void warnRoll();
    void customRhythm();
    void managePreset(juce::File);
    void checkpoint();
    void travelHistory(int);
    void mouseDown(const juce::MouseEvent&) override;
    juce::Component::SafePointer<juce::Slider> lastSlider;
    std::vector<StudioState> history;
    int historyIndex=-1;
    bool warn=true;
    std::array<juce::TextButton,4> menus;
    void configureButton(juce::TextButton& button, bool primary = false);
    void styleSlider(juce::Slider& slider, const juce::Colour& accent);
    void styleLabel(juce::Label& label, const juce::String& text, float size, bool muted = false);
    void paintCard(juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour fill = juce::Colour(0xff18212b));
    void showAudioError(const juce::String& errorMessage);

    class ErrorReportingAudioSourcePlayer final : public juce::AudioSourcePlayer
    {
    public:
        void audioDeviceError(const juce::String& message) override
        {
            if (errorHandler)
                errorHandler(message);
        }
        std::function<void(const juce::String&)> errorHandler;
    };

    TransportAudioEngine engine;
    juce::AudioDeviceManager audioDeviceManager;
    ErrorReportingAudioSourcePlayer audioSourcePlayer;
    RhythmPattern pattern;
    RhythmScene scene;
    LayerInspector inspector { engine, scene };
    juce::Viewport inspectorView;
    DockGrip sideGrip{true}, bottomGrip{false};
    int sideWidth=374, dockHeight=166;
    juce::TextButton presets{"Presets"}, fullscreen{"Full screen"}, track{"Note track"};
    juce::TextButton rollAll{"Randomize all"}, rollCustom{"Custom roll..."}, defaultsButton{"Defaults..."};
    std::vector<StudioSetting> catalogue;
    std::vector<RandomRule> recipe;
    StudioState factory, defaults;
    const juce::File userSettings;
    std::unique_ptr<SettingsSheet> sheet;
    juce::TextButton variation { "Random rhythm" }, back { "-5 s" }, forward { "+5 s" }, align { "Next alignment" };
    juce::Label title, subtitle, status, ratioTitle, explanation, tempoLabel, bpmValue,
                voiceALabel, voiceBLabel, cycleLabel, volumeLabel, presetLabel, transposeLabel;
    juce::ComboBox voiceABox, voiceBBox, presetBox, environmentBox, targetBox, meetingBox;
    std::array<juce::ToggleButton,studio::layerCount> layerButtons;
    juce::Slider tempoSlider, volumeSlider, transposeSlider;
    juce::TextButton playButton { "PLAY" }, resetButton { "RESET" };
    juce::Colour background { 0xff0c1117 };
    juce::Colour panel { 0xff141c25 };
    juce::Colour textPrimary { 0xffedf3f7 };
    juce::Colour textMuted { 0xff8b9aa8 };
    juce::Colour accentA { 0xff59e0d0 };
    juce::Colour accentB { 0xffffa66d };
    bool audioErrorVisible = false;
    bool audioCallbackRegistered = false;
    juce::String audioError;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
