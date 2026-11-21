#include "MainComponent.h"

#include <cmath>

MainComponent::MainComponent()
{
    setSize(1280, 820);
    setOpaque(true);

    styleLabel(title, "POLYRHYTHM", 12.0f, true);
    title.setColour(juce::Label::textColourId, accentA);
    styleLabel(subtitle, "PRO", 12.0f, true);
    subtitle.setColour(juce::Label::textColourId, textMuted);
    styleLabel(ratioTitle, "Two rhythms. One shared cycle.", 27.0f);
    styleLabel(explanation, "3 hits in A during the time of 2 hits in B.", 14.0f, true);
    styleLabel(status, "READY TO PLAY", 11.0f, true);
    styleLabel(tempoLabel, "TEMPO · VOICE B", 10.0f, true);
    styleLabel(bpmValue, "96 BPM", 17.0f);
    styleLabel(voiceALabel, "VOICE A", 10.0f, true);
    voiceALabel.setColour(juce::Label::textColourId, accentA);
    styleLabel(voiceBLabel, "VOICE B", 10.0f, true);
    voiceBLabel.setColour(juce::Label::textColourId, accentB);
    styleLabel(presetLabel, "START WITH A PATTERN", 10.0f, true);
    styleLabel(cycleLabel, "CYCLE 01  ·  00:00", 11.0f, true);
    styleLabel(volumeLabel, "MASTER LEVEL", 10.0f, true);

    for (int i = 2; i <= 9; ++i)
    {
        voiceABox.addItem(juce::String(i) + " events", i);
        voiceBBox.addItem(juce::String(i) + " events", i);
    }
    for (int i = 0; i < static_cast<int>(rhythmPresets.size()); ++i)
        presetBox.addItem(rhythmPresets[static_cast<size_t>(i)].name, i + 1);

    const auto styleCombo = [this] (juce::ComboBox& combo)
    {
        combo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff202b36));
        combo.setColour(juce::ComboBox::textColourId, textPrimary);
        combo.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff34424e));
        combo.setColour(juce::ComboBox::arrowColourId, textMuted);
        addAndMakeVisible(combo);
    };
    styleCombo(voiceABox);
    styleCombo(voiceBBox);
    styleCombo(presetBox);
    voiceABox.setSelectedId(3, juce::dontSendNotification);
    voiceBBox.setSelectedId(2, juce::dontSendNotification);
    presetBox.setSelectedId(1, juce::dontSendNotification);

    styleSlider(tempoSlider, accentB);
    tempoSlider.setRange(40.0, 220.0, 1.0);
    tempoSlider.setValue(96.0, juce::dontSendNotification);
    tempoSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    styleSlider(volumeSlider, accentA);
    volumeSlider.setRange(0.0, 1.0, 0.01);
    volumeSlider.setValue(0.65, juce::dontSendNotification);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

    configureButton(playButton, true);
    configureButton(resetButton);
    addAndMakeVisible(scene);

    for (auto* component : { &title, &subtitle, &ratioTitle, &explanation, &status, &tempoLabel,
                             &bpmValue, &voiceALabel, &voiceBLabel, &presetLabel, &cycleLabel, &volumeLabel })
        addAndMakeVisible(component);

    presetBox.onChange = [this]
    {
        const auto id = presetBox.getSelectedId();
        if (id > 0 && id <= static_cast<int>(rhythmPresets.size()))
        {
            const auto& preset = rhythmPresets[static_cast<size_t>(id - 1)];
            voiceABox.setSelectedId(preset.voiceA, juce::dontSendNotification);
            voiceBBox.setSelectedId(preset.voiceB, juce::dontSendNotification);
            updatePatternFromControls();
        }
    };
    voiceABox.onChange = [this] { presetBox.setSelectedId(0, juce::dontSendNotification); updatePatternFromControls(); };
    voiceBBox.onChange = [this] { presetBox.setSelectedId(0, juce::dontSendNotification); updatePatternFromControls(); };
    tempoSlider.onValueChange = [this] { updatePatternFromControls(); };
    volumeSlider.onValueChange = [this] { engine.setMasterVolume(static_cast<float>(volumeSlider.getValue())); };
    playButton.onClick = [this]
    {
        const auto shouldPlay = !engine.isPlaying();
        engine.setPlaying(shouldPlay);
        playButton.setButtonText(shouldPlay ? "PAUSE" : "PLAY" );
        status.setText(shouldPlay ? "PLAYING  ·  FOLLOW THE BEADS" : "PAUSED  ·  READY WHEN YOU ARE",
                       juce::dontSendNotification);
        repaint();
    };
    resetButton.onClick = [this]
    {
        engine.reset();
        engine.setPlaying(false);
        playButton.setButtonText("PLAY");
        status.setText("READY TO PLAY", juce::dontSendNotification);
        syncTransportLabel();
    };

    engine.setPattern(pattern.voiceA, pattern.voiceB, pattern.bpm);
    engine.setMasterVolume(0.65f);
    setAudioChannels(0, 2);
    startTimerHz(30);
    syncTransportLabel();
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::configureButton(juce::TextButton& button, bool primary)
{
    button.setColour(juce::TextButton::buttonColourId, primary ? accentA : juce::Colour(0xff202b36));
    button.setColour(juce::TextButton::buttonOnColourId, primary ? accentA.brighter(0.1f) : panel.brighter(0.15f));
    button.setColour(juce::TextButton::textColourOffId, primary ? juce::Colour(0xff0c1719) : textPrimary);
    button.setColour(juce::TextButton::textColourOnId, primary ? juce::Colour(0xff0c1719) : textPrimary);
    button.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    button.setClickingTogglesState(false);
    addAndMakeVisible(button);
}

