#pragma once
#include <JuceHeader.h>

namespace producerUI
{
// IDs also identify commands in the user's shortcut file; append rather than renumber.
enum Action {
    catchMeeting=1, catchRecent, stems, browse, resolve, lift, space, lockBass, evolve,
    melodyA, melodyB, melodyC, clearMelody, roadmapToggle, roadmapEdit, record, storeA,
    storeB, recallA, recallB, expose, saveStill, camera, portrait, unpin, shotSettings, completedPass,
    settle, restoreMix, holdExposure, openProducer, playPause, rewind, advance, nextAlignment, resetTransport,
    randomRhythm, savePerformance, openVideoEditor, fullScreen, undoEdit, redoEdit, noteTrack
};
struct Control { int id; const char* label; const char* group; };
inline constexpr Control controls[]{
    {lockBass,"Lock bass phrase","Phrase"},{evolve,"Generate A/B/C","Phrase"},
    {melodyA,"Melody A","Phrase"},{melodyB,"Melody B","Phrase"},{melodyC,"Melody C","Phrase"},
    {clearMelody,"Original melody","Phrase"},{roadmapToggle,"Follow roadmap","Phrase"},{roadmapEdit,"Edit four chords...","Phrase"},
    {resolve,"Resolve home chord","Reunion"},{space,"Lift reverb","Reunion"},{lift,"Open fan","Reunion"},{settle,"Settle back","Reunion"},
    {catchMeeting,"MIDI since reunion","Capture"},{catchRecent,"MIDI last 30 s","Capture"},{completedPass,"MIDI last pass","Capture"},
    {stems,"Render five stems","Capture"},{record,"Record automation","Capture"},{expose,"Expose to reunion","Capture"},
    {saveStill,"Save exposure PNG","Capture"},{browse,"Open exports","Capture"},
    {storeA,"Store mix A","Mix & stage"},{storeB,"Store mix B","Mix & stage"},{recallA,"Listen to A","Mix & stage"},
    {recallB,"Listen to B","Mix & stage"},{restoreMix,"Return to prior mix","Mix & stage"},
    {camera,"Reunion camera","Mix & stage"},{portrait,"Portrait guide","Mix & stage"},
    {shotSettings,"Frame shots...","Mix & stage"},{unpin,"Release strand","Mix & stage"},
    {holdExposure,"Hold to expose","Stage shortcuts"},{openProducer,"Open Producer","Stage shortcuts"},
    {playPause,"Play / pause","Transport"},{rewind,"Back five seconds","Transport"},{advance,"Forward five seconds","Transport"},
    {nextAlignment,"Next alignment","Transport"},{resetTransport,"Reset transport","Transport"},{randomRhythm,"Random rhythm","Transport"},
    {savePerformance,"Save performance","Studio"},{openVideoEditor,"Create video","Studio"},{fullScreen,"Full screen","Studio"},
    {undoEdit,"Undo","Studio"},{redoEdit,"Redo","Studio"},{noteTrack,"Show note track","Studio"}
};

class Panel final : public juce::Component
{
public:
    Panel()
    {
        addAndMakeVisible(melody);
        melody.setFont(juce::FontOptions(17.f));
        addAndMakeVisible(detail);
        detail.setFont(juce::FontOptions(14.f));
        for(int i=0;i<groups.size();++i){
            auto& tab=groupButtons[i];tab.setButtonText(groups[i]);addAndMakeVisible(tab);
            tab.onClick=[this,i]{group=i;resized();};
        }
        for(const auto& control:controls){
            if(!groups.contains(control.group))continue;
            auto* b=buttons.add(new juce::TextButton(control.label));
            b->setComponentID(juce::String(control.id));
            b->onClick=[this,id=control.id]{if(invoke)invoke(id);};
            addChildComponent(b);
        }
    }
    void resized() override
    {
        auto bounds=getLocalBounds();
        melody.setBounds(bounds.removeFromTop(30));
        auto tabs=bounds.removeFromTop(70);
        for(int i=0;i<4;++i){
            const int w=tabs.getWidth()/2;
            groupButtons[i].setBounds((i%2?tabs.withTrimmedLeft(w):tabs.withWidth(w)).withY(tabs.getY()+35*(i/2)).withHeight(34).reduced(2));
            groupButtons[i].setToggleState(group==i,juce::dontSendNotification);
        }
        detail.setBounds(bounds.removeFromTop(52).reduced(2));
        juce::Rectangle<int> row;int column=0;
        for(auto* b:buttons){
            const int id=b->getComponentID().getIntValue();
            const auto* spec=std::find_if(std::begin(controls),std::end(controls),[id](auto c){return c.id==id;});
            b->setVisible(groups[group]==spec->group);
            if(!b->isVisible())continue;
            if(column==0)row=bounds.removeFromTop(37);
            b->setBounds((column==0?row.withWidth(row.getWidth()/2):row.withTrimmedLeft(row.getWidth()/2)).reduced(2));
            column=1-column;
        }
        if(refresh)refresh();
    }
    const juce::StringArray groups{"Phrase","Reunion","Capture","Mix & stage"};
    int group=0;
    std::function<void(int)> invoke;
    std::function<void()> refresh;
    juce::Label melody,detail;
    juce::OwnedArray<juce::TextButton> buttons;
private:
    std::array<juce::TextButton,4> groupButtons;
};
}
