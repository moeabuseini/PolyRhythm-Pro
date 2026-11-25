#pragma once
#include "../UI/GlassTheme.h"
#include "../Studio/StudioSettings.h"

inline StudioState videoCue(const juce::String& key,int frame,int length,double value)
{
    StudioState c("Cue");
    c.setProperty("key",key,nullptr); c.setProperty("frame",frame,nullptr);
    c.setProperty("length",length,nullptr); c.setProperty("value",value,nullptr);
    return c;
}

class VideoEditor final : public juce::Component
{
public:
    StudioState sequence;
    juce::ComboBox library, cues, category, parameter;
    juce::TextEditor name, caption;
    juce::Slider frame,length,value,duration;
    juce::ComboBox choice;
    juce::ToggleButton loop{"Loop preview"};
    juce::ToggleButton reunion{"Finish at next full reunion"};
    juce::TextButton create{"New from current"}, add{"Add cue"}, replace{"Update cue"}, remove{"Remove cue"},
                     save{"Save"}, saveCopy{"Save copy"}, trash{"Remove saved..."}, preview{"Preview"}, close{"Close"},
                     importPreset{"Import..."}, exportPreset{"Export..."}, edit{"Edit timeline"}, opening{"Use current as opening"};
    juce::ToggleButton showCaption{"Caption"};
    juce::Label message;
    VideoEditor(const std::vector<StudioSetting>& specs):settings(specs)
    {
        for(juce::Component* c:std::initializer_list<juce::Component*>{&library,&name,&cues,&category,&parameter,&frame,&length,&value,&choice,&duration,&caption,&showCaption,&reunion,&create,&add,&replace,&remove,&save,&saveCopy,&trash,&preview,&close,&message,&edit,&loop,&importPreset,&exportPreset,&opening,&track})
            addAndMakeVisible(c);
        for(const auto& setting:settings)
            if(!sections.contains(setting.section)) sections.add(setting.section);
        category.addItemList(sections,1);
        category.setSelectedId(1,juce::dontSendNotification);
        category.onChange=[this]{ pickCategory(); configureValue(); };
        pickCategory();
        for(auto* slider:{&frame,&length,&duration,&value})
            slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,84,24);
        for(auto* slider:{&frame,&length,&duration}) {
            slider->setNumDecimalPlacesToDisplay(0);
            slider->textFromValueFunction=[](double frames){return juce::String(frames/30.,2)+" s";};
            slider->valueFromTextFunction=[](const juce::String& text){return std::round(text.getDoubleValue()*30);};
        }
        duration.setSliderStyle(juce::Slider::IncDecButtons);
        duration.setIncDecButtonsMode(juce::Slider::incDecButtonsDraggable_Horizontal);
        duration.onValueChange=[this]{
            sequence.setProperty("duration",int(duration.getValue()),nullptr);
            updateBounds();
        };
        caption.setTextToShowWhenEmpty("Leave blank for automatic wording",GlassTheme::current().colours().muted());
        showCaption.onClick=[this]{caption.setEnabled(showCaption.getToggleState());};
        track.select=[this](int id){cues.setSelectedId(id,juce::sendNotificationSync);};
        track.seek=[this](int position){frame.setValue(position);};
        edit.onClick=[this]{setEditing(!editing);};
        parameter.onChange=[this]{ configureValue(); };
        cues.onChange=[this]{ editSelected(); };
        add.onClick=[this]{commit(false);};replace.onClick=[this]{commit(true);};
        remove.onClick=[this]{int i=cues.getSelectedId(); if(i>0){sequence.removeChild(i,nullptr);list(i-1);}};
        loop.onClick=[this]{sequence.setProperty("loop",loop.getToggleState(),nullptr);};
        reunion.onClick=[this]{sequence.setProperty("reunion",reunion.getToggleState(),nullptr);};
        name.onTextChange=[this]{sequence.setProperty("name",name.getText(),nullptr);};
    }
    void load(StudioState s)
    {
        sequence=s.createCopy();name.setText(s["name"].toString(),false);
        loop.setToggleState(bool(s["loop"]),juce::dontSendNotification);
        reunion.setToggleState(bool(s["reunion"]),juce::dontSendNotification);
        updateBounds(); configureValue(); list(1);
        if(cues.getNumItems()==0) {
            frame.setValue(0);length.setValue(0);showCaption.setToggleState(true,juce::dontSendNotification);
            caption.clear();caption.setEnabled(true);
        }
        message.setText("Space pauses the preview. Esc returns here.",juce::dontSendNotification);
        setEditing(editing);
    }
    void setEditing(bool expanded)
    {
        editing=expanded;
        for(auto* control:std::initializer_list<juce::Component*>{&name,&cues,&category,&parameter,&frame,&length,&duration,
                            &caption,&showCaption,&reunion,&add,&replace,&remove,&opening,&track})
            control->setVisible(editing);
        save.setVisible(editing && sequence.hasProperty("file"));
        saveCopy.setVisible(editing);
        saveCopy.setButtonText(sequence.hasProperty("file")?"Save copy":"Save as preset");
        trash.setVisible(editing && sequence.hasProperty("file"));
        edit.setButtonText(editing?"Hide timeline":"Edit timeline");
        configureValue();resized();repaint();
    }
    void paint(juce::Graphics& g) override
    {
        const auto& theme=GlassTheme::current();
        const auto& p=theme.colours();
        g.fillAll(p.background.withAlpha(0.8f));
        theme.surface(g,panel.toFloat(),p.panel,false,false);
        g.setColour(p.text());g.setFont(theme.font(25.f));
        g.drawText("Create video",panel.reduced(22,12).removeFromTop(36),juce::Justification::centredLeft);
        g.setFont(theme.font(15.f));
        if(editing)
            for(auto item:std::initializer_list<std::pair<const char*,juce::Component*>>{{"Name",&name},{"Timeline length",&duration},
                    {"Selected cue",&cues},{"What changes",&category},{"Start",&frame},{"Fade to target",&length},{"Target",&value}})
                g.drawText(item.first,panel.getX()+22,item.second->getY(),126,29,juce::Justification::centredLeft);
        else
            g.drawText(juce::String(cues.getNumItems())+" cues  /  "+duration.getTextFromValue(duration.getValue())+
                       (reunion.getToggleState()?" + reunion finale":""),summary,juce::Justification::centredLeft);
    }
    void resized() override
    {
        panel=getLocalBounds().withSizeKeepingCentre(juce::jmin(800,getWidth()),juce::jmin(editing?600:218,getHeight()));
        auto r=panel.reduced(22,12);r.removeFromTop(42);
        auto lib=r.removeFromTop(35).reduced(1,2);
        exportPreset.setBounds(lib.removeFromRight(82));importPreset.setBounds(lib.removeFromRight(82));create.setBounds(lib.removeFromRight(148));library.setBounds(lib);
        if(editing) {
            auto row=[&]{return r.removeFromTop(31).withTrimmedLeft(126).reduced(1,2);};
            auto nameRow=row(); opening.setBounds(nameRow.removeFromRight(176).reduced(2,0));
            name.setBounds(nameRow);duration.setBounds(row());
            track.setBounds(r.removeFromTop(104).reduced(1,3));
            cues.setBounds(row());
            auto controls=row();category.setBounds(controls.removeFromLeft(controls.getWidth()/3).reduced(1,0));
            parameter.setBounds(controls);
            frame.setBounds(row());length.setBounds(row());value.setBounds(row());choice.setBounds(value.getBounds());
            auto words=r.removeFromTop(31).reduced(1,2);
            showCaption.setBounds(words.removeFromLeft(126));caption.setBounds(words);
            auto buttons=r.removeFromTop(32).reduced(1,2);
            add.setBounds(buttons.removeFromLeft(90));replace.setBounds(buttons.removeFromLeft(100));
            remove.setBounds(buttons.removeFromLeft(105));reunion.setBounds(buttons);
        } else summary=r.removeFromTop(40);
        auto status=r.removeFromTop(36);loop.setBounds(status.removeFromRight(130));message.setBounds(status);
        auto buttons=r.removeFromTop(35).reduced(1,2);
        close.setBounds(buttons.removeFromRight(75));preview.setBounds(buttons.removeFromRight(104));
        edit.setBounds(buttons.removeFromLeft(120));
        if(saveCopy.isVisible()) saveCopy.setBounds(buttons.removeFromLeft(114));
        if(save.isVisible()) save.setBounds(buttons.removeFromLeft(70));
        if(trash.isVisible()) trash.setBounds(buttons.removeFromLeft(120));
    }
