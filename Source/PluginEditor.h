#pragma once
#include "PluginProcessor.h"
class BaakEditor : public juce::AudioProcessorEditor, private juce::Timer {
  public:
    explicit BaakEditor(BaakProcessor &);
    ~BaakEditor() override;
    void paint(juce::Graphics &) override;
    void resized() override;

  private:
    struct Theme : juce::LookAndFeel_V4 {
        Theme();
        void drawRotarySlider(juce::Graphics &, int, int, int, int, float, float, float,
                              juce::Slider &) override;
    };
    struct Knob : juce::Component {
        juce::Slider slider;
        juce::Label title;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        Knob(BaakProcessor &, int);
        void resized() override;
    };
    Theme theme;
    BaakProcessor &processor;
    juce::TooltipWindow tips{this, 650};
    juce::Viewport viewport;
    juce::Component controls;
    std::vector<std::unique_ptr<Knob>> knobs;
    juce::TextButton mode{"Pro controls"}, bypass{"Bypass"}, apply{"Apply suggestion"}, undo{"Undo change"},
        freeze{"Pause advice"}, reset{"Reset meters"};
    std::array<juce::TextButton, 3> slots;
    juce::ComboBox presets;
    juce::Label advice, explanation;
    std::array<float, 220> history{};
    int historyIndex = 0, suggested = -1;
    float suggestionValue = 0;
    bool pro = false, paused = false;
    baak::Params previous = baak::defaults();
    bool hasUndo = false;
    void timerCallback() override;
    void updateAdvice();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaakEditor)
};
