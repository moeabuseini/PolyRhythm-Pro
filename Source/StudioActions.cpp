#include "MainComponent.h"

namespace
{
const juce::StringArray environmentNames{"Eclipse Garden","Aurora Loom","Lunar Tide"};
const juce::StringArray factoryNames{"Nocturne Atlas","Copper Canopy","Glass Tides","Polar Drift"};
const juce::StringArray switches{"Off","On"};
const auto quiet = juce::dontSendNotification;
juce::String layerPrefix(int layer) { return "layer" + juce::String(layer) + "_"; }
juce::String dialKey(int parameter) { return "dial" + juce::String(parameter); }
}

void MainComponent::makeCatalogue()
{
    juce::String section="Performance", prefix;
    auto add=[&](juce::String key, juce::String label, double lo, double hi, double step, juce::String unit,
                 juce::StringArray choices, std::function<double()> read, std::function<void(double)> write) {
        catalogue.push_back({prefix+key,section,label,unit,lo,hi,step,std::move(choices),std::move(read),std::move(write)});
    };
    auto bind=[&](juce::String key,juce::String name,auto& value,double lo,double hi,double step=1,
                  juce::String unit={},juce::StringArray choices={}) {
        add(key,name,lo,hi,step,unit,std::move(choices),[&value] { return static_cast<double>(value); },
            [&value](double x) { value=static_cast<decltype(+value)>(x); });
    };
    bind("bpm","Tempo",pattern.bpm,1,180,1," BPM");
    bind("melody","Melody events",pattern.voiceA,2,9);
    bind("bass","Bass events",pattern.voiceB,2,9);
    bind("seed","Composition seed",engine.seed,0,999998);
    bind("transpose","Master pitch",engine.masterTranspose,-12,12,1," st");
    add("level","Master level",0,100,1,"%",{},[this]{ return volumeSlider.getValue()*100; },
        [this](double n){ volumeSlider.setValue(n/100,quiet); engine.setMasterVolume(float(n/100)); });
    bind("shape","Chord voicing",engine.chordShape,0,4,1,"",{"Seventh","Triad","First inversion","Sus 2","Stacked fourths"});
    bind("touch","Chord articulation",engine.chordTouch,0,2,1,"",{"One tone","Block","Rolled"});
    section="Nodes & stage";
    bind("nodes","Nodes per layer",engine.nodesPerLayer,1,studio::BeatGrid::maxBodies);
    bind("passes","Reunion after",engine.reunionPasses,2,16,1," passes");
    bind("travel","Node motion",scene.equalSpeed,0,1,1,"",{"Variable speed","Equal speed / variable distance"});
    bind("orbitScale","Orbit distance",scene.orbitScale,25,800,1,"%");
    bind("pointCoverage","Point coverage",scene.pointCoverage,0,100,1,"%");
    bind("balance","Auto-balance paths",scene.autoBalance,0,1,1,"",switches);
    bind("offset","Travel offset",scene.timingOffset,-100,100,1,"%");
    bind("spread","Fan spread",scene.fanSpread,0,100,1,"%");
    bind("track","Note track",scene.showForecast,0,1,1,"",switches);
    bind("trackHeight","Note track height",scene.trackHeight,48,180,1," px");
    auto combo=[&](juce::String key,juce::String label,juce::ComboBox& control,juce::StringArray choices) {
        int size=choices.size();
        add(key,label,1,size,1,"",std::move(choices),[&control]{return double(control.getSelectedId());},
            [&control](double id){control.setSelectedId(int(id),quiet);});
    };
    combo("environment","Environment",environmentBox,environmentNames);
    combo("target","Convergence",targetBox,{"Line","Point"});
    combo("meeting","Skip destination",meetingBox,{"Every point","Layer attacks"});

    juce::StringArray sounds;
    for(auto name:studio::sounds) sounds.add(name);
    for(int l=0;l<studio::layerCount;++l) {
        section=studio::layers[l]; prefix=layerPrefix(l);
        auto& strip=engine.strips[l];
        bind("on","Layer enabled",strip.enabled,0,1,1,"",switches);
        bind("voice","Instrument",strip.sound,0,sounds.size()-1,1,"",sounds);
        bind("order","Pitch order",strip.pitchMode,0,3,1,"",{"Ascending","Descending","Seeded","Ensemble"});
        bind("copies","Nodes per pitch",strip.nodesPerPitch,1,studio::BeatGrid::maxBodies);
        for(int i=0;i<studio::parameterCount;++i) {
            const auto& d=studio::dials[i];
            bind(dialKey(i),d.name,strip.values[i],d.min,d.max,d.step,d.unit,i==studio::reverse ? switches : juce::StringArray{});
        }
    }
    section="Path placement";
    for(int l=0;l<studio::layerCount;++l) {
        prefix=layerPrefix(l);
        bind("line",juce::String(studio::layers[l])+" on line",scene.linePlacement[l],-100,100,1,"%");
        bind("point",juce::String(studio::layers[l])+" around point",scene.pointPlacement[l],-100,100,1,"%");
    }
    for(int i=0;i<3;++i) {
        section=environmentNames[i]; prefix="sky"+juce::String(i)+"_";
        auto& look=scene.settingsFor(i);
        const juce::StringArray names{"Sky hue","Stars","Note sparks","Sky drift",scene.details[i][0],scene.details[i][1],"Star twinkle"};
        for(int k=0;k<7;++k)
            bind("look"+juce::String(k),names[k],look[k],0,k==0?359:k==1?RhythmScene::maxStars:100);
        auto& motion=scene.motionFor(i);
        bind("cycle","Cycle hue",motion.hueCycle,0,1,1,"",switches);
        bind("speed","Hue cycle speed",motion.hueSpeed,0,2,1,"",{"Slow","Medium","Fast"});
        bind("range","Hue range",motion.hueRange,0,180,1," deg");
        bind("camera","Parallax",motion.camera,0,2,1,"",{"Fixed","Drifting","Follow pointer"});
        bind("depth","Parallax depth",motion.depth,0,100,1,"%");
    }
    for(const auto& setting:catalogue)
        recipe.push_back({false,setting.low,setting.high});
    factory=captureStudio(catalogue);
    defaults=readStudioFile(userSettings.getChildFile("defaults.xml"));
    if(!defaults.hasType("Defaults")) defaults=StudioState("Defaults");
}

