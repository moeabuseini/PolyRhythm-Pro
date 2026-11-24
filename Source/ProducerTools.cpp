#include "MainComponent.h"

using namespace producerUI;

namespace
{
const juce::StringArray rootNames{"C","Db","D","Eb","E","F","Gb","G","Ab","A","Bb","B"};
const juce::StringArray qualityNames{"Major","Minor 9","Major 7","Dominant 7","Add 9"};

juce::File mixFile(const juce::File& folder,int slot)
{
    return folder.getChildFile(slot==0?"Mix A.xml":"Mix B.xml");
}
bool isMixParameter(const juce::String& id)
{
    if(id=="level")return true;
    if(!id.startsWith("layer"))return false;
    if(id.endsWith("_on"))return true;
    if(!id.contains("_dial"))return false;
    const int index=id.fromLastOccurrenceOf("_dial",false,false).getIntValue();
    return index==studio::gain || index>=studio::cutoff;
}
}

void MainComponent::notifyProducer(const juce::String& text)
{
    status.setText(text,juce::dontSendNotification);
    status.setTooltip(text);
}
void MainComponent::saved(const juce::String& heading,const juce::File& file)
{
    receipt.post(heading,file.getFullPathName(),file);placeReceipt();
}
void MainComponent::captured(const juce::String& heading,const juce::String& description)
{
    receipt.post(heading,description,{});placeReceipt();
}
void MainComponent::placeReceipt()
{
    if(!receipt.isVisible())return;
    juce::Component* parent=this;
    if(video && video->isVisible())parent=video.get();
    else if(sheet && sheet->isVisible())parent=sheet.get();
    if(receipt.getParentComponent()!=parent)parent->addAndMakeVisible(receipt);
    auto area=parent==this && !scene.getBounds().isEmpty()?scene.getBounds():parent->getLocalBounds();
    receipt.setBounds(area.reduced(10).removeFromBottom(133).removeFromLeft(540));
    if(parent->getChildComponent(parent->getNumChildComponents()-1)!=&receipt)receipt.toFront(false);
}

void MainComponent::initProducer()
{
    for(int i=0;i<2;++i)storedMix[i]=mixFile(userSettings,i).existsAsFile();
    auto& desk=inspector.producerPanel;
    desk.invoke=[this](int id){if(actionEnabled(id))producerAction(id);refreshProducer();};
    desk.refresh=[this]{refreshProducer();};
    for(const auto& control:controls){
        juce::ApplicationCommandInfo info(control.id);
        info.setInfo(control.label,control.label,control.group,0);
        const int ctrl=juce::ModifierKeys::ctrlModifier,shift=juce::ModifierKeys::shiftModifier;
        switch(control.id){
            case catchMeeting:info.addDefaultKeypress('M',ctrl|shift);break;
            case holdExposure:info.addDefaultKeypress('E',0);break;
            case undoEdit:info.addDefaultKeypress('Z',ctrl);break;
            case redoEdit:info.addDefaultKeypress('Y',ctrl);info.addDefaultKeypress('Z',ctrl|shift);break;
            case fullScreen:info.addDefaultKeypress(juce::KeyPress::F11Key,0);break;
            default:break;
        }
        commands.registerCommand(info);
    }
    auto* mappings=commands.getKeyMappings();
    mappings->resetToDefaultMappings();
    if(auto xml=juce::parseXML(userSettings.getChildFile("shortcuts.xml")))mappings->restoreFromXml(*xml);
}

void MainComponent::showShortcuts()
{
    struct Editor final : juce::Component {
        explicit Editor(juce::KeyPressMappingSet& original):draft(original.getCommandManager()),keys(draft,true){
            if(auto snapshot=original.createXml(false))draft.restoreFromXml(*snapshot);
            addAndMakeVisible(keys);addAndMakeVisible(save);setSize(600,500);
            const auto& p=GlassTheme::current().colours();
            keys.setColours(p.panel,p.text());
        }
        void resized() override {
            auto area=getLocalBounds().reduced(10);save.setBounds(area.removeFromBottom(34).removeFromRight(160));
            area.removeFromBottom(8);keys.setBounds(area);
        }
        juce::KeyPressMappingSet draft;
        juce::KeyMappingEditorComponent keys;
        juce::TextButton save{"Save shortcuts"};
    };
    auto* editor=new Editor(*commands.getKeyMappings());
    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle="Keyboard shortcuts";options.content.setOwned(editor);options.componentToCentreAround=this;
    options.dialogBackgroundColour=GlassTheme::current().colours().panel;
    shortcutDialog.reset(options.create());
    editor->save.onClick=[this,editor]{
        auto xml=editor->draft.createXml(true);
        auto file=userSettings.getChildFile("shortcuts.xml");
        if(!xml || !storeStudioFile(file,StudioState::fromXml(*xml))){
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Shortcuts not saved",file.getFullPathName());return;
        }
        commands.getKeyMappings()->restoreFromXml(*xml);shortcutDialog->exitModalState(1);shortcutDialog->setVisible(false);
        saved("Keyboard shortcuts saved",file);
    };
    shortcutDialog->enterModalState(true,nullptr,false);
}

