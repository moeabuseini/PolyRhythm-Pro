#pragma once
#include <JuceHeader.h>

struct Appearance
{
    int palette = 0;
    juce::String font = "Bahnschrift";
    int corners = 2, borders = 1, dropdown = 0;
};

class GlassTheme final : public juce::LookAndFeel_V4
{
public:
    struct Palette
    {
        juce::Colour background, panel, control, menu, accent, secondary;
        juce::Colour text() const { return juce::Colour(0xffedf3f7); }
        juce::Colour muted() const { return panel.interpolatedWith(text(),0.68f); }
        juce::Colour border() const { return accent.interpolatedWith(text(),0.38f); }
        juce::Colour selected() const { return control.interpolatedWith(accent,0.34f); }
        juce::Colour accentInk() const { return juce::Colour(0xff10131a); }
    };

    inline static const juce::StringArray paletteNames{"Studio Glass","Copper","Violet","Slate"};
    inline static const juce::StringArray cornerNames{"Square","Soft","Round"};
    inline static const juce::StringArray borderNames{"Off","Subtle","Strong"};
    inline static const juce::StringArray dropdownNames{"Glass","Flat","Outlined"};

    GlassTheme() { apply({}); }

    static GlassTheme& current()
    {
        if(auto* theme=dynamic_cast<GlassTheme*>(&getDefaultLookAndFeel()))
            return *theme;
        static GlassTheme fallback;
        return fallback;
    }

    static const juce::StringArray& fonts()
    {
        static const auto choices=[] {
            const auto installed=juce::Font::findAllTypefaceNames();
            juce::StringArray result{"System default"};
            for(const auto* family:{"Bahnschrift","Segoe UI","Arial","Georgia","Consolas"})
                if(installed.contains(family,true)) result.add(family);
            return result;
        }();
        return choices;
    }

    const Palette& colours() const { return palette; }
    const Appearance& settings() const { return appearance; }

    void apply(Appearance updated)
    {
        if(!fonts().contains(updated.font)) updated.font="System default";
        appearance=std::move(updated);
        using C=juce::Colour;
        static const Palette palettes[]{
            {C(0xff0c1117),C(0xff18212b),C(0xff324659),C(0xff1c293a),C(0xff59e0d0),C(0xffffa66d)},
            {C(0xff1d1210),C(0xff33211d),C(0xff563931),C(0xff33241f),C(0xffffac76),C(0xfff5d283)},
            {C(0xff151121),C(0xff282039),C(0xff44355e),C(0xff302342),C(0xffd2a9ff),C(0xff9ed5fc)},
            {C(0xff111519),C(0xff222a32),C(0xff35424e),C(0xff252f39),C(0xffacd1f4),C(0xffcddd9e)}
        };
        palette=palettes[appearance.palette];
        const auto& p=palette;
        const auto family=appearance.font=="System default" ? juce::String{} : appearance.font;
        setDefaultSansSerifTypefaceName(family);
        setColourScheme(ColourScheme(p.background,p.panel,p.menu,p.border(),p.text(),p.accent,
                                     p.text(),p.selected(),p.text()));
        setColour(juce::TextButton::buttonColourId,p.control);
        setColour(juce::TextButton::buttonOnColourId,p.selected());
        setColour(juce::TextButton::textColourOffId,p.text());
        setColour(juce::TextButton::textColourOnId,p.text());
        setColour(juce::ComboBox::backgroundColourId,p.control);
        setColour(juce::ComboBox::arrowColourId,p.text());
        setColour(juce::ComboBox::focusedOutlineColourId,p.accent);
        setColour(juce::PopupMenu::highlightedBackgroundColourId,p.selected());
        setColour(juce::Slider::thumbColourId,p.accent);
        setColour(juce::Slider::trackColourId,p.control);
        setColour(juce::Slider::backgroundColourId,p.panel);
        setColour(juce::Slider::rotarySliderFillColourId,p.accent);
    }

    juce::Font font(float height, int style=juce::Font::plain) const
    {
        return withThemeFont(juce::Font(juce::FontOptions(height,style)));
    }

    juce::Font getLabelFont(juce::Label& label) override { return withThemeFont(label.getFont()); }

    float radius(juce::Rectangle<float> box) const
    {
        return juce::jmin(box.getWidth()*0.5f,box.getHeight()*0.5f,
                         appearance.corners==0 ? 0.f : appearance.corners==1 ? 6.f : 18.f);
    }

    void outline(juce::Graphics& g, juce::Rectangle<float> box, bool focused=false) const
    {
        const float width=appearance.borders==2 ? 2.f : 1.f;
        if(appearance.borders==0 && !focused) return;
        g.setColour(palette.border().withAlpha(focused || appearance.borders==2 ? 0.9f : 0.35f));
        g.drawRoundedRectangle(box.reduced(width*0.5f),radius(box.reduced(width*0.5f)),width);
    }

