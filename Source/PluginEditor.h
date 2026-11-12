#pragma once
#include "PluginProcessor.h"
class BaakEditor : public juce::AudioProcessorEditor, private juce::Timer, public juce::FileDragAndDropTarget {
public:
    explicit BaakEditor(BaakProcessor&);
    ~BaakEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray&,int,int) override;
    void setViewForTest(int);
private:
    struct Theme : juce::LookAndFeel_V4 {
        Theme();
        void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override;
        juce::Label* createSliderTextBox(juce::Slider&) override;
        void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
        void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider::SliderStyle,juce::Slider&) override;
        void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
    } theme;
    struct ResetSlider : juce::Slider {
        double initial=0;
        void mouseDoubleClick(const juce::MouseEvent&) override;
        std::unique_ptr<juce::AlertWindow> entry;
        void mouseDown(const juce::MouseEvent& e) override {if(e.mods.isAltDown()){if(onDragStart)onDragStart();setValue(initial);if(onDragEnd)onDragEnd();}else Slider::mouseDown(e);}
    };
    struct Control : juce::Component {
        ResetSlider slider; juce::TextButton label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        Control(BaakProcessor&,int,int);
        float saved=0;int style=0;
        void resized() override;
    };
    struct Preset {juce::String name,category;baak::Params values;juce::File file;};
    struct PresetPanel : juce::Component {
        explicit PresetPanel(BaakEditor&);
        void paint(juce::Graphics&) override;void resized() override;void mouseDown(const juce::MouseEvent&) override;
        BaakEditor& owner;juce::TextEditor search;juce::ComboBox category;juce::TextButton save{"Save preset"},folder{"Open folder"},close{"Close"},pagePrev{"Previous"},pageNext{"Next"};
        int page=0;
        std::vector<int> filtered;void update();
    };
    BaakProcessor& processor;
    juce::TooltipWindow tips{this,550};
    std::array<std::unique_ptr<Control>,baak::Count> controls;
    juce::TextButton bypass{"Bypass"},unity{"Unity"},dc{"DC"},ll{"LL"},tp{"TP"},settings{"Settings"},undo{"Undo"},redo{"Redo"},reset{"Reset meters"},calibrate{"Auto input"};
    juce::TextButton presetButton{"Choose preset"},previous{"<"},next{">"};
    juce::TextButton load{"Open audio"},audition{"Reference"},master{"Master"},loop{"Loop"},loopA{"Set A"},loopB{"Set B"},beatSnap{"Beat snap"};
    juce::TextButton normalize{"Normalize"},difference{"Differences"},phase{"Show phase"},bipolar{"Bipolar"},freeze{"Freeze"};
    std::array<juce::TextButton,3> slots;
    juce::ComboBox views,spectrumMode,quality,monitor;
    juce::Slider seek,refGain,historySeconds,releaseTime;
    std::unique_ptr<juce::FileChooser> chooser;
    std::unique_ptr<PresetPanel> browser;
    std::vector<Preset> factory;
    std::vector<baak::Params> undoStates,redoStates;
    bool advanced=false;int activeSlot=0;
    juce::dsp::FFT fft{11};
    std::array<float,2048> inputRing{},outputRing{},sideRing{};
    std::array<float,4096> fftData{};
    std::array<float,1024> inputSpectrum{},outputSpectrum{};
    std::array<float,600> waveHistory{},reductionHistory{};
    int ringPos=0,historyPos=0;double lastFrame=0;
    void saveUndo();void restore(bool);void timerCallback() override;
    void drawSpectrum(juce::Graphics&,juce::Rectangle<float>,bool);
    void drawScope(juce::Graphics&,juce::Rectangle<float>);
    void drawWave(juce::Graphics&,juce::Rectangle<float>);
    void drawReferenceComparison(juce::Graphics&);
    void drawMetrics(juce::Graphics&);void drawGraph(juce::Graphics&);
    void buildPresets();void applyPreset(int);void savePreset();void openReference(const juce::File&);
    juce::File presetDirectory() const;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaakEditor)
};
