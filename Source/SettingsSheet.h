#pragma once
#include "GlassTheme.h"
#include "StudioSettings.h"

// A sheet lives inside the main window, so it also works in full screen.
class SettingsSheet final : public juce::Component
{
    class Field final : public juce::Component
    {
    public:
        Field(const StudioSetting& spec, double value, std::function<void(double)> change)
        {
            if (!spec.choices.isEmpty())
            {
                combo.addItemList(spec.choices, 1);
                combo.setSelectedId(1 + juce::roundToInt(value-spec.low), juce::dontSendNotification);
                combo.onChange = [this, lo=spec.low, change] { change(lo + combo.getSelectedId()-1); };
                addAndMakeVisible(combo);
            }
            else
            {
                number.setSliderStyle(juce::Slider::IncDecButtons);
                number.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 83, 24);
                number.setRange(spec.low,spec.high,spec.step);
                number.setTextValueSuffix(spec.unit);
                number.setValue(value,juce::dontSendNotification);
                number.onValueChange = [this, change] { change(number.getValue()); };
                addAndMakeVisible(number);
            }
            setTitle(spec.label);
        }
        void setValue(double value, double low)
        {
            if (combo.isVisible()) combo.setSelectedId(1+juce::roundToInt(value-low),juce::dontSendNotification);
            else number.setValue(value,juce::dontSendNotification);
        }
        void resized() override { number.setBounds(getLocalBounds()); combo.setBounds(getLocalBounds()); }
    private:
        juce::ComboBox combo;
        juce::Slider number;
    };
    struct Row final : juce::Component
    {
        juce::ToggleButton selected;
        Field first, second;
        juce::TextButton choices;
        bool defaults;
        Row(const StudioSetting& spec, RandomRule& rule, bool isDefaults, std::function<void()> onSelection)
            : first(spec, rule.low, [this,&spec,&rule](double n) {
                  rule.low=n;
                  if (n>rule.high) { rule.high=n; second.setValue(n,spec.low); }
              }),
              second(spec,rule.high,[this,&spec,&rule](double n) {
                  rule.high=n;
                  if (n<rule.low) { rule.low=n; first.setValue(n,spec.low); }
              }), defaults(isDefaults)
        {
            selected.setButtonText(spec.label);
            selected.setToggleState(rule.enabled,juce::dontSendNotification);
            selected.onClick=[this,&rule,onSelection] { rule.enabled=selected.getToggleState(); sync(); onSelection(); };
            addAndMakeVisible(selected); addAndMakeVisible(first);
            addChildComponent(second); second.setVisible(!defaults);
            if(!defaults && !spec.choices.isEmpty()) {
                first.setVisible(false); second.setVisible(false); addAndMakeVisible(choices);
                choices.setButtonText("Allowed choices...");
                choices.onClick=[this,&spec,&rule] {
                    juce::PopupMenu menu;
                    for(int i=0;i<spec.choices.size();++i)
                        menu.addItem(i+1,spec.choices[i],true,(rule.allowed&(1u<<i))!=0);
                    juce::Component::SafePointer<Row> self(this);
                    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&choices),[self,&rule,&spec](int id){
                        if(!self || !id) return;
                        const unsigned mask=(1u<<spec.choices.size())-1;
                        const unsigned updated=(rule.allowed^(1u<<(id-1)))&mask;
                        if(updated) rule.allowed=updated;
                    });
                };
            }
            sync();
        }
        void sync() { choices.setEnabled(selected.getToggleState()); first.setEnabled(selected.getToggleState()); second.setEnabled(selected.getToggleState()); }
        void resized() override
        {
            auto r=getLocalBounds().reduced(2,3);
            selected.setBounds(r.removeFromLeft(r.getWidth()/3));
            choices.setBounds(r.reduced(3,0));
            if (!defaults) second.setBounds(r.removeFromRight(r.getWidth()/2).reduced(3,0));
            first.setBounds(r.reduced(3,0));
        }
    };
