#pragma once
#include "GlassTheme.h"
#include "NodeVoicing.h"
#include "PerformanceSlider.h"
#include "TransportAudioEngine.h"

class VoicingPanel final : public juce::Component
{
public:
    explicit VoicingPanel(TransportAudioEngine& audio) : engine(audio)
    {
        order.setTitle("Pitch order");
        order.addItemList({"Ascending arpeggio", "Descending arpeggio", "Seeded arpeggio", "Ensemble (original)"}, 1);
        copies.setTitle("Nodes per pitch");
        copies.setTextBoxStyle(juce::Slider::TextBoxRight, false, 38, 20);
        addAndMakeVisible(order);
        addAndMakeVisible(copies);
        shape.setTitle("Chord voicing");
        shape.addItemList({"Diatonic seventh", "Root-position triad", "First inversion", "Suspended second", "Stacked fourths"}, 1);
        touch.setTitle("How to play the chord");
        touch.addItemList({"One tone per node", "Block chord", "Upward roll"}, 1);
        addChildComponent(shape);
        addChildComponent(touch);
        shape.onChange = [this] { engine.chordShape = shape.getSelectedId() - 1; repaint(); };
        touch.onChange = [this] { engine.chordTouch = touch.getSelectedId() - 1; repaint(); };
        order.onChange = [this] {
            engine.strips[layer].pitchMode = order.getSelectedId()-1;
            selectLayer(layer);
        };
        copies.onValueChange = [this] {
            engine.strips[layer].nodesPerPitch = static_cast<int>(copies.getValue());
            repaint();
        };
    }
    void selectLayer(int index)
    {
        layer = index;
        copies.resetKeys={"layer"+juce::String(layer)+"_copies"};
        const auto& strip = engine.strips[layer];
        order.setSelectedId(strip.pitchMode.load()+1, juce::dontSendNotification);
        // Keep the upper limit editable even on a one-node score; a saved
        // group size will apply if more nodes are enabled later.
        copies.setRange(1, studio::BeatGrid::maxBodies, 1);
        copies.setValue(strip.nodesPerPitch.load(), juce::dontSendNotification);
        copies.setEnabled(strip.pitchMode.load()!=studio::ensemble);
        const bool editableChord = index == 2 && strip.pitchMode.load() != studio::ensemble;
        shape.setVisible(editableChord);
        touch.setVisible(editableChord);
        shape.setSelectedId(engine.chordShape.load() + 1, juce::dontSendNotification);
        touch.setSelectedId(engine.chordTouch.load() + 1, juce::dontSendNotification);
        litUntil.fill(0);
        resized();
        repaint();
    }
    void strike(const studio::Hit& hit)
    {
        if(hit.layer==layer && juce::isPositiveAndBelow(hit.body, studio::BeatGrid::maxBodies))
            litUntil[hit.body]=juce::Time::getMillisecondCounterHiRes()+240;
    }
    void resized() override
    {
        auto area=getLocalBounds();
        order.setBounds(area.removeFromTop(32).withTrimmedLeft(110).reduced(2));
        copies.setBounds(area.removeFromTop(32).withTrimmedLeft(110).reduced(2));
        if (shape.isVisible()) {
            shape.setBounds(area.removeFromTop(26).withTrimmedLeft(110).reduced(2, 1));
            touch.setBounds(area.removeFromTop(26).withTrimmedLeft(110).reduced(2, 1));
        }
    }
    void paint(juce::Graphics& g) override
    {
        auto area=getLocalBounds();
        const auto& theme=GlassTheme::current();
        g.setColour(theme.colours().text()); g.setFont(theme.font(13.f));
        g.drawText("Pitch order",area.removeFromTop(32),juce::Justification::centredLeft);
        g.drawText("Nodes per pitch",area.removeFromTop(32),juce::Justification::centredLeft);
        const int chordShape = engine.chordShape.load();
        if (shape.isVisible()) {
            g.drawText("Chord voicing", area.removeFromTop(26), juce::Justification::centredLeft);
            g.drawText("Play as", area.removeFromTop(26), juce::Justification::centredLeft);
            constexpr const char* formulas[]{"1 - 3 - 5 - 7  |  A triad with its seventh.",
                "1 - 3 - 5  |  The root sits at the bottom.",
                "3 - 5 - 1  |  The third sits at the bottom.",
                "1 - 2 - 5  |  The second replaces the third.",
                "1 - 4 - 7  |  Two scale fourths stacked."};
            g.drawFittedText(formulas[chordShape], area.removeFromTop(21), juce::Justification::centredLeft, 1);
        }
        const auto& strip=engine.strips[layer];
        const int mode=strip.pitchMode.load();
        if(mode==studio::ensemble) {
            g.drawFittedText("Pitches follow the changing harmony.", area.removeFromTop(40),
                             juce::Justification::centredLeft,2);
            return;
        }
        const int count=engine.nodesPerLayer.load(), groupSize=strip.nodesPerPitch.load();
        const int shift=static_cast<int>(strip.values[studio::pitch].load()+12*strip.values[studio::octave].load());
        const int seed=engine.seed.load(), cellWidth=area.getWidth()/4;
        const int globalShift = engine.masterTranspose.load();
        const int cellHeight=juce::jmin(38,area.getHeight()/((count+3)/4));
        const double now=juce::Time::getMillisecondCounterHiRes();
        const auto colour=juce::Colour(studio::colours[layer]);
        const bool forecastPitch=(engine.roadmap || engine.bassSeed>=0 || engine.melodyAfter>=0 || engine.melodyBefore>=0) && engine.grid().probability[layer]>0;
        for (int node=0;node<count;++node) {
            juce::Rectangle<int> cell(area.getX()+(node%4)*cellWidth, area.getY()+(node/4)*cellHeight,
                                      cellWidth,cellHeight);
            const bool active=litUntil[node]>now;
            g.setColour(colour.withAlpha(active ? 0.42f : 0.10f));
            g.fillRoundedRectangle(cell.reduced(2).toFloat(),5);
            g.setColour(colour); g.setFont(GlassTheme::current().font(11.f));
            g.drawText(juce::String(node+1),cell.removeFromLeft(23),juce::Justification::centred);
            const int midi=forecastPitch
                ? engine.nextNodePitch(layer,node,engine.playhead())
                : studio::nodePitch(layer,node,count,groupSize,mode,seed,shift,chordShape)+globalShift;
            g.setColour(juce::Colours::white); g.setFont(GlassTheme::current().font(14.f));
            g.drawText(juce::MidiMessage::getMidiNoteName(midi,true,true,4),cell,juce::Justification::centred);
        }
    }
private:
    TransportAudioEngine& engine;
    int layer=0;
    juce::ComboBox order, shape, touch;
    PerformanceSlider copies;
    std::array<double,studio::BeatGrid::maxBodies> litUntil{};
};