void MainComponent::recall(const StudioState& state,bool restart)
{
    if(!state.isValid()) return;
    {
        const juce::ScopedLock lock(audioDeviceManager.getAudioCallbackLock());
        if(state.hasType("Patch") && state.hasProperty("layer0_on") && !state.hasProperty("layer4_on"))
            engine.strips[4].enabled = false; // Performances saved before Synth existed.
        for(const auto& setting:catalogue) {
            if(!state.hasProperty(setting.id)) continue;
            const double value=state.getProperty(setting.id);
            if(std::isfinite(value)) setting.write(setting.snap(value));
        }
        voiceABox.setSelectedId(pattern.voiceA,quiet); voiceBBox.setSelectedId(pattern.voiceB,quiet);
        tempoSlider.setValue(pattern.bpm,quiet); transposeSlider.setValue(engine.masterTranspose.load(),quiet);
        engine.setPattern(pattern.voiceA,pattern.voiceB,pattern.bpm);
        if(restart) jumpTo(0);
    }
    int ratio=0;
    for(size_t i=0;i<rhythmPresets.size();++i)
        if(rhythmPresets[i].voiceA==pattern.voiceA && rhythmPresets[i].voiceB==pattern.voiceB) ratio=int(i)+1;
    if(restart || state.hasProperty("melody") || state.hasProperty("bass")) presetBox.setSelectedId(ratio,quiet);
    for(int l=0;l<studio::layerCount;++l) layerButtons[l].setToggleState(engine.strips[l].enabled.load(),quiet);
    track.setToggleState(scene.showForecast,quiet);
    if(scene.activeEnvironment()!=environmentBox.getSelectedId()-1) scene.setEnvironment(environmentBox.getSelectedId()-1);
    scene.setPointTarget(targetBox.getSelectedId()==2);
    scene.setGrid(engine.grid());
    updatePatternFromControls();
    inspector.syncControls();
}

