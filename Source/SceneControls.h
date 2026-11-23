#pragma once
#include "RhythmScene.h"
#include "TransportAudioEngine.h"

class SceneControls final : public juce::Component
{
public:
    SceneControls(TransportAudioEngine& audio,RhythmScene& canvas) : engine(audio),scene(canvas)
    {
        bindSlider(nodeCount,"Nodes per layer",1,studio::BeatGrid::maxBodies,"");
        bindSlider(offset,"Travel offset",-100,100,"%");
        bindSlider(spread,"Fan spread",0,100,"%");
        bindSlider(reunion,"Reunion after",2,16," passes");
        bindSlider(range,"Hue range",0,180," deg");
        bindSlider(depth,"Parallax depth",0,100,"%");
        bindSlider(tempo,"Travel tempo",1,180," BPM");
        bindSlider(distance,"Orbit distance",25,800,"%");
        bindSlider(coverage,"Point coverage",0,100,"%");
        bindSlider(placement,"Placement",-100,100,"%");
        for(auto name:studio::layers) lane.addItem(name,lane.getNumItems()+1);
        lane.setSelectedId(1,juce::dontSendNotification);addAndMakeVisible(lane);
        balance.setButtonText("Auto-balance active paths");addAndMakeVisible(balance);
        balance.onClick=[this]{scene.autoBalance=balance.getToggleState();scene.repaint();};
        lane.onChange=[this]{show(nodes);};
        distance.onValueChange=[this]{scene.orbitScale=float(distance.getValue());scene.repaint();};
        coverage.onValueChange=[this]{scene.pointCoverage=float(coverage.getValue());scene.repaint();};
        placement.onValueChange=[this]{
            auto& offsets=scene.usesPoint()?scene.pointPlacement:scene.linePlacement;
            offsets[lane.getSelectedId()-1]=float(placement.getValue());scene.repaint();
        };
        travel.addItemList({"Variable speed","Equal speed / variable distance"},1);
        travel.setTitle("Node motion");addAndMakeVisible(travel);
        travel.onChange=[this]{scene.equalSpeed=travel.getSelectedId()==2;show(nodes);scene.repaint();};
        hue.setButtonText("Cycle background hue"); addAndMakeVisible(hue);
        speed.addItemList({"Slow (120 s)","Medium (48 s)","Fast (16 s)"},1);
        camera.addItemList({"Fixed camera","Drifting camera","Follow pointer"},1);
        speed.setTitle("Hue cycle speed"); camera.setTitle("Parallax mode");
        addAndMakeVisible(speed); addAndMakeVisible(camera);
        nodeCount.onValueChange=[this]{
            engine.nodesPerLayer=static_cast<int>(nodeCount.getValue()); scene.setGrid(engine.grid()); scene.repaint();
        };
        offset.onValueChange=[this]{ scene.timingOffset=static_cast<float>(offset.getValue());scene.repaint(); };
        spread.onValueChange=[this]{ scene.fanSpread=static_cast<float>(spread.getValue());scene.repaint(); };
        reunion.onValueChange=[this]{
            engine.reunionPasses=static_cast<int>(reunion.getValue());
            scene.setGrid(engine.grid()); scene.repaint();
        };
        hue.onClick=[this]{scene.motion().hueCycle=hue.getToggleState(); show(nodes);};
        range.onValueChange=[this]{scene.motion().hueRange=static_cast<float>(range.getValue());scene.repaint();};
        speed.onChange=[this]{scene.motion().hueSpeed=speed.getSelectedId()-1;};
        camera.onChange=[this]{scene.motion().camera=camera.getSelectedId()-1;show(nodes);};
        depth.onValueChange=[this]{scene.motion().depth=static_cast<float>(depth.getValue());scene.repaint();};
    }
    void linkTempo(juce::Slider& master){tempo.getValueObject().referTo(master.getValueObject());}
    void show(bool nodePage)
    {
        nodes=nodePage;
        const auto& motion=scene.motion();
        constexpr auto quiet=juce::dontSendNotification;
        nodeCount.setValue(engine.nodesPerLayer.load(),quiet); offset.setValue(scene.timingOffset,quiet);
        reunion.setValue(engine.reunionPasses.load(),quiet); reunion.setVisible(nodes);
        spread.setValue(scene.fanSpread,quiet); hue.setToggleState(motion.hueCycle,quiet);
        speed.setSelectedId(motion.hueSpeed+1,quiet); range.setValue(motion.hueRange,quiet);
        camera.setSelectedId(motion.camera+1,quiet); depth.setValue(motion.depth,quiet);
        travel.setSelectedId(scene.equalSpeed?2:1,quiet);travel.setVisible(nodes);
        tempo.setVisible(nodes);balance.setVisible(nodes);balance.setToggleState(scene.autoBalance,quiet);
        distance.setVisible(nodes && scene.equalSpeed);distance.setValue(scene.orbitScale,quiet);
        coverage.setVisible(nodes && scene.equalSpeed && scene.usesPoint());coverage.setValue(scene.pointCoverage,quiet);
        lane.setVisible(nodes);placement.setVisible(nodes);
        placement.setTitle(scene.usesPoint()?"Point rotation":"Line offset");
        placement.setValue((scene.usesPoint()?scene.pointPlacement:scene.linePlacement)[lane.getSelectedId()-1],quiet);
        nodeCount.setVisible(nodes); offset.setVisible(nodes && !scene.equalSpeed); spread.setVisible(nodes && !coverage.isVisible());
        hue.setVisible(!nodes); range.setVisible(!nodes&&motion.hueCycle); speed.setVisible(range.isVisible());
        camera.setVisible(!nodes); depth.setVisible(!nodes&&motion.camera!=0);
        resized(); repaint();
    }
    void resized() override
    {
        auto rows=getLocalBounds();
        auto row=[&](juce::Component& control,bool labelled) {
            if(!control.isVisible()) return;
            auto bounds=rows.removeFromTop(33).reduced(2,2);
            if(labelled) bounds.removeFromLeft(112);
            control.setBounds(bounds);
        };
        row(travel,true);row(nodeCount,true);row(offset,true);row(spread,true);row(reunion,true);
        row(tempo,true);row(distance,true);row(coverage,true);row(balance,false);row(lane,true);row(placement,true);
        row(hue,false);row(speed,true);row(range,true);row(camera,true);row(depth,true);
    }
    void paint(juce::Graphics& g) override
    {
        g.setFont(juce::FontOptions(13.f));g.setColour(juce::Colour(0xffdae0ed));
        for(auto* s:{&nodeCount,&offset,&spread,&reunion,&range,&depth,&tempo,&distance,&coverage,&placement})
            if(s->isVisible()) g.drawText(s->getTitle(),3,s->getY(),109,s->getHeight(),juce::Justification::centredLeft);
        if(travel.isVisible()) g.drawText("Node motion",3,travel.getY(),109,travel.getHeight(),juce::Justification::centredLeft);
        if(lane.isVisible()) g.drawText("Place layer",3,lane.getY(),109,lane.getHeight(),juce::Justification::centredLeft);
        if(speed.isVisible()) g.drawText("Cycle speed",3,speed.getY(),109,speed.getHeight(),juce::Justification::centredLeft);
        if(camera.isVisible()) g.drawText("Parallax",3,camera.getY(),109,camera.getHeight(),juce::Justification::centredLeft);
    }
private:
    void bindSlider(juce::Slider& s,const char* name,double lo,double hi,const char* suffix)
    {
        s.setTitle(name); s.setRange(lo,hi,1);s.setTextBoxStyle(juce::Slider::TextBoxRight,false,55,21);
        s.setTextValueSuffix(suffix); addAndMakeVisible(s);
    }
    TransportAudioEngine& engine; RhythmScene& scene;
    juce::ToggleButton balance;juce::ComboBox lane;
    juce::Slider tempo,distance,coverage,placement;
    juce::Slider nodeCount,offset,spread,reunion,range,depth;
    juce::ToggleButton hue; juce::ComboBox speed,camera,travel;
    bool nodes=false;
};
