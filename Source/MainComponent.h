#pragma once

#include <JuceHeader.h>
#include "RhythmModel.h"
#include "RhythmScene.h"
#include "TransportAudioEngine.h"

class MainComponent final : public juce::AudioAppComponent,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;
    void audioDeviceError(const juce::String& errorMessage) override;

private:
    void timerCallback() override;
    void updatePatternFromControls();
    void syncTransportLabel();
    void configureButton(juce::TextButton& button, bool primary = false);
    void styleSlider(juce::Slider& slider, const juce::Colour& accent);
    void styleLabel(juce::Label& label, const juce::String& text, float size, bool muted = false);
    void paintCard(juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour fill = juce::Colour(0xff18212b));

    TransportAudioEngine engine;
    RhythmPattern pattern;
    RhythmScene scene;
    juce::Label title, subtitle, status, ratioTitle, explanation, tempoLabel, bpmValue,
                voiceALabel, voiceBLabel, cycleLabel, volumeLabel, presetLabel;
    juce::ComboBox voiceABox, voiceBBox, presetBox;
    juce::Slider tempoSlider, volumeSlider;
    juce::TextButton playButton { "PLAY" }, resetButton { "RESET" };
    juce::Colour background { 0xff0c1117 };
    juce::Colour panel { 0xff141c25 };
    juce::Colour textPrimary { 0xffedf3f7 };
    juce::Colour textMuted { 0xff8b9aa8 };
    juce::Colour accentA { 0xff59e0d0 };
    juce::Colour accentB { 0xffffa66d };
    bool audioErrorVisible = false;
    juce::String audioError;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
