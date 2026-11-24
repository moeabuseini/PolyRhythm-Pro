#pragma once
#include "TransportAudioEngine.h"
#include "RhythmScene.h"
#include "SceneControls.h"
#include "VoicingPanel.h"
#include "GlassTheme.h"
#include "ProducerPanel.h"

class InspectorTab final : public juce::Button
{
  public:
    InspectorTab() : Button("") {}
    int index = 0;
    void paintButton(juce::Graphics &g, bool hover, bool down) override
    {
        const float x = getWidth() * 0.5f, y = 13;
        const auto& theme=GlassTheme::current();
        const auto& p=theme.colours();
        theme.surface(g,getLocalBounds().toFloat().reduced(2,2),
                      getToggleState() ? p.selected() : p.control,hover,down,false,hasKeyboardFocus(true));
        g.setColour(getToggleState() ? p.text() : p.muted());
        if (index == 0)
        {
            g.fillEllipse(x - 6, y + 2, 6, 4);
            g.drawLine(x, y + 4, x, y - 6, 1.7f);
            g.drawLine(x, y - 6, x + 6, y - 4, 2);
        }
        else if (index == 1)
        {
            for (int i = -1; i <= 1; ++i)
            {
                float xx = x + i * 5;
                g.drawLine(xx, y - 6, xx, y + 6);
                g.fillEllipse(xx - 2, y + i * 3 - 2, 4, 4);
            }
        }
        else if (index == 2)
        {
            for (int i = 0; i < 3; ++i)
                g.drawEllipse(x - 7 + i * 4, y - 5 + i, 10.f - i * 2, 10.f - i * 2, 1.1f);
        }
        else if (index == 3)
        {
            for (int i = 0; i < 12; ++i)
                g.fillEllipse(x - 7 + (i * 7) % 15, y - 5 + (i * 3) % 11, 2, 2);
        }
        else
        {
            g.drawEllipse(x - 6, y - 6, 12, 12, 1);
            g.drawLine(x - 9, y + 4, x + 8, y - 3, 1.5f);
        }
        g.setFont(theme.font(12.f));
        g.drawText(getName(), 0, 21, getWidth(), 18, juce::Justification::centred);
    }
};
class LayerInspector final : public juce::Component
{
  public:
    LayerInspector(TransportAudioEngine &audio, RhythmScene &canvas) : engine(audio), scene(canvas)
    {
        using namespace studio;
        addChildComponent(sceneControls);
        addChildComponent(voicing);
        addChildComponent(producerPanel);
        for (int i = 0; i < 4; ++i)
        {
            auto &mode = modes[i];
            mode.index = i == 0 ? 0 : i==1 ? 4 : i==2 ? 3 : 1;
            mode.setName(i == 0 ? "Music" : i==1 ? "Environment" : i==2 ? "Nodes" : "Producer");
            mode.setRadioGroupId(2);
            mode.onClick = [this, i] { selectPage(i == 0 ? 0 : i==1 ? 4 : i==2 ? 7 : 9); };
            addAndMakeVisible(mode);
        }
        for (int i = 0; i < layerCount; ++i)
            layerBox.addItem(layers[i], i + 1);
        for (int i = 0; i < static_cast<int>(sounds.size()); ++i)
            soundBox.addItem(sounds[i], i + 1);
        for (auto *box : {&layerBox, &soundBox})
            addAndMakeVisible(*box);
        layerBox.setTitle("Layer");
        soundBox.setTitle("Instrument");
        layerBox.setSelectedId(1, juce::dontSendNotification);
        layerBox.onChange = [this] { refresh(); };
        soundBox.onChange = [this] { strip().sound = soundBox.getSelectedId() - 1; };
        ambience.setTextWhenNothingSelected("Reverb / chorus preset");
        ambience.addItemList({"Close studio", "Velvet chamber", "Glacial hall", "Ensemble haze"}, 1);
        ambience.onChange = [this]
        {
            constexpr float patches[][7] = {{22, 10, 75, 40, 4, 12, 0},
                                            {54, 24, 62, 76, 22, 32, 7},
                                            {93, 46, 26, 100, 12, 28, 12},
                                            {72, 30, 52, 94, 57, 72, 24}};
            constexpr Parameter params[] = {room,   wet,         damping,       stereoWidth,
                                            chorus, chorusDepth, chorusFeedback};
            int index = ambience.getSelectedId() - 1;
            if (index >= 0)
                for (int i = 0; i < 7; ++i)
                    strip().values[params[i]] = patches[index][i];
            ambience.setSelectedId(0, juce::dontSendNotification);
            refresh();
        };
        addAndMakeVisible(ambience);
        const char *names[]{"Score", "EQ", "Space", "Cloud", "Sky", "Acoustics","Motion","Paths","Pitches"};
        for (int i = 0; i < static_cast<int>(tabs.size()); ++i)
        {
            auto &tab = tabs[i];
            tab.index = i;
            tab.setName(names[i]);
            tab.setTitle(names[i]);
            tab.setRadioGroupId(1);
            tab.onClick = [this, i] { selectPage(i); };
            addAndMakeVisible(tab);
        }
        for (int i = 0; i < 6; ++i)
        {
            auto &knob = knobs[i];
            knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 82, 19);
            knob.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            knob.onValueChange = [this, i] { changed(i); };
            labels[i].setJustificationType(juce::Justification::centred);
            addAndMakeVisible(knob);
            addAndMakeVisible(labels[i]);
        }
        twinkleLabel.setText("Star twinkle", juce::dontSendNotification);
        twinkle.setTitle("Star twinkle");
        twinkle.setSliderStyle(juce::Slider::LinearHorizontal);
        twinkle.setTextBoxStyle(juce::Slider::TextBoxRight, false, 54, 20);
        twinkle.setRange(0, 100, 1);
        twinkle.setTextValueSuffix("%");
        twinkle.onValueChange = [this]
        {
            scene.settings()[RhythmScene::twinkleSetting] = static_cast<float>(twinkle.getValue());
            scene.repaint();
        };
        addChildComponent(twinkleLabel);
        addChildComponent(twinkle);
        refresh();
    }
    void selectPage(int value)
    {
        page = value;
        refresh();
        resized();
    }
    producerUI::Panel producerPanel;
    void inspectPitch(int l){layerBox.setSelectedId(l+1,juce::dontSendNotification);selectPage(8);}
    void syncTempo(double bpm){sceneControls.syncTempo(bpm);}
    void linkTempo(juce::Slider& master){sceneControls.linkTempo(master);}
    void noteHit(const studio::Hit& hit) { if(page==8) voicing.strike(hit); }
    void refreshPitchMap() { if(page==8) voicing.repaint(); }
    void syncControls() { selectPage(page); }
    void resized() override
    {
        auto b = getLocalBounds().reduced(12, 8);
        for(int row=0;row<2;++row){
            auto top=b.removeFromTop(41);
            modes[row*2].setBounds(top.removeFromLeft(top.getWidth()/2));
            modes[row*2+1].setBounds(top);
        }
        if(page==9){producerPanel.setBounds(b.reduced(0,5));return;}
        auto selectors = b.removeFromTop(section(page)==0 ? 30 : 0);
        layerBox.setBounds(
            selectors
                .removeFromLeft((page < 2 || page == 5) ? selectors.getWidth() / 2 : selectors.getWidth())
                .reduced(2, 0));
        soundBox.setBounds(selectors.reduced(2, 0));
        ambience.setBounds(selectors.reduced(2, 0));
        b.removeFromTop(10);
        auto bar = b.removeFromTop(page==7 ? 0 : 42);
        int width = bar.getWidth() / (section(page)==0 ? 6 : 2);
        for (auto &tab : tabs)
            if (tab.isVisible())
                tab.setBounds(bar.removeFromLeft(width));
        b.removeFromTop(8);
        if(page==8) { voicing.setBounds(b); return; }
        if(page>=6) { sceneControls.setBounds(b); return; }
        if (page == 4)
        {
            auto row = b.removeFromTop(28);
            twinkleLabel.setBounds(row.removeFromLeft(90));
            twinkle.setBounds(row);
        }
        int height = juce::jmin(142, b.getHeight() / 2);
        for (int row = 0; row < 2; ++row)
        {
            auto line = b.removeFromTop(height);
            int cellWidth = line.getWidth() / 3;
            for (int col = 0; col < 3; ++col)
            {
                int i = row * 3 + col;
                auto cell = line.removeFromLeft(cellWidth).reduced(2);
                labels[i].setBounds(cell.removeFromTop(20));
                knobs[i].setBounds(cell);
            }
        }
    }

  private:
    static int section(int tab) { return tab==9 ? 3 : tab==7 ? 2 : (tab==4 || tab==6) ? 1 : 0; }
    studio::Strip &strip() { return engine.strips[layerBox.getSelectedId() - 1]; }
    enum
    {
        hue = studio::parameterCount,
        starCount,
        sparkle,
        drift,
        detailA,
        detailB
    };
    static constexpr int pages[6][6]{
        {studio::gain, studio::pitch, studio::octave, studio::rate, studio::chance, studio::decay},
        {studio::cutoff, studio::lowEQ, studio::highEQ, -1, -1, -1},
        {studio::wet, studio::room, studio::delay, studio::feedback, studio::chorus, studio::modulation},
        {studio::cloud, studio::grainDensity, studio::grainRange, studio::grainLength, studio::shimmer,
         studio::reverse},
        {hue, starCount, sparkle, drift, detailA, detailB},
        {studio::damping, studio::stereoWidth, studio::chorusDepth, studio::chorusFeedback,
         studio::chorusCentre, -1}};
    float &skyValue(int id) { return scene.settings()[id - hue]; }
    void changed(int i)
    {
        if(page>=6) return;
        int id = pages[page][i];
        if (syncing || id < 0)
            return;
        float value = static_cast<float>(knobs[i].getValue());
        if (id < hue)
            strip().values[id] = value;
        else
        {
            skyValue(id) = value;
            scene.repaint();
        }
    }
    void refresh()
    {
        syncing = true;
        modes[section(page)].setToggleState(true, juce::dontSendNotification);
        producerPanel.setVisible(page==9);
        voicing.setVisible(page==8);
        if(page==8) voicing.selectLayer(layerBox.getSelectedId()-1);
        sceneControls.setVisible(page==6 || page==7);
        if(sceneControls.isVisible()) sceneControls.show(page==7);
        for (int i = 0; i < static_cast<int>(tabs.size()); ++i)
            tabs[i].setVisible(section(i)==section(page) && page!=7);
        layerBox.setVisible(section(page)==0);
        soundBox.setVisible(page < 2);
        ambience.setVisible(page == 5);
        twinkleLabel.setVisible(page == 4);
        twinkle.setVisible(page == 4);
        const auto skyPrefix="sky"+juce::String(scene.activeEnvironment())+"_look";
        twinkle.resetKeys={skyPrefix+"6"};
        twinkle.setValue(scene.settings()[RhythmScene::twinkleSetting], juce::dontSendNotification);
        const studio::Dial sky[]{{"Sky hue", 0, 359, 1, 225, " deg"},
                                 {"Stars", 0, RhythmScene::maxStars, 1, 720, ""},
                                 {"Note sparks", 0, 100, 1, 80, "%"},
                                 {"Sky drift", 0, 100, 1, 55, "%"},
                                 {scene.details[scene.activeEnvironment()][0], 0, 100, 1, 50, "%"},
                                 {scene.details[scene.activeEnvironment()][1], 0, 100, 1, 50, "%"}};
        soundBox.setSelectedId(strip().sound.load() + 1, juce::dontSendNotification);
        if(page<int(tabs.size())) tabs[page].setToggleState(true, juce::dontSendNotification);
        for (int i = 0; i < 6; ++i)
        {
            int id = page>=6 ? -1 : pages[page][i];
            auto &knob = knobs[i];
            knob.setVisible(id >= 0);
            labels[i].setVisible(id >= 0);
            if (id < 0)
                continue;
            const auto &d = id < hue ? studio::dials[id] : sky[id - hue];
            labels[i].setText(d.name, juce::dontSendNotification);
            knob.setTitle(d.name);
            knob.resetKeys={id<hue ? "layer"+juce::String(layerBox.getSelectedId()-1)+"_dial"+juce::String(id)
                                   : skyPrefix+juce::String(id-hue)};
            knob.setSliderStyle(page == 4 ? juce::Slider::LinearHorizontal
                                          : juce::Slider::RotaryHorizontalVerticalDrag);
            knob.setRange(d.min, d.max, d.step);
            knob.setSkewFactor(id == studio::cutoff ? 0.35 : 1.0);
            knob.setTextValueSuffix(d.unit);
            knob.setValue(id < hue ? strip().values[id].load() : skyValue(id), juce::dontSendNotification);
            knob.setColour(juce::Slider::rotarySliderFillColourId,
                           juce::Colour(studio::colours[layerBox.getSelectedId() - 1]));
        }
        syncing = false;
    }
    TransportAudioEngine &engine;
    RhythmScene &scene;
    juce::ComboBox layerBox, soundBox, ambience;
    SceneControls sceneControls{engine,scene};
    VoicingPanel voicing{engine};
    std::array<InspectorTab, 4> modes;
    std::array<InspectorTab, 9> tabs;
    std::array<juce::Label, 6> labels;
    std::array<PerformanceSlider, 6> knobs;
    PerformanceSlider twinkle;
    juce::Label twinkleLabel;
    int page = 0;
    bool syncing = false;
};
