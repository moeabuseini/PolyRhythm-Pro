#include "MainComponent.h"

#include <cmath>

MainComponent::MainComponent(juce::File settingsFolder) : userSettings(std::move(settingsFolder))
{
    setSize(1280, 820);
    setOpaque(true);

    styleLabel(title, "POLYRHYTHM", 12.0f, true);
    title.setColour(juce::Label::textColourId, accentA);
    styleLabel(subtitle, "PRO", 12.0f, true);
    subtitle.setColour(juce::Label::textColourId, textMuted);
    styleLabel(ratioTitle, "Nocturne Atlas", 27.0f);
    styleLabel(explanation, "Even current   /   D Dorian   /   3 melody pulses : 2 bass pulses", 14.0f, true);
    styleLabel(status, "READY TO PLAY", 11.0f, true);
    styleLabel(tempoLabel, "TEMPO · BASS", 10.0f, true);
    styleLabel(bpmValue, "96 BPM", 17.0f);
    styleLabel(voiceALabel, "MELODY", 10.0f, true);
    voiceALabel.setColour(juce::Label::textColourId, accentA);
    styleLabel(voiceBLabel, "BASS", 10.0f, true);
    voiceBLabel.setColour(juce::Label::textColourId, accentB);
    styleLabel(presetLabel, "START WITH A PATTERN", 10.0f, true);
    styleLabel(cycleLabel, "CYCLE 01  ·  00:00", 11.0f, true);
    styleLabel(volumeLabel, "MASTER LEVEL", 10.0f, true);
    styleLabel(transposeLabel, "MASTER PITCH", 10.0f, true);
    for (int i = 2; i <= 9; ++i)
    {
        voiceABox.addItem(juce::String(i) + " events", i);
        voiceBBox.addItem(juce::String(i) + " events", i);
    }
    for (int i = 0; i < static_cast<int>(rhythmPresets.size()); ++i)
        presetBox.addItem(rhythmPresets[static_cast<size_t>(i)].name, i + 1);

    const auto styleCombo = [this] (juce::ComboBox& combo)
    {
        combo.setScrollWheelEnabled(true);
        combo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff202b36));
        combo.setColour(juce::ComboBox::textColourId, textPrimary);
        combo.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff34424e));
        combo.setColour(juce::ComboBox::arrowColourId, textMuted);
        addAndMakeVisible(combo);
    };
    styleCombo(voiceABox);
    styleCombo(voiceBBox);
    styleCombo(presetBox);
    presetBox.setTextWhenNothingSelected("Custom");
    styleCombo(targetBox); styleCombo(meetingBox);
    targetBox.setTitle("Convergence target");
    targetBox.addItemList({"Convergence line","Convergence point"},1);
    targetBox.setSelectedId(1,juce::dontSendNotification);
    targetBox.onChange=[this]{ scene.setPointTarget(targetBox.getSelectedId()==2); inspector.syncControls(); };
    meetingBox.setTitle("Skip destination");
    meetingBox.addItemList({"Every point","Layer attacks"},1);
    meetingBox.setSelectedId(1,juce::dontSendNotification);
    for(int i=0;i<studio::layerCount;++i) {
        auto& button=layerButtons[i]; button.setButtonText(studio::layers[i]);
        button.setToggleState(engine.strips[i].enabled,juce::dontSendNotification); addAndMakeVisible(button);
        button.onClick=[this,i]{ engine.strips[i].enabled=layerButtons[i].getToggleState(); scene.setGrid(engine.grid()); };
    }
    styleCombo(environmentBox);
    environmentBox.setTitle("Visual environment");
    environmentBox.addItemList({"Eclipse Garden","Aurora Loom","Lunar Tide"},1);
    environmentBox.setSelectedId(1,juce::dontSendNotification);
    environmentBox.onChange=[this]{ scene.setEnvironment(environmentBox.getSelectedId()-1); inspector.selectPage(4); };
    voiceABox.setSelectedId(3, juce::dontSendNotification);
    voiceBBox.setSelectedId(2, juce::dontSendNotification);
    presetBox.setSelectedId(1, juce::dontSendNotification);

    styleSlider(tempoSlider, accentB);
    tempoSlider.setRange(1.0, 180.0, 1.0);
    tempoSlider.setValue(96.0, juce::dontSendNotification);
    tempoSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    styleSlider(volumeSlider, accentA);
    volumeSlider.setRange(0.0, 1.0, 0.01);
    volumeSlider.setValue(0.65, juce::dontSendNotification);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    styleSlider(transposeSlider, accentA);
    transposeSlider.setTitle("Master pitch (all layers)");
    transposeSlider.setRange(-12, 12, 1);
    transposeSlider.setValue(engine.masterTranspose.load(), juce::dontSendNotification);
    transposeSlider.setTextValueSuffix(" st");
    transposeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 54, 22);
    transposeSlider.setDoubleClickReturnValue(true, 0);
    transposeSlider.onValueChange = [this] {
        engine.masterTranspose = static_cast<int>(transposeSlider.getValue());
        updatePatternFromControls();
    };
    configureButton(playButton, true);
    configureButton(resetButton);
    scene.setOpaque(true);
    addAndMakeVisible(scene);
    inspectorView.setViewedComponent(&inspector,false);
    inspectorView.setScrollBarsShown(true,false);
    addAndMakeVisible(inspectorView);
    addAndMakeVisible(sideGrip); addAndMakeVisible(bottomGrip);
    sideGrip.moveBy=[this](int delta){ sideWidth-=delta; resized(); repaint(); };
    bottomGrip.moveBy=[this](int delta){ dockHeight-=delta; resized(); repaint(); };
    for(auto* button:{&presets,&fullscreen,&track,&rollAll,&rollCustom,&defaultsButton}) configureButton(*button);
    presets.onClick=[this]{ showPresets(); };
    fullscreen.onClick=[this]{ toggleFullscreen(); };
    track.setClickingTogglesState(true);
    track.setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff386768));
    track.setToggleState(true,juce::dontSendNotification);
    track.onClick=[this]{ scene.showForecast=track.getToggleState(); scene.repaint(); };
    rollAll.onClick=[this]{ warnRoll(); };
    rollCustom.onClick=[this]{ showSettings(false); };
    defaultsButton.setButtonText("Custom rhythm...");
    defaultsButton.onClick=[this]{ customRhythm(); };
    const char* menuNames[]{"File","Edit","View","Settings"};
    for(int i=0;i<4;++i){configureButton(menus[i]); menus[i].setButtonText(menuNames[i]); menus[i].onClick=[this,i]{menuFor(i);};}
    configureButton(variation);
    configureButton(createVideo);createVideo.onClick=[this]{openVideo();};
    variation.onClick = [this] { generateRhythm(); };
    for(auto* button:{&back,&forward,&align}) configureButton(*button);
    back.setTitle("Rewind five seconds"); forward.setTitle("Fast forward five seconds");
    back.onClick=[this]{ jumpTo(engine.playhead()-5/pattern.cycleSeconds()); };
    forward.onClick=[this]{ jumpTo(engine.playhead()+5/pattern.cycleSeconds()); };
    align.onClick=[this]{
        if(!engine.isAudioReady()) { showAudioError("No audio output available."); return; }
        const double now=engine.playhead();
        const double lead=std::min(0.75,2.5/pattern.cycleSeconds());
        const double meeting=engine.grid().nextMeeting(now,meetingBox.getSelectedId()==1);
        if(meeting<0) { status.setText("Enable an audible layer first",juce::dontSendNotification); return; }
        jumpTo(std::max(now,meeting-lead));
        engine.setPlaying(true); playButton.setButtonText("PAUSE");
        status.setText("PLAYING",juce::dontSendNotification);
    };

    for (auto* component : { &title, &subtitle, &ratioTitle, &explanation, &status, &tempoLabel,
                             &bpmValue, &voiceALabel, &voiceBLabel, &presetLabel, &cycleLabel, &volumeLabel, &transposeLabel })
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
    inspector.linkTempo(tempoSlider);
    tempoSlider.onValueChange = [this] { updatePatternFromControls(); };
    volumeSlider.onValueChange = [this] { engine.setMasterVolume(static_cast<float>(volumeSlider.getValue())); };
    playButton.onClick = [this]
    {
        const auto shouldPlay = !engine.isPlaying();
        if (shouldPlay && !engine.isAudioReady())
        {
            showAudioError(audioError.isNotEmpty() ? audioError : "Connect an audio output device and restart the app.");
            return;
        }
        engine.setPlaying(shouldPlay);
        playButton.setButtonText(shouldPlay ? "PAUSE" : "PLAY" );
        status.setText(shouldPlay ? "PLAYING" : "PAUSED",
                       juce::dontSendNotification);
        repaint();
    };
    resetButton.onClick = [this]
    {
        engine.setPlaying(false);
        jumpTo(0);
        playButton.setButtonText("PLAY");
        status.setText("READY TO PLAY", juce::dontSendNotification);
        syncTransportLabel();
    };

    engine.setPattern(pattern.voiceA, pattern.voiceB, pattern.bpm);
    engine.setMasterVolume(0.65f);
    makeCatalogue();
    recall(defaults,false);
    checkpoint();
    addMouseListener(this,true);
    warn=int(readStudioFile(userSettings.getChildFile("preferences.xml")).getProperty("warn",1))!=0;
    resized();

    const auto deviceError = audioDeviceManager.initialise(0, 2, nullptr, true);
    if (deviceError.isNotEmpty() || audioDeviceManager.getCurrentAudioDevice() == nullptr)
    {
        showAudioError(deviceError.isNotEmpty() ? deviceError : "No audio output device is available.");
    }
    else
    {
        const juce::Component::SafePointer<MainComponent> safeThis(this);
        audioSourcePlayer.errorHandler = [safeThis] (const juce::String& message)
        {
            juce::MessageManager::callAsync([safeThis, message]
            {
                if (safeThis != nullptr)
                    safeThis->showAudioError(message);
            });
        };
        audioSourcePlayer.setSource(this);
        audioDeviceManager.addAudioCallback(&audioSourcePlayer);
        audioCallbackRegistered = true;
    }
    displaySync=std::make_unique<juce::VBlankAttachment>(this,[this](double){
        lastVblankMs=juce::Time::getMillisecondCounterHiRes();
        if(engine.isPlaying() && isShowing())animateStage();
    });
    startTimerHz(30); // Inspector, history and fallback, not the animation ceiling.
    syncTransportLabel();
}