private:
    struct CueTrack : juce::Component {
        StudioState sequence;
        int limit=900, selected=0;
        std::function<void(int)> select,seek;
        juce::Rectangle<float> rail() const {return getLocalBounds().toFloat().reduced(16,7);}
        float xAt(int frame) const {return rail().getX()+float(frame)/limit*rail().getWidth();}
        float yAt(int id) const {
            int previous=-1, stack=0;
            for(int i=1;i<=id;++i) {
                int cueFrame=sequence.getChild(i)["frame"];
                stack=cueFrame==previous ? (stack+1)%4 : 0;
                previous=cueFrame;
            }
            return rail().getY()+26+stack*13.f;
        }
        void paint(juce::Graphics& g) override {
            const auto& theme=GlassTheme::current();
            const auto& p=theme.colours();
            theme.surface(g,getLocalBounds().toFloat(),p.background,false,false,true);
            g.setFont(theme.font(12.f));
            for(int tick=0;tick<=4;++tick){
                const int f=limit*tick/4;const float x=xAt(f);
                g.setColour(p.control);g.drawVerticalLine(int(x),rail().getY()+16,rail().getBottom());
                const int px=juce::jlimit(2,std::max(2,getWidth()-52),int(x)-25);
                g.setColour(p.muted());g.drawText(juce::String(f/30.,1)+" s",px,4,50,16,juce::Justification::centred);
            }
            for(int i=1;i<sequence.getNumChildren();++i){
                const auto cue=sequence.getChild(i);
                const float x=xAt(int(cue["frame"])), y=yAt(i);
                g.setColour(i==selected ? p.accent : p.secondary);
                if(int(cue["length"])>0)
                    g.drawLine(x,y,xAt(int(cue["frame"])+int(cue["length"])),y,i==selected?3.f:1.4f);
                g.fillEllipse(x-4,y-4,8,8);
            }
        }
        void mouseDown(const juce::MouseEvent& event) override {
            float best=9.f;int hit=0;
            for(int i=1;i<sequence.getNumChildren();++i){
                const float x=xAt(int(sequence.getChild(i)["frame"]));
                const float distance=event.position.getDistanceFrom({x,yAt(i)});
                if(distance<best || (distance==best && i>selected && hit<=selected)){best=distance;hit=i;}
            }
            if(hit>0) select(hit);
            else seek(juce::jlimit(0,limit,juce::roundToInt((event.position.x-rail().getX())/rail().getWidth()*limit)));
        }
    } track;
    const std::vector<StudioSetting>& settings;
    juce::StringArray sections;
    juce::Rectangle<int> panel,summary;
    bool editing=false;
    void updateBounds()
    {
        int minimum=30;
        for(int i=1;i<sequence.getNumChildren();++i){
            auto cue=sequence.getChild(i);
            minimum=std::max(minimum,int(cue["frame"])+int(cue["length"]));
        }
        minimum=juce::jlimit(30,216000,minimum);
        duration.setRange(minimum,216000,1);
        duration.setValue(juce::jmax(minimum,int(sequence.getProperty("duration",std::max(900,minimum+75)))),juce::dontSendNotification);
        const int limit=int(duration.getValue());
        frame.setRange(0,juce::jmin(108000,limit),1);
        length.setRange(0,juce::jmin(108000,limit),1);
        track.sequence=sequence;track.limit=limit;track.repaint();
        repaint();
    }
    void pickCategory()
    {
        parameter.clear(juce::dontSendNotification);
        for(size_t i=0;i<settings.size();++i)
            if(settings[i].section==sections[category.getSelectedId()-1])
                parameter.addItem(settings[i].label,int(i)+1);
        parameter.setSelectedItemIndex(0,juce::dontSendNotification);
    }
    void configureValue()
    {
        if(parameter.getSelectedId()<=0) return;
        auto& s=settings[parameter.getSelectedId()-1]; value.setRange(s.low,s.high,s.step);value.setValue(s.read(),juce::dontSendNotification);
        choice.clear(juce::dontSendNotification);choice.addItemList(s.choices,1);
        choice.setSelectedId(int(s.read()-s.low)+1,juce::dontSendNotification);
        choice.setVisible(editing && !s.choices.isEmpty());value.setVisible(editing && s.choices.isEmpty()); length.setEnabled(s.choices.isEmpty());
    }
    void list(int id)
    {
        cues.clear(juce::dontSendNotification);
        for(int i=1;i<sequence.getNumChildren();++i){
            auto c=sequence.getChild(i);
            for(const auto& s:settings) if(s.id==c["key"].toString())
                cues.addItem(juce::String(double(c["frame"])/30.,2)+" s   /   "+s.section+" / "+s.label,i);
        }
        if(cues.getNumItems()){cues.setSelectedId(juce::jlimit(1,cues.getNumItems(),id),juce::dontSendNotification);editSelected();}
        replace.setEnabled(cues.getNumItems()>0);remove.setEnabled(cues.getNumItems()>0);
        updateBounds();
    }
    void editSelected(){
        auto c=sequence.getChild(cues.getSelectedId()); if(!c.hasType("Cue"))return;
        for(size_t i=0;i<settings.size();++i)if(settings[i].id==c["key"].toString()){
            category.setSelectedId(sections.indexOf(settings[i].section)+1,juce::dontSendNotification);
            pickCategory();parameter.setSelectedId(int(i)+1,juce::dontSendNotification);configureValue();
            track.selected=cues.getSelectedId();track.repaint();
            const auto words=c["caption"].toString();
            showCaption.setToggleState(!c.hasProperty("caption") || words.isNotEmpty(),juce::dontSendNotification);
            caption.setText(words,false);caption.setEnabled(showCaption.getToggleState());
            frame.setValue(c["frame"]);length.setValue(c["length"]);value.setValue(c["value"]);
            choice.setSelectedId(int(c["value"])-int(settings[i].low)+1,juce::dontSendNotification);break;
        }
    }
    void commit(bool update){
        const auto& s=settings[parameter.getSelectedId()-1];
        int f=int(frame.getValue()), n=choice.isVisible()?0:int(length.getValue());
        if(f+n>int(duration.getValue())){
            message.setText("Extend Timeline length or shorten the transition.",juce::dontSendNotification);return;
        }
        auto cue=videoCue(s.id,f,n,choice.isVisible()?s.low+choice.getSelectedId()-1:value.getValue());
        if(!showCaption.getToggleState()) cue.setProperty("caption","",nullptr);
        else if(caption.getText().trim().isNotEmpty()) cue.setProperty("caption",caption.getText().trim(),nullptr);
        for(int i=1;i<sequence.getNumChildren();++i){
            auto c=sequence.getChild(i);if((update && i==cues.getSelectedId()) || c["key"].toString()!=s.id)continue;
            int start=c["frame"],end=start+int(c["length"]);
            if(f==start || (f<end && f+n>start)){message.setText("Two cues for this setting overlap.",juce::dontSendNotification);return;}
        }
        if(update) sequence.removeChild(cues.getSelectedId(),nullptr);
        int pos=1;while(pos<sequence.getNumChildren() && int(sequence.getChild(pos)["frame"])<=f)++pos;
        sequence.addChild(cue,pos,nullptr);list(pos);message.setText(update?"Cue updated.":"Cue added to timeline.",juce::dontSendNotification);
    }
};