void MainComponent::randomize(bool everything)
{
    StudioState patch("Patch");
    auto& rng=juce::Random::getSystemRandom();
    for(size_t i=0;i<catalogue.size();++i) {
        const auto& s=catalogue[i]; const auto& rule=recipe[i];
        const bool protectedMix=s.id=="bpm" || s.id=="level" || s.id=="transpose" || s.id.endsWith("_on") ||
            s.id.endsWith("_"+dialKey(studio::pitch)) || s.id.endsWith("_"+dialKey(studio::octave));
        if(everything ? protectedMix : !rule.enabled) continue;
        double value=s.draw(rng,everything?s.low:rule.low,everything?s.high:rule.high);
        if(!everything && !s.choices.isEmpty()) {
            juce::Array<int> candidates;
            for(int k=0;k<s.choices.size();++k) if(rule.allowed&(1u<<k)) candidates.add(k);
            if(candidates.isEmpty()) continue;
            value=s.low+candidates[rng.nextInt(candidates.size())];
        }
        patch.setProperty(s.id,value,nullptr);
    }
    if(!patch.getNumProperties()) return;
    recall(patch,everything);
    ratioTitle.setText(everything?"Full variation":"Custom variation",quiet);
}

void MainComponent::showSettings(bool editDefaults)
{
    auto initial=recipe;
    if(editDefaults)
        for(size_t i=0;i<catalogue.size();++i) {
            const auto& s=catalogue[i];
            const double val=s.snap(double(defaults.getProperty(s.id,s.read())));
            initial[i]={defaults.hasProperty(s.id),val,val};
        }
    sheet=std::make_unique<SettingsSheet>(catalogue,std::move(initial),editDefaults,
        [this,editDefaults](const std::vector<RandomRule>& values) {
            if(editDefaults) {
                StudioState saved("Defaults");
                for(size_t i=0;i<values.size();++i)
                    if(values[i].enabled) saved.setProperty(catalogue[i].id,values[i].low,nullptr);
                if(!storeStudioFile(userSettings.getChildFile("defaults.xml"),saved)) {
                    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Defaults not saved","Could not write the settings file.");
                    return;
                }
                defaults=saved;

            } else { recipe=values; randomize(false); }
            sheet->setVisible(false);
        },[this]{sheet->setVisible(false);});
    addAndMakeVisible(*sheet);
    sheet->setBounds(getLocalBounds()); sheet->enterModalState(true,nullptr,false);
}

StudioState MainComponent::factoryPatch(int index)
{
    auto patch=factory.createCopy();
    // Each row describes an arrangement, not just a ratio or a reverb patch.
    struct Arrangement { int bpm,a,b,seed,key,env,nodes,shape,touch; std::array<int,4> voice,order,pitch,wet; };
    constexpr Arrangement arrangements[]{
        {82,5,3,2371,-2,1,9,2,1,{9,7,3,10},{0,0,1,2},{0,0,-2,7},{22,8,35,42}},
        {72,7,4,6142,3,3,12,3,2,{8,6,1,4},{1,0,0,2},{0,-2,0,5},{38,14,49,64}},
        {108,8,5,9810,0,2,10,4,2,{1,5,9,10},{2,1,2,0},{7,0,0,0},{24,11,37,52}}
    };
    auto set=[&](const juce::String& key,int v){patch.setProperty(key,v,nullptr);};
    if(index>0) {
        auto& a=arrangements[index-1];
        set("bpm",a.bpm); set("melody",a.a); set("bass",a.b); set("seed",a.seed); set("transpose",a.key);
        set("environment",a.env); set("nodes",a.nodes); set("shape",a.shape); set("touch",a.touch);
        for(int l=0;l<int(a.voice.size());++l) {
            auto p=layerPrefix(l); set(p+"voice",a.voice[l]); set(p+"order",a.order[l]);
            set(p+dialKey(studio::pitch),a.pitch[l]); set(p+dialKey(studio::wet),a.wet[l]);
        }
    }
    return patch;
}