MainComponent::~MainComponent()
{
    stopTimer();
    displaySync.reset();
    removeMouseListener(this);
    inspectorView.setViewedComponent(nullptr,false);
    audioSourcePlayer.setSource(nullptr);
    if (audioCallbackRegistered)
        audioDeviceManager.removeAudioCallback(&audioSourcePlayer);
    audioDeviceManager.closeAudioDevice();
    audioSourcePlayer.errorHandler = {};
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
    g.setGradientFill({fill.brighter(0.30f).withAlpha(0.76f),box.getTopLeft(),
                       fill.withAlpha(0.65f),box.getBottomRight(),false});
    g.fillRoundedRectangle(box, 16.0f);
    g.setGradientFill({juce::Colour(0x548eadba),box.getTopLeft(),juce::Colour(0x225d7089),box.getBottomRight(),false});
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

    const auto w=getWidth(), dockY=getHeight()-18-dockHeight;
    const int right=w-18-sideWidth;
    const int panelW=w-36, first=panelW*35/100, second=panelW*29/100;
    g.setGradientFill({juce::Colour(0x44437685),0.f,0.f,juce::Colour(0x001d263a),w*0.7f,getHeight()*0.85f,true});
    g.fillRect(bg);
    paintCard(g,{18,104,right-30,dockY-118},juce::Colour(0xff172a36));
    paintCard(g,{right,104,sideWidth,dockY-118},juce::Colour(0xff24263b));
    paintCard(g,{18,dockY,first-6,dockHeight});
    paintCard(g,{18+first,dockY,second-6,dockHeight});
    paintCard(g,{18+first+second,dockY,panelW-first-second,dockHeight},juce::Colour(0xff263141));
    // Two intersecting orbits mark the instrument without competing with the stage.
    g.setColour(accentA.withAlpha(0.85f));
    g.drawEllipse(25,16,17,13,1.3f); g.drawEllipse(32,16,17,13,1.3f);
    g.fillEllipse(38,15,4,4);


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
    const int w=getWidth(), h=getHeight();
    if(w<400 || h<300) return;
    if(video)video->setBounds(getLocalBounds());
    if(filming){scene.setBounds(getLocalBounds());return;}
    sideWidth=juce::jlimit(340,juce::jmax(340,w/2),sideWidth);
    dockHeight=juce::jlimit(154,juce::jmax(154,h-392),dockHeight);
    const int dockY=h-18-dockHeight, right=w-18-sideWidth;
    title.setBounds(53,11,90,23); subtitle.setBounds(141,11,44,23);
    ratioTitle.setBounds(20,32,w-755,40);
    explanation.setBounds(22,71,w-550,23);
    auto bar=juce::Rectangle<int>(w-502,14,484,40);
    back.setBounds(bar.removeFromLeft(68)); playButton.setBounds(bar.removeFromLeft(92));
    forward.setBounds(bar.removeFromLeft(68)); align.setBounds(bar.removeFromLeft(154)); resetButton.setBounds(bar);
    presets.setBounds(w-736,16,105,34); fullscreen.setBounds(w-626,16,111,34);
    for(int i=0;i<4;++i) menus[i].setBounds(182+i*60,8,59,27);
    cycleLabel.setBounds(w-498,64,191,24); meetingBox.setBounds(w-202,60,180,34);
    auto stageTools=juce::Rectangle<int>(26,110,right-50,33);
    createVideo.setBounds(stageTools.removeFromRight(110));
    environmentBox.setBounds(stageTools.removeFromLeft(juce::jmin(194,stageTools.getWidth()/3)));
    targetBox.setBounds(stageTools.removeFromLeft(158)); track.setBounds(stageTools.removeFromRight(108));
    scene.setBounds(24,148,right-42,juce::jmax(60,dockY-168));
    sideGrip.setBounds(right-12,104,12,dockY-118);
    bottomGrip.setBounds(18,dockY-14,w-36,14);
    inspectorView.setBounds(right+4,108,sideWidth-8,dockY-126);
    const int inspectorHeight=juce::jmax(470,inspectorView.getHeight());
    inspector.setSize(inspectorView.getWidth()-(inspectorHeight>inspectorView.getHeight()?inspectorView.getScrollBarThickness():0),inspectorHeight);
    const int panelW=w-36, first=panelW*35/100, second=panelW*29/100;
    auto mix=juce::Rectangle<int>(30,dockY+10,first-30,dockHeight-20);
    auto level=juce::Rectangle<int>(30+first,dockY+10,second-30,dockHeight-20);
    auto perform=juce::Rectangle<int>(30+first+second,dockY+10,panelW-first-second-24,dockHeight-20);
    const int rowH=juce::jmin(39,mix.getHeight()/4);
    auto labels=mix.removeFromTop(23), choices=mix.removeFromTop(rowH);
    const int ratioWidth=choices.getWidth()*4/10, eventsWidth=(choices.getWidth()-ratioWidth)/2;
    presetLabel.setBounds(labels.removeFromLeft(ratioWidth)); voiceALabel.setBounds(labels.removeFromLeft(eventsWidth)); voiceBLabel.setBounds(labels);
    presetBox.setBounds(choices.removeFromLeft(ratioWidth)); voiceABox.setBounds(choices.removeFromLeft(eventsWidth)); voiceBBox.setBounds(choices);
    auto layers=mix.withSizeKeepingCentre(mix.getWidth(),34);
    for(int i=0;i<studio::layerCount;++i)
        layerButtons[i].setBounds(layers.removeFromLeft(layers.getWidth()/(studio::layerCount-i)));
    auto tempo=level.removeFromTop(23); bpmValue.setBounds(tempo.removeFromRight(92)); tempoLabel.setBounds(tempo);
    tempoSlider.setBounds(level.removeFromTop(rowH));
    auto levelRow=level.removeFromTop(level.getHeight()/2); volumeLabel.setBounds(levelRow.removeFromLeft(96)); volumeSlider.setBounds(levelRow);
    transposeLabel.setBounds(level.removeFromLeft(96)); transposeSlider.setBounds(level);
    status.setBounds(perform.removeFromTop(24));
    auto actions=perform.removeFromTop(perform.getHeight()/2);
    variation.setBounds(actions.removeFromLeft(actions.getWidth()/2).reduced(2)); rollAll.setBounds(actions.reduced(2));
    rollCustom.setBounds(perform.removeFromLeft(perform.getWidth()/2).reduced(2)); defaultsButton.setBounds(perform.reduced(2));
    if(sheet) sheet->setBounds(getLocalBounds());
}