void MainComponent::styleSlider(juce::Slider& slider, const juce::Colour& accent)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setColour(juce::Slider::trackColourId, juce::Colour(0xff33414d));
    slider.setColour(juce::Slider::thumbColourId, accent);
    slider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff202b36));
    slider.setColour(juce::Slider::rotarySliderFillColourId, accent);
    addAndMakeVisible(slider);
}

void MainComponent::styleLabel(juce::Label& label, const juce::String& content, float size, bool mutedText)
{
    label.setText(content, juce::dontSendNotification);
    label.setFont(juce::FontOptions(size, mutedText ? juce::Font::bold : juce::Font::plain));
    label.setColour(juce::Label::textColourId, mutedText ? textMuted : textPrimary);
    label.setJustificationType(juce::Justification::centredLeft);
}

void MainComponent::paintCard(juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour fill)
{
    auto box = bounds.toFloat();
    g.setColour(juce::Colour(0x12000000));
    g.fillRoundedRectangle(box.translated(0.0f, 5.0f), 16.0f);
    g.setColour(fill);
    g.fillRoundedRectangle(box, 16.0f);
    g.setColour(juce::Colour(0xff26333f));
    g.drawRoundedRectangle(box, 16.0f, 1.0f);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(background);
    auto bg = getLocalBounds().toFloat();
    juce::ColourGradient gradient(juce::Colour(0xff17212b), 0.0f, 0.0f,
                                  background, bg.getWidth() * 0.8f, bg.getHeight(), true);
    g.setGradientFill(gradient);
    g.fillRect(bg);

    paintCard(g, { 28, 116, getWidth() - 56, juce::jmax(340, getHeight() - 334) }, juce::Colour(0xff111922));
    const auto bottomY = getHeight() - 194;
    paintCard(g, { 28, bottomY, 398, 162 });
    paintCard(g, { 440, bottomY, 442, 162 });
    paintCard(g, { 896, bottomY, getWidth() - 924, 162 });
    g.setColour(juce::Colour(0x1949cab9));
    g.fillRoundedRectangle(28.0f, 28.0f, 3.0f, 46.0f, 1.5f);

    g.setColour(textMuted);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText("ORBIT VIEW  /  SHARED CYCLE", 48, 126, 260, 20, juce::Justification::centredLeft);
    g.setColour(accentA.withAlpha(0.9f));
    g.fillEllipse(32.0f, 128.0f, 7.0f, 7.0f);

    if (audioErrorVisible)
    {
        g.setColour(juce::Colour(0xff381f20));
        g.fillRoundedRectangle(28.0f, getHeight() - 24.0f, getWidth() - 56.0f, 18.0f, 6.0f);
        g.setColour(juce::Colour(0xffffb2a8));
        g.setFont(juce::FontOptions(10.0f));
        g.drawFittedText("Audio output unavailable. Check Windows sound settings and restart the app. " + audioError,
                         40, getHeight() - 23, getWidth() - 80, 16, juce::Justification::centredLeft, 1);
    }
}

