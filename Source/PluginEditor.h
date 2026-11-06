#pragma once
#include "PluginProcessor.h"
class BaakEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit BaakEditor(BaakProcessor&);
    ~BaakEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    struct Theme : juce::LookAndFeel_V4 {
        bool dark=false;
        Theme(); void refresh();
        void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
        void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider::SliderStyle,juce::Slider&) override;
        void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
    } theme;
    struct Control : juce::Component {
        juce::Slider slider; juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        Control(BaakProcessor&,int,bool);
        void resized() override;
    };
    BaakProcessor& processor;
    juce::TooltipWindow tips{this,550};
    std::array<std::unique_ptr<Control>,baak::Count> controls;
    juce::TextButton bypass{"Bypass"},unity{"Unity"},dc{"DC"},mode{"Advanced"},themeButton{"Dark"},undo{"Undo"},redo{"Redo"},reset{"Reset meters"},calibrate{"Auto input"};
    juce::TextButton load{"Load reference"},audition{"Audition reference"},master{"Master"};
    std::array<juce::TextButton,3> slots;
    juce::ComboBox presets,views;
    juce::Slider seek,refGain;
    std::unique_ptr<juce::FileChooser> chooser;
    std::vector<baak::Params> undoStates,redoStates;
    bool advanced=false;
    juce::dsp::FFT fft{11};
    std::array<float,2048> inputRing{},outputRing{},sideRing{};
    std::array<float,4096> fftData{};
    std::array<float,1024> inputSpectrum{},outputSpectrum{};
    std::array<float,240> loudHistory{},reductionHistory{};
    int ringPos=0,historyPos=0;
    void saveUndo(); void restore(bool);
    void timerCallback() override;
    void drawGraph(juce::Graphics&,juce::Rectangle<float>);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaakEditor)
};