void MainComponent::loadFactory(int index)
{
    recall(factoryPatch(index),true);
    ratioTitle.setText(factoryNames[index],quiet);
}

void MainComponent::chooseStartupPreset()
{
    juce::PopupMenu menu;
    juce::Array<StudioState> patches;
    for(int i=0;i<factoryNames.size();++i) {
        patches.add(factoryPatch(i));
        menu.addItem(patches.size(),factoryNames[i]);
    }
    for(const auto& file:userSettings.getChildFile("Presets").findChildFiles(juce::File::findFiles,false,"*.xml")) {
        auto patch=readStudioFile(file);
        if(!patch.hasType("Patch")) continue;
        patches.add(patch);menu.addItem(patches.size(),patch["name"].toString());
    }
    juce::Component::SafePointer<MainComponent> self(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&menus[3]),[self,patches](int id) {
        if(!self || !juce::isPositiveAndBelow(id-1,patches.size())) return;
        StudioState saved("Defaults");
        saved.copyPropertiesFrom(self->factory,nullptr);
        saved.copyPropertiesFrom(patches[id-1],nullptr);
        if(storeStudioFile(self->userSettings.getChildFile("defaults.xml"),saved))
            self->defaults=saved;
        else juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                    "Defaults not saved","Could not write startup settings.");
    });
}

void MainComponent::showPresets()
{
    juce::PopupMenu menu;
    menu.addSectionHeader("Factory performances");
    for(int i=0;i<factoryNames.size();++i) menu.addItem(i+1,factoryNames[i]);
    juce::Array<StudioState> saved;
    auto files=userSettings.getChildFile("Presets").findChildFiles(juce::File::findFiles,false,"*.xml");
    files.sort();
    for(auto& file:files) {
        auto patch=readStudioFile(file);
        if(!patch.hasType("Patch")) continue;
        if(saved.isEmpty()) menu.addSectionHeader("Your performances");
        saved.add(patch);
        menu.addItem(100+saved.size(),patch["name"].toString());
    }
    menu.addSeparator(); menu.addItem(99,"Save current performance...");
    juce::Component::SafePointer<MainComponent> self(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&presets),[self,saved](int id) {
        if(!self || id==0) return;
        if(id==99) self->savePreset();
        else if(id>=101) {
            auto patch=saved[id-101]; self->recall(patch,true); self->ratioTitle.setText(patch["name"].toString(),quiet);
        } else self->loadFactory(id-1);
    });
}

void MainComponent::savePreset()
{
    auto dialog=std::make_unique<juce::AlertWindow>("Save performance","Name this instrument and scene.",juce::MessageBoxIconType::NoIcon,this);
    dialog->addTextEditor("name",ratioTitle.getText(),"Name");
    dialog->addButton("Save",1,juce::KeyPress(juce::KeyPress::returnKey));
    dialog->addButton("Cancel",0,juce::KeyPress(juce::KeyPress::escapeKey));
    juce::Component::SafePointer<MainComponent> self(this);
    auto* prompt=dialog.release();
    prompt->enterModalState(true,juce::ModalCallbackFunction::create([self,prompt](int id) {
        if(!self || id!=1) return;
        auto name=prompt->getTextEditorContents("name").trim();
        if(name.isEmpty()) name="Untitled performance";
        auto patch=captureStudio(self->catalogue);
        patch.setProperty("name",name,nullptr);
        auto file=self->userSettings.getChildFile("Presets").getChildFile(juce::Uuid().toString()+".xml");
        if(storeStudioFile(file,patch)) self->ratioTitle.setText(name,quiet);
        else juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Preset not saved","Could not write the preset file.");
    }),true);
}