void MainComponent::toggleFullscreen()
{
    auto& desktop=juce::Desktop::getInstance();
    auto* window=getTopLevelComponent();
    if(!window || !window->isOnDesktop()) return;
    desktop.setKioskModeComponent(desktop.getKioskModeComponent()==window ? nullptr : window);
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    if(filming){
        if(key==juce::KeyPress::escapeKey)finishVideo();
        else if(key==juce::KeyPress::spaceKey)engine.setPlaying(!engine.isPlaying());
        return true;
    }
    if(key.getModifiers().isCtrlDown() && key.getKeyCode()=='Z'){travelHistory(key.getModifiers().isShiftDown()?1:-1);return true;}
    if(key.getModifiers().isCtrlDown() && key.getKeyCode()=='Y'){travelHistory(1);return true;}
    if(lastSlider && lastSlider->isShowing() && lastSlider->isEnabled() && (key==juce::KeyPress::leftKey || key==juce::KeyPress::rightKey)){
        lastSlider->setValue(lastSlider->getValue()+(key==juce::KeyPress::leftKey?-1:1)*lastSlider->getInterval(),juce::sendNotificationSync);return true;
    }
    if(key==juce::KeyPress::F11Key || (key==juce::KeyPress::escapeKey && juce::Desktop::getInstance().getKioskModeComponent()!=nullptr)) {
        toggleFullscreen(); return true;
    }
    return false;
}


