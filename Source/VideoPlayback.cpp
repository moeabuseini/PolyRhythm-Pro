#include "MainComponent.h"

namespace
{
StudioState newSequence(juce::String name,StudioState patch)
{
    StudioState s("Sequence"); s.setProperty("name",name,nullptr);
    s.setProperty("reunion",true,nullptr);s.addChild(patch.createCopy(),-1,nullptr);return s;
}
}

void MainComponent::populateVideos()
{
    sequences.clear();
    struct Score {
        const char* name;
        int melody,bass,tempo,nodes,peak;
        std::array<int,studio::layerCount> sounds;
        const char* opening; const char* expansion; const char* texture; const char* finale;
    };
    const Score scores[]{
        {"Infinity bloom",5,3,58,4,14,{0,7,1,4,11},
         "Start with the felt piano. Follow a single light.",
         "Watch the fan open. Can you still follow that first light?",
         "Hear the brass rise through the piano and glass.",
         "Stay with the quieter pulse. The whole sky is about to meet."},
        {"Silk in motion",7,4,76,7,16,{1,5,3,8,13},
         "Follow the glass keys as the aurora begins to turn.",
         "Listen for seven against four as more lights join in.",
         "Hear the strings swell underneath the rolled chords.",
         "Watch the curtains fold inward. Give the last crossing room to breathe."},
        {"Tidal echoes",5,4,64,6,15,{8,6,9,10,12},
         "Listen for a celesta on the water, then its reflection.",
         "Follow the echoes as the tide draws the paths apart.",
         "Hear the flute and prism lead answer across the water.",
         "Follow the last bright notes back to the moonlit line."}
    };
    constexpr int offsets[]{-2,0,3};
    for(int i=0;i<3;++i){
        const auto& score=scores[i];
        auto patch=factory.createCopy();
        auto set=[&](const juce::String& key,int value){patch.setProperty(key,value,nullptr);};
        set("environment",i+1);set("target",i==1?2:1);
        set("spread",i==1?18:0);set("nodes",score.nodes);set("passes",2);
        set("bpm",score.tempo);set("melody",score.melody);set("bass",score.bass);
        set("seed",1709+i*412);set("transpose",offsets[i]);set("track",0);
        set("shape",i==2?4:3);set("touch",2);
        for(int l=0;l<studio::layerCount;++l){
            const auto prefix="layer"+juce::String(l)+"_";
            set(prefix+"voice",score.sounds[l]);set(prefix+"on",l<2);
            set(prefix+"dial3",l>=2?4:1);
            set(prefix+"dial0",l==0?64:l==1?48:0);
            set(prefix+"order",(i+l)%3);
        }
        const auto sky="sky"+juce::String(i)+"_";
        set(sky+"look1",RhythmScene::maxStars);set(sky+"look6",100);
        set(sky+"look2",94);set(sky+"look3",82);
        set(sky+"look4",i==1?88:95);set(sky+"look5",88);
        set(sky+"camera",1);set(sky+"depth",92);set(sky+"cycle",1);set(sky+"range",48+i*14);
        auto sequence=newSequence(score.name,patch);
        sequence.setProperty("duration",1800,nullptr);
        auto cue=[&](int frame,int length,const juce::String& key,double value,const char* caption=""){
            auto event=videoCue(key,frame,length,value);
            event.setProperty("caption",caption,nullptr);
            sequence.addChild(event,-1,nullptr);
        };
        cue(0,0,"spread",i==1?18:0,score.opening);
        cue(180,270,"spread",i==2?90:100,score.expansion);
        cue(200,150,"layer0_dial11",i==2?56:35);
        cue(200,160,"layer0_dial12",i==2?54:41);
        cue(260,180,"nodes",score.peak);
        cue(300,240,"bpm",score.tempo+22);
        cue(330,140,"layer0_dial1",i==0?2:i==1?-2:5);
        cue(360,0,"layer2_on",1);
        cue(360,170,"layer2_dial0",42);
        cue(360,160,"layer2_dial9",89);
        cue(380,190,"layer2_dial10",48);
        cue(450,0,"layer3_on",1);
        cue(450,170,"layer3_dial0",31);
        cue(450,180,"layer3_dial13",55);
        cue(540,0,"layer4_on",1,score.texture);
        cue(540,180,"layer4_dial0",43);
        cue(540,170,"layer4_dial10",39);
        cue(580,160,"layer4_dial13",i==1?57:31);
        cue(660,0,"layer0_order",(i+1)%3);
        cue(675,0,"shape",i==0?2:i==1?4:3);
        cue(690,110,"layer3_dial19",36);
        cue(745,0,"touch",i==2?2:1);
        cue(770,180,"layer2_dial21",70);
        cue(780,200,sky+"look4",100);
        cue(780,200,sky+"look5",100);
        cue(800,160,"layer4_dial19",28);
        cue(820,190,"layer0_dial6",i==1?9000:6600);
        cue(960,210,"layer4_dial0",28);
        cue(1110,240,"spread",i==1?24:15,score.finale);
        cue(1140,235,"bpm",score.tempo-6);
        cue(1260,180,"layer0_dial11",18);
        sequences.push_back(sequence);
    }
    for(auto f:userSettings.getChildFile("Sequences").findChildFiles(juce::File::findFiles,false,"*.xml")){
        auto s=readStudioFile(f);
        if(s.hasType("Sequence") && s.getChild(0).hasType("Patch")){
            s.setProperty("file",f.getFileName(),nullptr);sequences.push_back(s);
        }
    }
    video->library.clear(juce::dontSendNotification);
    for(size_t i=0;i<sequences.size();++i)video->library.addItem(sequences[i]["name"].toString(),int(i)+1);
}