void MainComponent::checkpoint()
{
    if(catalogue.empty()) return;
    if(historyIndex>=0) {
        const auto& last=history[historyIndex];
        if(last["name"].toString()==ratioTitle.getText() &&
           std::all_of(catalogue.begin(),catalogue.end(),[&](const auto& s){return double(last[s.id])==s.read();})) return;
    }
    auto state=captureStudio(catalogue);
    state.setProperty("name",ratioTitle.getText(),nullptr);
    if(historyIndex>=0 && juce::Logger::getCurrentLogger()!=nullptr) {
        juce::StringArray edits;
        for(const auto& setting:catalogue)
            if(double(history[historyIndex][setting.id])!=double(state[setting.id]))
                edits.add(setting.id+"="+state[setting.id].toString());
        if(!edits.isEmpty()) juce::Logger::writeToLog(juce::Time::getCurrentTime().formatted("%H:%M:%S")+" "+edits.joinIntoString(" "));
    }
    history.resize(historyIndex+1); history.push_back(state);
    if(history.size()>100) history.erase(history.begin());
    historyIndex=int(history.size())-1;
}

void MainComponent::travelHistory(int direction)
{
    checkpoint();
    int next=historyIndex+direction;
    if(next<0 || next>=int(history.size())) return;
    historyIndex=next;
    recall(history[next],false);
    // Recall snaps stored floats to the precision of the physical controls.
    auto name=history[next]["name"].toString();
    history[next]=captureStudio(catalogue); history[next].setProperty("name",name,nullptr);
    ratioTitle.setText(name,quiet);
}

void MainComponent::mouseDown(const juce::MouseEvent& e)
{
    auto* c=e.eventComponent;
    if(!c) return;
    auto* slider=dynamic_cast<juce::Slider*>(c);
    if(!slider) slider=c->findParentComponentOfClass<juce::Slider>();
    if(slider) lastSlider=slider;
    checkpoint();
}

void MainComponent::warnRoll()
{
    if(!warn){randomize(true);return;}
    auto* prompt=new juce::AlertWindow("Randomize performance?",
        "Tempo, master level, pitch and octave offsets, and the layer switches stay put. Other settings can change dramatically and may not sound pleasant. To keep this version, go back and save it under Presets first.",
        juce::MessageBoxIconType::NoIcon,this);
    auto* skip=new juce::ToggleButton("Do not show this warning again");
    skip->setSize(290,30); prompt->addCustomComponent(skip);
    prompt->addButton("Go back",0,juce::KeyPress(juce::KeyPress::escapeKey));
    prompt->addButton("Randomize",1);
    juce::Component::SafePointer<MainComponent> self(this);
    prompt->enterModalState(true,juce::ModalCallbackFunction::create([self,skip](int id){
        std::unique_ptr<juce::ToggleButton> owned(skip);
        if(!self || !id) return;
        if(skip->getToggleState()){
            StudioState pref("Preferences");pref.setProperty("warn",0,nullptr);
            self->warn=!storeStudioFile(self->userSettings.getChildFile("preferences.xml"),pref);
        }
        self->randomize(true);
    }),true);
}

void MainComponent::customRhythm()
{
    auto* dialog=new juce::AlertWindow("Custom rhythm","Choose the pulse counts and groove. Instruments, effects and tempo stay unchanged.",juce::MessageBoxIconType::NoIcon,this);
    juce::StringArray events;
    for(int i=2;i<=9;++i)events.add(juce::String(i));
    dialog->addComboBox("a",events,"Melody events"); dialog->getComboBoxComponent("a")->setSelectedId(pattern.voiceA-1);
    dialog->addComboBox("b",events,"Bass events"); dialog->getComboBoxComponent("b")->setSelectedId(pattern.voiceB-1);
    juce::StringArray grooves; for(auto g:studio::BeatGrid::grooves)grooves.add(g);
    dialog->addComboBox("g",grooves,"Groove"); dialog->getComboBoxComponent("g")->setSelectedId(studio::BeatGrid::groove(engine.seed)+1);
    dialog->addComboBox("motion",{"Variable speed","Equal speed / variable distance"},"Node motion");
    dialog->getComboBoxComponent("motion")->setSelectedId(scene.equalSpeed ? 2 : 1);
    dialog->addButton("Generate",1); dialog->addButton("Cancel",0,juce::KeyPress(juce::KeyPress::escapeKey));
    juce::Component::SafePointer<MainComponent> self(this);
    dialog->enterModalState(true,juce::ModalCallbackFunction::create([self,dialog](int id){
        if(!id || !self)return;
        StudioState patch("Patch");
        patch.setProperty("melody",dialog->getComboBoxComponent("a")->getSelectedId()+1,nullptr);
        patch.setProperty("bass",dialog->getComboBoxComponent("b")->getSelectedId()+1,nullptr);
        int groove=dialog->getComboBoxComponent("g")->getSelectedId()-1;
        int seed=4*juce::Random::getSystemRandom().nextInt(249999)+(groove+1)%4;
        patch.setProperty("seed",seed,nullptr);
        patch.setProperty("travel",dialog->getComboBoxComponent("motion")->getSelectedId()==2,nullptr);
        self->recall(patch,true);self->ratioTitle.setText("Custom rhythm",quiet);
    }),true);
}