bool MainComponent::actionEnabled(int id) const
{
    switch(id){
        case melodyA:case melodyB:case melodyC:return variants[id-melodyA]>=0;
        case recallA:case recallB:return storedMix[id-recallA];
        case restoreMix:return priorMix.isValid();
        case saveStill:return scene.exposureStill.isValid();
        case unpin:return scene.pinLayer>=0;
        case stems:return renderPool.getNumJobs()==0;
        case settle:return fanCue;
        default:return true;
    }
}

void MainComponent::refreshProducer()
{
    auto& desk=inspector.producerPanel;
    if(!desk.isVisible())return;
    const auto grid=engine.grid();
    const auto step=int64_t(engine.playhead()*grid.divisions[0]*0.5);
    const int sounding=grid.laneSeed(0,step);
    auto name=[&](int seed)->juce::String{
        if(seed<0 || seed==grid.seed)return "Original";
        for(int i=0;i<3;++i)if(variants[i]==seed)return juce::String::charToString('A'+i);
        return "Previous variation";
    };
    auto melodyText="Melody: "+name(sounding);
    const bool queued=step<grid.melodySwitch && sounding!=grid.laneSeed(0,grid.melodySwitch);
    if(queued)melodyText+="   /   Next: "+name(grid.melodyAfter);
    desk.melody.setText(melodyText,juce::dontSendNotification);
    juce::String detail;
    switch(desk.group){
        case 0:
            if(engine.roadmap){
                static const juce::StringArray suffix{"","m9","maj7","7","add9"};
                for(int i=0;i<4;++i){if(i)detail+="  /  ";detail+=rootNames[engine.chordRoots[i]]+suffix[engine.chordKinds[i]];}
            }else detail="Melody changes on the next phrase while playing.";
            break;
        case 1:detail=fanReturns?"Fan: two beats open, two beats back to your spread.":"Fan: opens over two beats and stays at 100%.";break;
        case 2:detail=gesture.isValid()?"Recording controls. Finish the take to edit or save it.":scene.exposing?"Collecting strikes until the next reunion.":"MIDI catches played notes; stems render forward.";break;
        default:detail=priorMix.isValid()?"Comparing mixes. Return restores the mix you started with.":"Store two mixes to compare levels and effects.";break;
    }
    desk.detail.setText(detail,juce::dontSendNotification);
    for(auto* b:desk.buttons){
        if(!b->isVisible())continue;
        const int id=b->getComponentID().getIntValue();
        b->setEnabled(actionEnabled(id));
        bool active=false;
        switch(id){
            case lift:active=fanCue;break;case settle:active=fanReturns;b->setButtonText(fanReturns?"After: settle back":"After: stay open");break;
            case space:active=engine.spaceCue;break;case resolve:active=engine.resolveCue;break;
            case lockBass:active=engine.bassSeed>=0;break;case roadmapToggle:active=engine.roadmap;break;
            case clearMelody:active=sounding==grid.seed;break;
            case melodyA:case melodyB:case melodyC:active=sounding==variants[id-melodyA];break;
            case camera:active=scene.shotCamera;break;case portrait:active=scene.portraitSafe;break;
            case record:active=gesture.isValid();b->setButtonText(active?"Finish take...":"Record automation");break;
            case expose:active=scene.exposing;b->setButtonText(active?"Finish exposure":"Expose to reunion");break;
            default:break;
        }
        b->setToggleState(active,juce::dontSendNotification);
    }
}
void MainComponent::cancelFan(){fanLastCycle=-1;scene.fanBloom=0;}