void MainComponent::openVideo()
{
    if(!video){
        video=std::make_unique<VideoEditor>(catalogue);addChildComponent(*video);
        populateVideos();
        video->library.onChange=[this]{int i=video->library.getSelectedId()-1;if(juce::isPositiveAndBelow(i,int(sequences.size())))video->load(sequences[i]);};
        video->library.setSelectedId(1,juce::sendNotificationSync);
        video->create.onClick=[this]{
            video->library.setSelectedId(0,juce::dontSendNotification);
            video->load(newSequence("Untitled sequence",captureStudio(catalogue)));
            video->setEditing(true);
        };
        auto saveSequence=[this](bool copy){
            auto s=video->sequence.createCopy();
            if(s["name"].toString().trim().isEmpty()){video->message.setText("Enter a sequence name first.",juce::dontSendNotification);return;}
            auto filename=copy ? juce::Uuid().toString()+".xml" : s["file"].toString();
            if(filename.isEmpty()) return;
            if(copy) {
                auto name=s["name"].toString().trim();
                for(const auto& saved:sequences) if(saved["name"].toString()==name) {
                    name+=" (copy)";break;
                }
                s.setProperty("name",name,nullptr);
            }
            s.removeProperty("file",nullptr);
            if(!storeStudioFile(userSettings.getChildFile("Sequences").getChildFile(filename),s)){
                video->message.setText("Could not write the sequence.",juce::dontSendNotification);return;
            }
            populateVideos();
            for(size_t i=0;i<sequences.size();++i)if(sequences[i]["file"].toString()==filename)
                video->library.setSelectedId(int(i)+1,juce::sendNotificationSync);
            video->message.setText("Sequence saved.",juce::dontSendNotification);
        };
        video->opening.onClick=[this]{
            video->sequence.removeChild(0,nullptr);
            video->sequence.addChild(captureStudio(catalogue),0,nullptr);
            video->message.setText("Opening settings captured. Timeline cues still apply.",juce::dontSendNotification);
        };
        video->save.onClick=[saveSequence]{saveSequence(false);};
        video->saveCopy.onClick=[saveSequence]{saveSequence(true);};
        video->trash.onClick=[this]{
            const auto file=userSettings.getChildFile("Sequences").getChildFile(video->sequence["file"].toString());
            juce::Component::SafePointer<MainComponent> self(this);
            juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon,"Remove saved sequence?",
                video->sequence["name"].toString(),"Move to Recycle Bin","Keep",video.get(),
                juce::ModalCallbackFunction::create([self,file](int ok){
                    if(!ok || !self) return;
                    if(!file.moveToTrash()) {
                        self->video->message.setText("Could not remove the saved sequence.",juce::dontSendNotification);
                        return;
                    }
                    self->populateVideos();
                    self->video->library.setSelectedId(1,juce::sendNotificationSync);
                }));
        };
        video->close.onClick=[this]{video->setVisible(false);};
        video->preview.onClick=[this]{previewVideo();};
    }
    video->setBounds(getLocalBounds());video->setVisible(true);video->toFront(true);
    video->enterModalState(true,nullptr,false);
}