void MainComponent::managePreset(juce::File file)
{
    auto patch=readStudioFile(file); if(!patch.hasType("Patch"))return;
    auto* dialog=new juce::AlertWindow("Edit saved preset",
        "Rename changes only the name. Update saves your current instrument and scene into this preset.",
        juce::MessageBoxIconType::NoIcon,this);
    dialog->addTextEditor("name",patch["name"].toString(),"Preset name");
    dialog->addButton("Rename",1);dialog->addButton("Update",2);dialog->addButton("Remove...",3);dialog->addButton("Cancel",0);
    juce::Component::SafePointer<MainComponent> self(this);
    dialog->enterModalState(true,juce::ModalCallbackFunction::create([self,dialog,file,patch](int id) mutable {
        if(!self || !id)return;
        if(id==3){
            juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon,"Remove saved preset?",
                patch["name"].toString(),"Move to Recycle Bin","Keep",self,juce::ModalCallbackFunction::create([file](int ok){
                    if(ok && !file.moveToTrash())juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Remove failed","The preset was not removed.");
                }));return;
        }
        auto name=dialog->getTextEditorContents("name").trim();
        if(name.isEmpty())name=patch["name"].toString();
        if(id==2)patch=captureStudio(self->catalogue);
        patch.setProperty("name",name,nullptr);
        if(!storeStudioFile(file,patch))juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Preset not saved","Could not update the preset file.");
    }),true);
}