void MainComponent::producerAction(int id)
{
    const auto score=engine.grid();
    const int phrase=score.phraseLength();
    auto stepFor=[&](int l){return int(engine.playhead()*score.divisions[l]*0.5);};
    switch(id){
        case catchMeeting:case catchRecent:case completedPass:catchMidi(id==catchRecent?0:id==completedPass?2:1);return;
        case browse:
            if(exports().createDirectory().wasOk()) exports().startAsProcess();
            return;
        case stems:exportStems();return;
        case settle:fanReturns=!fanReturns;cancelFan();break;
        case restoreMix:if(priorMix.isValid()){auto mix=priorMix;priorMix={};recall(mix,false);}return;
        case openProducer:producer.onClick();return;
        case playPause:playButton.onClick();return;
        case producerUI::rewind:back.onClick();return;
        case advance:forward.onClick();return;
        case nextAlignment:align.onClick();return;
        case resetTransport:resetButton.onClick();return;
        case randomRhythm:generateRhythm();return;
        case savePerformance:savePreset();return;
        case openVideoEditor:openVideo();return;
        case fullScreen:toggleFullscreen();return;
        case undoEdit:travelHistory(-1);return;
        case redoEdit:travelHistory(1);return;
        case noteTrack:track.triggerClick();return;
        case resolve:engine.resolveCue=!engine.resolveCue;break;
        case lift:fanCue=!fanCue;if(!fanCue)cancelFan();break;
        case space:engine.spaceCue=!engine.spaceCue;break;
        case lockBass:{
            const juce::ScopedLock lock(audioDeviceManager.getAudioCallbackLock());
            if(engine.bassSeed>=0)engine.bassSeed=-1;
            else {engine.bassStart=std::max(0,stepFor(1)/phrase-1)*phrase;engine.bassSeed=score.seed;}
            break;
        }
        case evolve:{
            auto& rng=juce::Random::getSystemRandom();
            for(int i=0;i<3;++i)do{variants[i]=rng.nextInt(999999);}
                while(variants[i]==score.seed || std::find(variants.begin(),variants.begin()+i,variants[i])!=variants.begin()+i);
            id=melodyA;
            [[fallthrough]];
        }
        case melodyA:case melodyB:case melodyC:case clearMelody:{
            if(id!=clearMelody && variants[id-melodyA]<0)return;
            const juce::ScopedLock lock(audioDeviceManager.getAudioCallbackLock());
            const int step=stepFor(0);
            engine.melodyBefore=score.laneSeed(0,step);
            engine.melodyAfter=id==clearMelody?-1:variants[id-melodyA];
            engine.melodySwitch=engine.isPlaying()?(step/phrase+1)*phrase:step;
            notifyProducer(engine.isPlaying()?"Melody queued for next phrase":"Melody selected");
            break;
        }
        case roadmapToggle:engine.roadmap=!engine.roadmap;updatePatternFromControls();break;
        case roadmapEdit:editRoadmap();return;
        case record:
            if(gesture.isValid()){sampleGesture();finishGesture();return;}
            if(!engine.isAudioReady()){notifyProducer("Connect audio before recording");return;}
            gesture=StudioState("Sequence");
            gesture.setProperty("name","Live automation take",nullptr);
            gesture.setProperty("openingPhase",engine.playhead(),nullptr);
            gesture.addChild(captureStudio(catalogue),-1,nullptr);
            gestureValues.clear();
            for(const auto& setting:catalogue)gestureValues.push_back(setting.read());
            automationStartedTransport=!engine.isPlaying();
            gestureStart=engine.elapsedSeconds();engine.setPlaying(true);playButton.setButtonText("PAUSE");
            producer.setButtonText("Recording...");notifyProducer("Recording controls");return;
        case storeA:case storeB:mixSlot(id-storeA,true);return;
        case recallA:case recallB:mixSlot(id-recallA,false);return;
        case expose:if(scene.exposing)scene.endExposure();else scene.beginExposure();break;
        case saveStill:{
            if(!scene.exposureStill.isValid() || exports().createDirectory().failed()){notifyProducer("No writable exposure");return;}
            auto file=exports().getNonexistentChildFile("Constellation",".png",false);
            juce::TemporaryFile temp(file);
            bool ok=false;
            if(auto out=temp.getFile().createOutputStream()){
                ok=juce::PNGImageFormat().writeImageToStream(scene.exposureStill,*out);
                out->flush();ok=ok && out->getStatus().wasOk();
            }
            if(ok && temp.overwriteTargetFileWithTemporary())saved("Exposure exported",file);else notifyProducer("PNG export failed");return;
        }
        case shotSettings:{
            auto* dialog=new juce::AlertWindow("Two-shot camera","The close shot follows the pinned strand. The wide shot centres the stage at reunion.",juce::MessageBoxIconType::NoIcon,this);
            dialog->addComboBox("zoom",{"1.25x","1.5x","1.85x","2.25x"},"Close-up magnification");
            dialog->addComboBox("lead",{"3 seconds","6 seconds","10 seconds","16 seconds"},"Start pulling back");
            dialog->getComboBoxComponent("zoom")->setSelectedItemIndex(scene.closeShot,juce::dontSendNotification);
            dialog->getComboBoxComponent("lead")->setSelectedItemIndex(scene.shotLead,juce::dontSendNotification);
            dialog->addButton("Use shots",1);dialog->addButton("Cancel",0);
            dialog->enterModalState(true,juce::ModalCallbackFunction::create([self=juce::Component::SafePointer<MainComponent>(this),dialog](int ok){
                if(!self || !ok)return;
                self->scene.closeShot=dialog->getComboBoxComponent("zoom")->getSelectedItemIndex();
                self->scene.shotLead=dialog->getComboBoxComponent("lead")->getSelectedItemIndex();
                self->scene.shotCamera=true;self->scene.repaint();
            }),true);
            return;
        }
        case camera:scene.shotCamera=!scene.shotCamera;break;
        case portrait:scene.portraitSafe=!scene.portraitSafe;break;
        case unpin:scene.pinLayer=-1;break;
        default:return;
    }
    scene.setGrid(engine.grid());scene.repaint();
}

