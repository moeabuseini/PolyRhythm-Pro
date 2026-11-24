#pragma once

#include <JuceHeader.h>
#include <functional>
#include "RhythmModel.h"
#include "LayerInspector.h"
#include "TransportAudioEngine.h"
#include "GlassTheme.h"
#include "SettingsSheet.h"
#include "VideoEditor.h"
#include "SaveReceipt.h"

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
    void transferPreset(bool exporting,bool sequence);
    std::unique_ptr<juce::FileChooser> transferChooser;
    void savePreset();
    StudioState factoryPatch(int index);
    void chooseStartupPreset();
    void loadFactory(int index);
    void toggleFullscreen();
    void menuFor(int);
    juce::TextButton producer{"Producer"};
    void initProducer();
    void refreshProducer();
    bool actionEnabled(int) const;
    juce::ApplicationCommandManager commands;
    std::unique_ptr<juce::DialogWindow> shortcutDialog;
    void showShortcuts();
    SaveReceipt receipt;
    void saved(const juce::String&,const juce::File&);
    void captured(const juce::String&,const juce::String&);
    void placeReceipt();
    StudioState priorMix;
    std::array<bool,2> storedMix{};
    bool fanReturns=true,automationStartedTransport=false;
    void cancelFan();
    void producerAction(int);
    void catchMidi(int window);
    void exportStems();
    juce::ThreadPool renderPool{1};
    void mixSlot(int slot,bool store);
    void editRoadmap();
    int heldExposureKey=0,lastShortcutKey=0;
    bool keyStateChanged(bool) override;
    void sampleGesture();
    void finishGesture();
    void notifyProducer(const juce::String& text);
    juce::File exports() const { return userSettings.getChildFile("Exports"); }
    StudioState gesture;
    double gestureStart=0, fanLastCycle=-1, fanBeats=0;
    std::vector<double> gestureValues;
    std::array<int,3> variants{{-1,-1,-1}};
    bool fanCue=false;
    void openVideo();
    void previewVideo();
    void updateVideo(double elapsed,bool reunion);
    void finishVideo();
    void populateVideos();
    std::unique_ptr<VideoEditor> video;
    std::vector<StudioState> sequences;
    struct LiveCue {size_t index; int frame,length; double from,to;};
    std::vector<LiveCue> timeline;
    size_t nextVideoCue=0;
    std::vector<size_t> fadingCues;
    struct Narration { double start, end; juce::String text; };
    std::vector<Narration> narration;
    bool videoReunion = false;
    StudioState beforeVideo,videoOpening,videoPeak;
    double openingPhase=0,videoClockOffset=0,returnStart=-1;
    bool returnToOpening=false,loopVideo=false;
    void restartVideoPass();
    void completeVideoPass();
    double beforePosition=0, videoEnd=0, reunionEnd=-1;
    bool filming=false, wasPlaying=false;
    juce::TextButton createVideo{"Create video"};
    void warnRoll();
    void customRhythm();
    void managePreset(juce::File);
    void checkpoint();
    void travelHistory(int);
    void resetPerformanceControls(const juce::StringArray& keys);
    void mouseDown(const juce::MouseEvent&) override;
    juce::Component::SafePointer<juce::Slider> lastSlider;
    std::vector<StudioState> history;
    int historyIndex=-1;
    std::vector<double> currentHistoryValues;
    void indexCurrentHistory();
    bool warn=true;
    std::array<juce::TextButton,4> menus;
    void configureButton(juce::TextButton& button);
    void addAppearanceMenu(juce::PopupMenu&);
    void applyAppearance(const Appearance&);
    void saveAppearance(const Appearance&);
    void styleSlider(juce::Slider& slider);
    void styleLabel(juce::Label& label, const juce::String& text, float size, bool muted = false);
    void paintCard(juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour fill);
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
    PerformanceSlider tempoSlider, volumeSlider, transposeSlider;
    juce::TextButton playButton { "PLAY" }, resetButton { "RESET" };
    bool audioErrorVisible = false;
    bool audioCallbackRegistered = false;
    juce::String audioError;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