void MainComponent::menuFor(int index)
{
    checkpoint();
    juce::PopupMenu menu;
    juce::Array<juce::File> files;
    if(index==0){
        menu.addItem(1,"Load performance..."); menu.addItem(2,"Save current performance...");
        juce::PopupMenu edit;
        for(auto file:userSettings.getChildFile("Presets").findChildFiles(juce::File::findFiles,false,"*.xml")){
            auto p=readStudioFile(file); if(!p.hasType("Patch"))continue;
            files.add(file);edit.addItem(100+files.size(),p["name"].toString());
        }
        menu.addSubMenu("Manage saved presets",edit,!files.isEmpty());
    } else if(index==1){
        menu.addItem(3,"Undo    Ctrl+Z",historyIndex>0); menu.addItem(4,"Redo    Ctrl+Y",historyIndex+1<int(history.size()));
    } else if(index==2){
        menu.addItem(5,"Full screen    F11",true,juce::Desktop::getInstance().getKioskModeComponent()!=nullptr);
        menu.addItem(6,"Note track",true,scene.showForecast);
        menu.addItem(10,"Reset panel sizes");
        menu.addItem(17,"Equal-speed nodes",true,scene.equalSpeed);
        menu.addItem(18,"Auto-balance paths",true,scene.autoBalance);
        menu.addItem(19,"Adjust path placement...");
        menu.addItem(11,"Show stage FPS",true,scene.showFps);
        menu.addItem(16,"Cache scenery for smoother nodes",true,scene.cacheSky);
        if(auto* peer=getPeer()){
            juce::PopupMenu renderers;
            auto names=peer->getAvailableRenderingEngines();
            for(int i=0;i<names.size();++i) renderers.addItem(20+i,names[i],true,peer->getCurrentRenderingEngine()==i);
            menu.addSubMenu("Rendering engine",renderers);
        }
    } else {
        menu.addItem(6,"Show note track",true,scene.showForecast);
        juce::PopupMenu sizes;
        for(int i=0;i<4;++i){int px=48+40*i;sizes.addItem(12+i,juce::String(px)+" px",true,int(scene.trackHeight)==px);}
        menu.addSubMenu("Note track height",sizes);
        menu.addItem(7,"Save startup defaults...");menu.addItem(8,"Load defaults",defaults.getNumProperties()>0);
        menu.addItem(9,"Warn before randomizing",true,warn);
        menu.addItem(61,"Open diagnostics folder");
        menu.addItem(60,"Use preset as startup defaults...");
    }
    juce::Component::SafePointer<MainComponent> self(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&menus[index]),[self,files](int id){
        if(!self)return;
        if(id>=101){self->managePreset(files[id-101]);return;}
        if(id>=12 && id<=15){self->scene.trackHeight=float(48+40*(id-12));self->scene.repaint();return;}
        if(id==61){
            juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                .getChildFile("PolyRhythm Pro/Diagnostics").startAsProcess();return;
        }
        if(id==60){self->chooseStartupPreset();return;}
        if(id>=20){if(auto* peer=self->getPeer())peer->setCurrentRenderingEngine(id-20);return;}
        switch(id){
        case 1:self->showPresets();break;
        case 2:self->savePreset();break;
        case 3:self->travelHistory(-1);break;
        case 4:self->travelHistory(1);break;
        case 5:self->toggleFullscreen();break;
        case 6:self->track.triggerClick();break;
        case 7:self->showSettings(true);break;
        case 8:self->recall(self->defaults,false);break;
        case 9:{StudioState pref("Preferences");pref.setProperty("warn",int(!self->warn),nullptr);
                if(storeStudioFile(self->userSettings.getChildFile("preferences.xml"),pref))self->warn=!self->warn;break;}
        case 10:self->sideWidth=374;self->dockHeight=166;self->resized();self->repaint();break;
        case 11:self->scene.showFps=!self->scene.showFps;self->scene.repaint();break;
        case 17:self->scene.equalSpeed=!self->scene.equalSpeed;self->inspector.syncControls();self->scene.repaint();break;
        case 18:self->scene.autoBalance=!self->scene.autoBalance;self->inspector.syncControls();self->scene.repaint();break;
        case 19:{
            auto* dialog=new juce::AlertWindow("Path placement","Offset each layer on the line or rotate its paths around the point.",
                                             juce::MessageBoxIconType::NoIcon,self);
            for(int i=0;i<studio::layerCount;++i) {
                auto label=juce::String(studio::layers[i]);
                dialog->addTextEditor(layerPrefix(i)+"line",juce::String(self->scene.linePlacement[i],0),label+" on line (%)");
                dialog->addTextEditor(layerPrefix(i)+"point",juce::String(self->scene.pointPlacement[i],0),label+" around point (%)");
            }
            dialog->addButton("Apply",1);dialog->addButton("Cancel",0,juce::KeyPress(juce::KeyPress::escapeKey));
            dialog->enterModalState(true,juce::ModalCallbackFunction::create([self,dialog](int ok) {
                if(!ok || !self) return;
                auto patch=StudioState("Patch");
                for(int i=0;i<studio::layerCount;++i)
                    for(const auto* key:{"line","point"}) {
                        const auto id=layerPrefix(i)+key;
                        auto input=dialog->getTextEditorContents(id).trim();
                        if(input.isNotEmpty() && input.containsOnly("-0123456789"))
                            patch.setProperty(id,input.getIntValue(),nullptr);
                    }
                self->recall(patch,false);self->scene.repaint();
            }),true);
            break;
        }
        case 16:self->scene.cacheSky=!self->scene.cacheSky;self->scene.repaint();break;
        }
    });
}