bool MainComponent::keyStateChanged(bool)
{
    if(lastShortcutKey && !juce::KeyPress::isKeyCurrentlyDown(lastShortcutKey))lastShortcutKey=0;
    if(heldExposureKey && !juce::KeyPress::isKeyCurrentlyDown(heldExposureKey)){
        heldExposureKey=0;scene.endExposure();return true;
    }
    return false;
}
void MainComponent::editRoadmap()
{
    auto* dialog=new juce::AlertWindow("Harmonic roadmap","Each chord lasts two shared cycles. Bass follows the root; upper voices move to the nearest chord tone.",juce::MessageBoxIconType::NoIcon,this);
    for(int i=0;i<4;++i){
        auto slot=juce::String(i+1);
        dialog->addComboBox("root"+slot,rootNames,"Chord "+slot);
        dialog->getComboBoxComponent("root"+slot)->setSelectedItemIndex(engine.chordRoots[i],juce::dontSendNotification);
        dialog->addComboBox("quality"+slot,qualityNames);
        dialog->getComboBoxComponent("quality"+slot)->setSelectedItemIndex(engine.chordKinds[i],juce::dontSendNotification);
    }
    dialog->addButton("Use roadmap",1,juce::KeyPress(juce::KeyPress::returnKey));dialog->addButton("Cancel",0,juce::KeyPress(juce::KeyPress::escapeKey));
    dialog->enterModalState(true,juce::ModalCallbackFunction::create([self=juce::Component::SafePointer<MainComponent>(this),dialog](int ok){
        if(!self || !ok)return;
        StudioState changes("Edit");
        for(int i=1;i<=4;++i)for(auto prefix:{"root","quality"}){
            const auto key=juce::String(prefix)+juce::String(i);
            changes.setProperty(key,dialog->getComboBoxComponent(key)->getSelectedItemIndex(),nullptr);
        }
        changes.setProperty("roadmap",1,nullptr);self->recall(changes,false);
    }),true);
}