    void surface(juce::Graphics& g, juce::Rectangle<float> box, juce::Colour tint,
                 bool hot, bool down, bool flat=false, bool focused=false) const
    {
        if(box.isEmpty()) return;
        const float r=radius(box);
        if(flat)
        {
            g.setColour(hot ? tint.brighter(0.18f) : tint);
        }
        else
        {
            g.setColour(juce::Colour(0x45070b15));
            g.fillRoundedRectangle(box.translated(0,down ? 1.f : 2.f),r);
            const auto top=tint.interpolatedWith(juce::Colours::white,hot ? 0.30f : 0.15f);
            g.setGradientFill({top,box.getTopLeft(),tint,box.getBottomRight(),false});
        }
        g.fillRoundedRectangle(box,r);
        outline(g,box,focused);
    }

    void drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour& c,bool over,bool down) override
    {
        const auto box=b.getLocalBounds().toFloat().reduced(1.5f,2.5f);
        const bool focused=b.hasKeyboardFocus(true);
        const auto tint=b.isEnabled() ? c : c.interpolatedWith(palette.panel,0.55f);
        surface(g,box.translated(0,down ? 1.f : over ? -1.f : 0.f),tint,over || focused,down,false,focused);
    }

    void drawComboBox(juce::Graphics& g,int w,int h,bool down,int,int,int,int,juce::ComboBox& box) override
    {
        const auto bounds=box.getLocalBounds().toFloat().reduced(1.5f,2.f);
        const bool focused=box.hasKeyboardFocus(true);
        const bool hot=box.isMouseOver(true) || focused || down;
        const auto tint=appearance.dropdown==2 ? palette.panel : box.findColour(juce::ComboBox::backgroundColourId);
        surface(g,bounds,tint,hot && box.isEnabled(),down,appearance.dropdown!=0,focused);
        if(appearance.dropdown==2)
        {
            g.setColour(palette.border().withAlpha(0.55f));
            g.drawVerticalLine(w-27,5.f,static_cast<float>(h)-5.f);
        }
        juce::Path arrow;
        arrow.startNewSubPath(w-20.f,h*0.46f); arrow.lineTo(w-16.f,h*0.60f); arrow.lineTo(w-12.f,h*0.46f);
        g.setColour(box.findColour(juce::ComboBox::arrowColourId).withMultipliedAlpha(box.isEnabled() ? 1.f : 0.45f));
        g.strokePath(arrow,juce::PathStrokeType(1.5f));
    }

    void drawPopupMenuBackground(juce::Graphics& g,int width,int height) override
    {
        g.fillAll(palette.menu);
        if(appearance.borders!=0)
        {
            g.setColour(palette.border().withAlpha(appearance.borders==2 ? 0.9f : 0.35f));
            g.drawRect(0,0,width,height,appearance.borders);
        }
    }

    void drawToggleButton(juce::Graphics& g,juce::ToggleButton& b,bool over,bool down) override
    {
        const auto box=b.getLocalBounds().toFloat().reduced(1.5f,2.5f);
        const bool on=b.getToggleState(), focused=b.hasKeyboardFocus(true);
        const auto tint=on ? palette.selected() : palette.control;
        surface(g,box,b.isEnabled() ? tint : palette.panel,over || focused,down,false,focused);
        const auto dot=juce::Rectangle<float>(8,box.getCentreY()-3,6,6);
        g.setColour((on ? palette.accent : palette.muted()).withMultipliedAlpha(b.isEnabled() ? 1.f : 0.45f));
        if(on) g.fillEllipse(dot); else g.drawEllipse(dot,1);
        g.setColour(palette.text().withMultipliedAlpha(b.isEnabled() ? 1.f : 0.45f));
        g.setFont(font(13.f));
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
        g.setGradientFill({palette.control.brighter(0.25f),face.getTopLeft(),palette.panel,face.getBottomRight(),false});
        g.fillEllipse(face);
        if(appearance.borders!=0) { g.setColour(palette.border().withAlpha(0.4f)); g.drawEllipse(face,float(appearance.borders)); }
        const auto arc=[&](float last,juce::Colour colour) {
            juce::Path path; path.addCentredArc(centre.x,centre.y,radius,radius,0,start,last,true);
            g.setColour(colour); g.strokePath(path,juce::PathStrokeType(2.5f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        };
        const float angle=start+(end-start)*value;
        arc(end,palette.control); arc(angle,accent);
        auto tip=centre.getPointOnCircumference(radius*0.59f,angle);
        g.setColour(accent.brighter(0.3f)); g.fillEllipse(tip.x-2,tip.y-2,4,4);
    }
private:
    juce::Font withThemeFont(juce::Font original) const
    {
        if(appearance.font!="System default" && original.getTypefaceName()==juce::Font::getDefaultSansSerifFontName())
            original.setTypefaceName(appearance.font);
        return original;
    }

    Appearance appearance;
    Palette palette;
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