public:
    SettingsSheet(const std::vector<StudioSetting>& settings, std::vector<RandomRule> initial,
                  bool defaults, std::function<void(const std::vector<RandomRule>&)> apply,
                  std::function<void()> cancel)
        : catalogue(settings), rules(std::move(initial)), editingDefaults(defaults)
    {
        heading.setText(defaults ? "Startup defaults" : "Custom randomizer",juce::dontSendNotification);
        heading.setFont(juce::FontOptions(25.f));
        info.setText(defaults ? "Checked values apply at startup or via Load defaults. Saving leaves this performance unchanged."
                              : "Check settings to roll. Set numeric limits or tick the choices you want to include.",juce::dontSendNotification);
        info.setFont(juce::FontOptions(13.f));
        for (const auto& s : catalogue) sections.addIfNotAlreadyThere(s.section);
        groups.addItemList(sections,1); groups.setSelectedId(1,juce::dontSendNotification);
        groups.onChange=[this] { rebuild(); };
        all.setButtonText("Select section"); none.setButtonText("Clear section");
        all.onClick=[this] { selectSection(true); }; none.onClick=[this] { selectSection(false); };
        confirm.setButtonText(defaults ? "Save defaults" : "Randomize selected");
        confirm.onClick=[this,apply] { apply(rules); };
        close.setButtonText("Cancel"); close.onClick=std::move(cancel);
        for (juce::Component* child : std::initializer_list<juce::Component*>{&heading,&info,&groups,&all,&none,&confirm,&close,&viewport,&lowHeading,&highHeading})
            addAndMakeVisible(child);
        lowHeading.setText(defaults ? "Default value" : "Minimum",juce::dontSendNotification);
        highHeading.setText("Maximum",juce::dontSendNotification);
        highHeading.setVisible(!defaults);
        viewport.setViewedComponent(&list,false);
        viewport.setScrollBarsShown(true,false);
        setWantsKeyboardFocus(true);
        rebuild();
    }
    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::escapeKey) { close.triggerClick(); return true; }
        return false;
    }
    void paint(juce::Graphics& g) override
    {
        const auto& theme=GlassTheme::current();
        g.fillAll(theme.colours().background.withAlpha(0.74f));
        theme.surface(g,sheet.toFloat(),theme.colours().panel,false,false);
    }
    void resized() override
    {
        if (getWidth() < 60 || getHeight() < 60) return;
        sheet=getLocalBounds().reduced(18).withSizeKeepingCentre(juce::jmin(740,getWidth()-36),juce::jmin(620,getHeight()-36));
        auto area=sheet.reduced(24,16);
        heading.setBounds(area.removeFromTop(36)); info.setBounds(area.removeFromTop(35));
        auto toolbar=area.removeFromTop(36);
        groups.setBounds(toolbar.removeFromLeft(210).reduced(2)); toolbar.removeFromLeft(10);
        all.setBounds(toolbar.removeFromLeft(132).reduced(2)); none.setBounds(toolbar.removeFromLeft(132).reduced(2));
        auto columns=area.removeFromTop(28).withTrimmedRight(viewport.getScrollBarThickness());
        columns.removeFromLeft(columns.getWidth()/3);
        if(!editingDefaults) highHeading.setBounds(columns.removeFromRight(columns.getWidth()/2));
        lowHeading.setBounds(columns);
        auto footer=area.removeFromBottom(44);
        confirm.setBounds(footer.removeFromRight(172).reduced(3));
        close.setBounds(footer.removeFromRight(88).reduced(3));
        viewport.setBounds(area);
        list.setSize(area.getWidth()-viewport.getScrollBarThickness(),int(rows.size())*36);
        for (size_t i=0;i<rows.size();++i) rows[i]->setBounds(0,int(i)*36,list.getWidth(),36);
    }
private:
    void selectSection(bool selected)
    {
        for(size_t i=0;i<rules.size();++i)
            if(catalogue[i].section==groups.getText()) rules[i].enabled=selected;
        rebuild();
    }
    void rebuild()
    {
        rows.clear();
        for(size_t i=0;i<rules.size();++i)
            if(catalogue[i].section==groups.getText()) {
                auto row=std::make_unique<Row>(catalogue[i],rules[i],editingDefaults,[this]{updateApply();});
                list.addAndMakeVisible(*row);
                rows.push_back(std::move(row));
            }
        resized();
        viewport.setViewPosition(0,0);
        updateApply();
    }
    void updateApply()
    {
        const auto selected=std::count_if(rules.begin(),rules.end(),[](const auto& r){return r.enabled;});
        confirm.setEnabled(editingDefaults || selected>0);
        if(!editingDefaults) confirm.setButtonText("Randomize selected ("+juce::String(int(selected))+")");
    }
    const std::vector<StudioSetting>& catalogue;
    std::vector<RandomRule> rules;
    bool editingDefaults;
    juce::Rectangle<int> sheet;
    juce::StringArray sections;
    juce::Label heading,info,lowHeading,highHeading;
    juce::ComboBox groups;
    juce::TextButton all,none,confirm,close;
    juce::Component list;
    juce::Viewport viewport;
    std::vector<std::unique_ptr<Row>> rows;
};