void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    engine.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    engine.getNextAudioBlock(bufferToFill);
}

void MainComponent::releaseResources()
{
    engine.releaseResources();
}

void MainComponent::showAudioError(const juce::String& errorMessage)
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
    const auto tonic = juce::MidiMessage::getMidiNoteName(62 + engine.masterTranspose.load(), true, false, 4);
    explanation.setText(juce::String(studio::BeatGrid::grooves[studio::BeatGrid::groove(engine.seed.load())]) + "   /   " + tonic + " Dorian   /   " + juce::String(pattern.voiceA) + " melody pulses : "
                        + juce::String(pattern.voiceB) + " bass pulses", juce::dontSendNotification);
    bpmValue.setText(juce::String(static_cast<int>(std::round(pattern.bpm))) + " BPM", juce::dontSendNotification);
    repaint();
}

void MainComponent::jumpTo(double cycles)
{
    displayHits.clear();
    const juce::ScopedLock callbackLock(audioDeviceManager.getAudioCallbackLock());
    studio::Hit stale {};
    while(engine.popHit(stale)) {}
    scene.reset();
    engine.seek(std::max(0.,cycles));
}

void MainComponent::generateRhythm()
{
    auto& random=juce::Random::getSystemRandom();
    // Choose a different ratio, not just a different starting note.
    int melody, bass;
    do {
        melody=2+random.nextInt(8);
        bass=2+random.nextInt(6);
    } while(melody==bass || melody*pattern.voiceB==bass*pattern.voiceA);
    int nextSeed;
    do { nextSeed=random.nextInt(999999); } while(studio::BeatGrid::groove(nextSeed)==studio::BeatGrid::groove(engine.seed.load()));
    engine.seed=nextSeed;
    voiceABox.setSelectedId(melody,juce::dontSendNotification);
    voiceBBox.setSelectedId(bass,juce::dontSendNotification);
    presetBox.setSelectedId(0,juce::dontSendNotification);
    updatePatternFromControls();
    jumpTo(0);
}