void MainComponent::mixSlot(int slot,bool store)
{
    auto file=mixFile(userSettings,slot);
    if(store){
        StudioState mix("Mix");
        for(const auto& s:catalogue)if(isMixParameter(s.id))mix.setProperty(s.id,s.read(),nullptr);
        mix.setProperty("power",engine.isPlaying()?engine.outputPower.load():0.f,nullptr);
        if(storeStudioFile(file,mix)){storedMix[slot]=true;saved("Mix stored",file);}else notifyProducer("Mix save failed");return;
    }
    auto mix=readStudioFile(file),other=readStudioFile(mixFile(userSettings,1-slot));
    if(!mix.hasType("Mix")){notifyProducer("Empty mix slot");return;}
    if(!priorMix.isValid()){
        priorMix=StudioState("Mix");
        for(const auto& s:catalogue)if(isMixParameter(s.id))priorMix.setProperty(s.id,s.read(),nullptr);
    }
    const double own=mix["power"],reference=other.getProperty("power",0.);
    bool matched=own>1.e-9 && reference>1.e-9;
    if(matched)mix.setProperty("level",double(mix["level"])*std::sqrt(std::min(own,reference)/own),nullptr);
    recall(mix,false);
    notifyProducer(matched?"Mix recalled, RMS matched":"Mix recalled at stored level");
}

void MainComponent::sampleGesture()
{
    const int f=juce::roundToInt((engine.elapsedSeconds()-gestureStart)*30.);
    if(f<0 || f>108000){finishGesture();return;}
    for(size_t i=0;i<catalogue.size();++i){
        const auto& setting=catalogue[i];
        const double value=setting.snap(setting.read());
        if(std::abs(value-setting.snap(gestureValues[i]))<setting.step*0.1)continue;
        // Sampled points are already a gesture curve; don't interpolate across idle gaps.
        auto cue=videoCue(setting.id,f,0,value);
        cue.setProperty("caption","",nullptr);
        bool updated=false;
        for(int j=gesture.getNumChildren()-1;j>0;--j){
            auto prior=gesture.getChild(j);
            if(int(prior["frame"])!=f)break;
            if(prior["key"].toString()==setting.id){
                prior.setProperty("value",value,nullptr);updated=true;break;
            }
        }
        if(!updated)gesture.addChild(cue,-1,nullptr);
        gestureValues[i]=value;
    }
}
void MainComponent::finishGesture()
{
    if(!gesture.isValid())return;
    auto take=gesture;gesture={};producer.setButtonText("Producer");
    if(automationStartedTransport){engine.setPlaying(false);playButton.setButtonText("PLAY");automationStartedTransport=false;}
    if(take.getNumChildren()<2){notifyProducer("No control changes in take");return;}
    take.setProperty("duration",juce::jlimit(1,108000,juce::roundToInt((engine.elapsedSeconds()-gestureStart)*30)),nullptr);
    openVideo();video->library.setSelectedId(0,juce::dontSendNotification);video->load(take);video->setEditing(true);
    captured("Automation captured","In the video editor, not yet on disk. Choose Save as preset to keep the take.");
    video->message.setText("Live take: edit a cue or Save as preset.",juce::dontSendNotification);
}

void MainComponent::catchMidi(int window)
{
    std::vector<TransportAudioEngine::ScoreNote> notes;
    notes.reserve(engine.journal.size());
    double end=0,start=0;
    {
        const juce::ScopedLock lock(audioDeviceManager.getAudioCallbackLock());
        end=engine.elapsedSeconds();
        start=std::max(engine.journalOrigin,window?engine.lastMeeting:end-30.);
        if(window==2){
            end=engine.lastMeeting;
            start=engine.previousMeeting<0 && engine.journalOrigin>0?end:std::max(engine.journalOrigin,engine.previousMeeting);
        }
        if(engine.journalCount==engine.journal.size()){
            double oldest=engine.journal[engine.journalWrite].seconds;
            start=std::max(start,oldest);
        }
        for(size_t i=0;i<engine.journalCount;++i){
            const size_t index=(engine.journalWrite+engine.journal.size()-engine.journalCount+i)%engine.journal.size();
            auto note=engine.journal[index];if(note.seconds>=start && (window==2?note.seconds<end:note.seconds<=end))notes.push_back(note);
        }
    }
    if(notes.empty()){
        captured("No MIDI exported",window==2?"There is no completed reunion pass to capture yet.":"Play some notes first. The capture window is empty.");
        return;
    }
    if(exports().createDirectory().failed()){notifyProducer("Cannot open exports folder");return;}
    juce::MidiFile midi;
    constexpr int ppq=960;
    midi.setTicksPerQuarterNote(ppq);
    // Wall-clock attacks remain intact even if the tempo changed during the take.
    const int micros=pattern.bpm>=4?juce::roundToInt(60000000./pattern.bpm):500000;
    const double ticksPerSecond=1000000.*ppq/micros;
    for(int l=0;l<studio::layerCount;++l){
        juce::MidiMessageSequence trackEvents;
        trackEvents.addEvent(juce::MidiMessage::textMetaEvent(3,studio::layers[l]));
        if(l==0)trackEvents.addEvent(juce::MidiMessage::tempoMetaEvent(micros));
        for(auto note:notes)if(note.layer==l){
            const int pitch=juce::jlimit(0,127,note.pitch);
            auto on=juce::MidiMessage::noteOn(l+1,pitch,juce::jlimit(0.01f,1.f,note.velocity));
            auto off=juce::MidiMessage::noteOff(l+1,pitch);
            on.setTimeStamp(std::round((note.seconds-start)*ticksPerSecond));
            off.setTimeStamp(std::max(on.getTimeStamp()+1,std::round((std::min(end,note.seconds+note.duration)-start)*ticksPerSecond)));
            trackEvents.addEvent(on);trackEvents.addEvent(off);
        }
        midi.addTrack(trackEvents);
    }
    auto file=exports().getNonexistentChildFile("Caught performance",".mid",false);
    juce::TemporaryFile temp(file);bool ok=false;
    if(auto out=temp.getFile().createOutputStream()){ok=midi.writeTo(*out);out->flush();ok=ok && out->getStatus().wasOk();}
    if(ok && temp.overwriteTargetFileWithTemporary())saved("MIDI exported",file);else notifyProducer("MIDI export failed");
}

