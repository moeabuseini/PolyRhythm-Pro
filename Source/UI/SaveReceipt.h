#pragma once
#include "GlassTheme.h"
#include <JuceHeader.h>
#include <deque>

// A receipt never takes focus from a playing instrument or opens a modal loop.
class SaveReceipt final : public juce::Component, private juce::Timer
{
public:
    SaveReceipt()
    {
        setAlwaysOnTop(true);setVisible(false);
        for(auto* button:{&close,&reveal,&copy}){
            addAndMakeVisible(button);button->setMouseClickGrabsKeyboardFocus(false);
        }
        close.onClick=[this]{next();};
        copy.onClick=[this]{if(!receipts.empty())juce::SystemClipboard::copyTextToClipboard(receipts.front().file.getFullPathName());};
        reveal.onClick=[this]{if(!receipts.empty())receipts.front().file.revealToUser();};
    }
    void post(juce::String title,juce::String detail,juce::File file)
    {
        receipts.push_front({std::move(title),std::move(detail),file});
        showCurrent();
    }
    void resized() override
    {
        auto row=getLocalBounds().reduced(14,8).removeFromBottom(25);
        close.setBounds(row.removeFromRight(75));row.removeFromRight(6);reveal.setBounds(row.removeFromRight(110));row.removeFromRight(6);copy.setBounds(row.removeFromRight(85));
    }
    void paint(juce::Graphics& g) override
    {
        if(receipts.empty())return;
        const auto& current=receipts.front();
        auto area=getLocalBounds().toFloat().reduced(1);
        const auto& theme=GlassTheme::current();
        theme.surface(g,area,theme.colours().panel,false,false,true);
        auto text=getLocalBounds().reduced(15,8);
        g.setColour(theme.colours().text());g.setFont(theme.font(17.f));g.drawText(current.title,text.removeFromTop(25),juce::Justification::centredLeft);
        g.setColour(theme.colours().muted());g.setFont(theme.font(14.f));
        text.removeFromBottom(28);g.drawFittedText(current.detail,text,juce::Justification::centredLeft,4);
    }
private:
    struct Receipt {juce::String title,detail;juce::File file;};
    std::deque<Receipt> receipts;
    juce::TextButton close{"Dismiss"},reveal{"Show location"},copy{"Copy path"};
    void showCurrent(){
        setVisible(true);toFront(false);reveal.setVisible(receipts.front().file!=juce::File{});copy.setVisible(reveal.isVisible());
        setDescription(receipts.front().title+". "+receipts.front().detail);
        repaint();startTimer(9500);
    }
    void next(){stopTimer();receipts.pop_front();if(receipts.empty())setVisible(false);else showCurrent();}
    void timerCallback() override {if(!isMouseOverOrDragging(true))next();}
};