void MainComponent::resized()
{
    const auto w = getWidth();
    const auto h = getHeight();
    title.setBounds(46, 30, 108, 18);
    subtitle.setBounds(155, 30, 42, 18);
    ratioTitle.setBounds(46, 52, 560, 34);
    explanation.setBounds(48, 83, 600, 24);
    playButton.setBounds(w - 252, 38, 126, 44);
    resetButton.setBounds(w - 112, 38, 84, 44);
    cycleLabel.setBounds(w - 260, 88, 220, 16);

    const auto sceneY = 116;
    const auto bottomY = h - 194;
    scene.setBounds(30, sceneY + 18, w - 60, bottomY - sceneY - 32);

    const auto y = bottomY;
    presetLabel.setBounds(48, y + 18, 180, 16);
    presetBox.setBounds(48, y + 43, 162, 30);
    voiceALabel.setBounds(222, y + 20, 70, 16);
    voiceABox.setBounds(222, y + 42, 86, 32);
    voiceBLabel.setBounds(320, y + 20, 70, 16);
    voiceBBox.setBounds(320, y + 42, 86, 32);

    tempoLabel.setBounds(460, y + 18, 160, 16);
    bpmValue.setBounds(752, y + 16, 106, 22);
    tempoSlider.setBounds(462, y + 43, 396, 24);
    volumeLabel.setBounds(460, y + 88, 120, 16);
    volumeSlider.setBounds(572, y + 83, 286, 24);

    status.setBounds(916, y + 23, w - 956, 16);
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    engine.prepareToPlay(samplesPerBlockExpected, sampleRate);
    audioErrorVisible = false;
    repaint();
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    engine.getNextAudioBlock(bufferToFill);
}

void MainComponent::releaseResources()
{
    engine.releaseResources();
}

void MainComponent::audioDeviceError(const juce::String& errorMessage)
{
    audioError = errorMessage;
    audioErrorVisible = true;
    status.setText("AUDIO OUTPUT UNAVAILABLE", juce::dontSendNotification);
    repaint();
}

void MainComponent::updatePatternFromControls()
{
    pattern.voiceA = juce::jlimit(2, 9, voiceABox.getSelectedId());
    pattern.voiceB = juce::jlimit(2, 9, voiceBBox.getSelectedId());
    pattern.bpm = tempoSlider.getValue();
    engine.setPattern(pattern.voiceA, pattern.voiceB, pattern.bpm);
    scene.setPattern(pattern);
    explanation.setText(juce::String(pattern.voiceA) + " hits in A during the time of "
                        + juce::String(pattern.voiceB) + " hits in B.", juce::dontSendNotification);
    bpmValue.setText(juce::String(static_cast<int>(std::round(pattern.bpm))) + " BPM", juce::dontSendNotification);
    repaint();
}

void MainComponent::timerCallback()
{
    scene.setTransport(engine.cyclePhase(), engine.cycleCount(), engine.isPlaying());
    syncTransportLabel();
}

void MainComponent::syncTransportLabel()
{
    const auto totalSeconds = static_cast<int>(engine.elapsedSeconds());
    const auto minutes = totalSeconds / 60;
    const auto seconds = totalSeconds % 60;
    cycleLabel.setText("CYCLE " + juce::String(engine.cycleCount() + 1).paddedLeft('0', 2)
                       + "  ·  " + juce::String(minutes).paddedLeft('0', 2)
                       + ":" + juce::String(seconds).paddedLeft('0', 2), juce::dontSendNotification);
}