void MainComponent::exportStems()
{
    if(renderPool.getNumJobs()>0)return;
    if(exports().createDirectory().failed()){notifyProducer("Cannot open exports folder");return;}
    auto folder=exports().getNonexistentChildFile("Five stems","",false);
    if(folder.createDirectory().failed()){notifyProducer("Cannot create stem folder");return;}
    auto render=std::make_shared<TransportAudioEngine>();
    double start;
    {
        const juce::ScopedLock lock(audioDeviceManager.getAudioCallbackLock());
        engine.copySetupTo(*render);start=engine.playhead();
    }
    notifyProducer("Rendering five stems...");
    renderPool.addJob([render,folder,start,self=juce::Component::SafePointer<MainComponent>(this)](){
        constexpr int rate=48000,blockSize=512,attackSamples=16*rate,total=24*rate;
        juce::WavAudioFormat wav;
        std::array<std::unique_ptr<juce::AudioFormatWriter>,studio::layerCount> writers;
        juce::Array<juce::File> created;
        bool ok=true;
        for(int i=0;i<studio::layerCount;++i){
            auto file=folder.getChildFile(juce::String(studio::layers[i])+".wav");
            std::unique_ptr<juce::OutputStream> out=file.createOutputStream();
            if(!out){ok=false;break;}
            created.add(file);
            auto options=juce::AudioFormatWriterOptions{}.withSampleRate(rate).withNumChannels(2).withBitsPerSample(24);
            writers[i]=wav.createWriterFor(out,options);
            if(!writers[i]){ok=false;break;}
        }
        render->prepareToPlay(blockSize,rate);render->seek(start);render->setPlaying(true);
        render->stemSink=[&](int layer,const juce::AudioBuffer<float>& buffer,int n){
            if(ok)ok=writers[layer]->writeFromAudioSampleBuffer(buffer,0,n);
        };
        juce::AudioBuffer<float> scratch(2,blockSize);
        for(int sample=0;ok && sample<total;){
            if(auto* job=juce::ThreadPoolJob::getCurrentThreadPoolJob();job && job->shouldExit()){ok=false;break;}
            render->suppressAttacks=sample>=attackSamples;
            const int n=std::min(blockSize,(render->suppressAttacks?total:attackSamples)-sample);
            render->getNextAudioBlock({&scratch,0,n});sample+=n;
        }
        render->stemSink={};
        for(auto& writer:writers){if(writer)ok=writer->flush() && ok;writer.reset();}
        if(!ok){for(auto file:created)file.deleteFile();folder.deleteFile();}
        juce::MessageManager::callAsync([self,ok,folder]{
            if(!self)return;
            if(ok)self->saved("Five stereo stems exported",folder);else self->notifyProducer("Stem render did not finish");
        });
    });
}