void MainComponent::timerCallback()
{
    if(!filming && !juce::ModifierKeys::getCurrentModifiersRealtime().isAnyMouseButtonDown()) checkpoint();
    const bool full=juce::Desktop::getInstance().getKioskModeComponent()==getTopLevelComponent();
    const juce::String fullText=full ? "Exit full screen" : "Full screen";
    if(fullscreen.getButtonText()!=fullText) fullscreen.setButtonText(fullText);
    if(isShowing() && (!engine.isPlaying() || juce::Time::getMillisecondCounterHiRes()-lastVblankMs>150))
        animateStage();
    inspector.refreshPitchMap();
    syncTransportLabel();
}

void MainComponent::animateStage()
{
    studio::Hit hit {};
    bool reunion=false;
    const auto clock=engine.displayTime();
    while(engine.popHit(hit)) if(engine.isPlaying()) displayHits.push_back(hit);
    displayHits.erase(std::remove_if(displayHits.begin(),displayHits.end(),[&](const studio::Hit& h){
        if(h.time>clock.seconds && engine.isPlaying())return false;
        scene.strike(h);inspector.noteHit(h);reunion |= h.fullReunion;return true;
    }),displayHits.end());
    if(filming)updateVideo(clock.seconds,reunion);
    scene.setGrid(engine.grid());
    scene.update(clock.seconds,clock.cycles);
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
