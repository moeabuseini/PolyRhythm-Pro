#pragma once
#include <JuceHeader.h>

class GlassTheme final : public juce::LookAndFeel_V4
{
public:
    GlassTheme()
    {
        setDefaultSansSerifTypefaceName("Bahnschrift");
        setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff1c293a));
        setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour(0xff365568));
        setColour(juce::TextButton::buttonColourId,juce::Colour(0xff324659));
        setColour(juce::Slider::thumbColourId,juce::Colour(0xffb8eae4));
    }
    static void surface(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour tint, bool hot, bool down)
    {
        if(b.isEmpty()) return;
        const float r=juce::jmin(18.f,b.getHeight()*0.46f);
        g.setColour(juce::Colour(0x45070b15));
        g.fillRoundedRectangle(b.translated(0,down ? 1.f : 2.f),r);
        auto top=tint.interpolatedWith(juce::Colours::white,hot ? 0.30f : 0.18f).withAlpha(0.76f);
        g.setGradientFill({top,b.getTopLeft(),tint.darker(0.3f).withAlpha(0.64f),b.getBottomRight(),false});
        g.fillRoundedRectangle(b,r);
        auto reflection=b.reduced(2.f).withHeight(b.getHeight()*0.45f);
        g.setGradientFill({juce::Colour(0x407df4ee),reflection.getTopLeft(),juce::Colour(0x0194a8ff),reflection.getBottomRight(),false});
        g.fillRoundedRectangle(reflection,r*0.8f);
        g.setGradientFill({juce::Colours::white.withAlpha(hot ? 0.62f : 0.31f),b.getTopLeft(),
                           juce::Colour(0x225badc1),b.getBottomRight(),false});
        g.drawRoundedRectangle(b,r,1.f);
    }
    void drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour& c,bool over,bool down) override
    {
        auto box=b.getLocalBounds().toFloat().reduced(1.5f,2.5f);
        surface(g,box.translated(0,down ? 1.f : over ? -1.f : 0.f),c,over || b.hasKeyboardFocus(true),down);
    }
    void drawComboBox(juce::Graphics& g,int w,int h,bool down,int,int,int,int,juce::ComboBox& box) override
    {
        surface(g,box.getLocalBounds().toFloat().reduced(1.5f,2),juce::Colour(0xff344656),
                box.isMouseOver(true) || box.hasKeyboardFocus(true),down);
        juce::Path arrow;
        arrow.startNewSubPath(w-20.f,h*0.46f); arrow.lineTo(w-16.f,h*0.60f); arrow.lineTo(w-12.f,h*0.46f);
        g.setColour(juce::Colour(0xffc0d9dc)); g.strokePath(arrow,juce::PathStrokeType(1.5f));
    }
    void drawToggleButton(juce::Graphics& g,juce::ToggleButton& b,bool over,bool down) override
    {
        auto box=b.getLocalBounds().toFloat().reduced(1.5f,2.5f);
        const bool on=b.getToggleState();
        surface(g,box,on ? juce::Colour(0xff305354) : juce::Colour(0xff242d3c),over || b.hasKeyboardFocus(true),down);
        auto dot=juce::Rectangle<float>(8,box.getCentreY()-3,6,6);
        g.setColour(on ? juce::Colour(0xff9affda) : juce::Colour(0xff87929d));
        if(on) g.fillEllipse(dot); else g.drawEllipse(dot,1);
        g.setColour(juce::Colour(0xffe1ebef));
        g.setFont(juce::FontOptions(13.f));
        g.drawFittedText(b.getButtonText(),box.toNearestInt().withTrimmedLeft(18).withTrimmedRight(4),
                         juce::Justification::centredLeft,1);
    }
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float value,float start,float end,juce::Slider& s) override
    {
        auto centre=juce::Point<float>(x+w*0.5f,y+h*0.5f);
        const float radius=juce::jmin(33.f,juce::jmin(w,h)*0.43f);
        if(radius<3) return;
        auto face=juce::Rectangle<float>(radius*1.62f,radius*1.62f).withCentre(centre);
        const auto accent=s.findColour(juce::Slider::rotarySliderFillColourId);
        g.setGradientFill({juce::Colour(0xff506070),face.getTopLeft(),juce::Colour(0xff192636),face.getBottomRight(),false});
        g.fillEllipse(face);
        g.setColour(juce::Colour(0x668dafb9)); g.drawEllipse(face,0.9f);
        const auto arc=[&](float last,juce::Colour colour) {
            juce::Path path; path.addCentredArc(centre.x,centre.y,radius,radius,0,start,last,true);
            g.setColour(colour); g.strokePath(path,juce::PathStrokeType(2.5f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        };
        const float angle=start+(end-start)*value;
        arc(end,juce::Colour(0xff384554)); arc(angle,accent);
        auto tip=centre.getPointOnCircumference(radius*0.59f,angle);
        g.setColour(accent.brighter(0.3f)); g.fillEllipse(tip.x-2,tip.y-2,4,4);
    }
};

class DockGrip final : public juce::Component
{
public:
    explicit DockGrip(bool isVertical) : vertical(isVertical)
    {
        setMouseCursor(vertical ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::UpDownResizeCursor);
        setTitle(vertical ? "Resize inspector" : "Resize playing controls");
    }
    std::function<void(int)> moveBy;
    void mouseDown(const juce::MouseEvent& e) override { last=e.getScreenPosition(); }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        const auto pos=e.getScreenPosition();
        if(moveBy) moveBy(vertical ? pos.x-last.x : pos.y-last.y);
        last=pos;
    }
    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override { repaint(); }
    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colours::white.withAlpha(isMouseOverOrDragging() ? 0.68f : 0.24f));
        g.fillRoundedRectangle(getLocalBounds().toFloat().withSizeKeepingCentre(vertical?3.f:44.f,vertical?44.f:3.f),1.5f);
    }
private:
    bool vertical; juce::Point<int> last;
};