void MainComponent::previewVideo()
{
    if(!engine.isAudioReady()){video->message.setText("An audio output is needed to preview.",juce::dontSendNotification);return;}
    auto seq=video->sequence;
    if(seq.getNumChildren()<2){video->message.setText("Add at least one cue first.",juce::dontSendNotification);return;}
    timeline.clear();narration.clear();
    videoEnd=juce::jlimit(0,216000,int(seq["duration"]))/30.0;
    videoReunion=bool(seq["reunion"]);reunionEnd=-1;
    auto start=factory.createCopy();
    start.copyPropertiesFrom(seq.getChild(0),nullptr);
    auto prior=start.createCopy();
    for(int i=1;i<seq.getNumChildren();++i){
        auto cue=seq.getChild(i);
        for(size_t k=0;k<catalogue.size();++k){
            auto& s=catalogue[k];if(s.id!=cue["key"].toString())continue;
            const int f=juce::jlimit(0,108000,int(cue["frame"])),n=s.choices.isEmpty()?juce::jlimit(0,108000,int(cue["length"])):0;
            const double from=s.snap(prior.getProperty(s.id,factory[s.id])),to=s.snap(cue["value"]);
            if(f==0 && n==0)start.setProperty(s.id,to,nullptr);
            timeline.push_back({k,f,n,from,to});prior.setProperty(s.id,to,nullptr);
            videoEnd=std::max(videoEnd,(f+std::max(75,n))/30.0);
            auto line=cue["caption"].toString();
            if(!cue.hasProperty("caption")){
                if(s.choices.isEmpty())
                    line="Notice how "+s.label.toLowerCase()+" changes.";
                else
                    line="Now try listening with "+s.choices[int(to-s.low)]+".";
            }
            if(line.isNotEmpty() && (narration.empty() || narration.back().start < f/30.0)){
                const double startTime=f/30.0;
                if(!narration.empty()) narration.back().end=std::min(narration.back().end,startTime);
                narration.push_back({startTime,startTime+std::max(165,n)/30.0,line});
            }
            break;
        }
    }
    if(timeline.empty())return;
    if(!narration.empty()) videoEnd=std::max(videoEnd,narration.back().end);
    beforeVideo=captureStudio(catalogue);beforePosition=engine.playhead();wasPlaying=engine.isPlaying();
    {
        const juce::ScopedLock lock(audioDeviceManager.getAudioCallbackLock());
        engine.setPlaying(false);recall(start,true);engine.setPlaying(true);
    }
    scene.cleanStage=true;
    filming=true;video->setVisible(false);scene.toFront(false);scene.setWantsKeyboardFocus(true);
    scene.enterModalState(true,nullptr,false);resized();
}

void MainComponent::updateVideo(double elapsed,bool reunion)
{
    // The transport's elapsed clock freezes on pause and doesn't jump as BPM changes.
    if(engine.hasPendingSeek())return;
    const double frame=elapsed*30;
    scene.captions.clear();
    for(auto& c:timeline){
        const double age=frame-c.frame;if(age<0)continue;
        // A later cue for this same control supersedes the earlier one.
        const auto& s=catalogue[c.index];
        const double t=c.length?std::min(1.,age/c.length):1.;
        const double v=s.snap(c.from+(c.to-c.from)*t*t*(3-2*t));
        if(!c.complete && s.read()!=v)s.write(v);
        c.complete=t>=1.;
    }
    for(const auto& line:narration){
        if(elapsed<line.start) break;
        if(elapsed>=line.end) continue;
        const auto t=std::clamp(std::min((elapsed-line.start)/1.25,(line.end-elapsed)/1.7),0.,1.);
        scene.captions.emplace_back(line.text,float(t*t*(3-2*t)));
        break;
    }
    engine.setPattern(pattern.voiceA,pattern.voiceB,pattern.bpm);scene.setPattern(pattern);
    if(environmentBox.getSelectedId()-1!=scene.activeEnvironment())scene.setEnvironment(environmentBox.getSelectedId()-1);
    scene.setPointTarget(targetBox.getSelectedId()==2);
    if(elapsed<videoEnd)return;
    auto grid=engine.grid();int lanes=0;for(auto probability:grid.probability)if(probability>0)++lanes;
    if(!videoReunion || lanes*grid.bodies<2){finishVideo();return;}
    if(reunion && reunionEnd<0)reunionEnd=elapsed+3.5;
    if(reunionEnd>=0 && elapsed>=reunionEnd)finishVideo();
}

void MainComponent::finishVideo()
{
    scene.cleanStage=false;
    filming=false;scene.exitModalState(0);scene.captions.clear();
    {
        const juce::ScopedLock lock(audioDeviceManager.getAudioCallbackLock());
        engine.setPlaying(false);recall(beforeVideo,false);jumpTo(beforePosition);engine.setPlaying(wasPlaying);
    }
    resized(); openVideo();
}
