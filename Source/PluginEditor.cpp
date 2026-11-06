#include "PluginEditor.h"
#include <complex>
using namespace juce;
using namespace baak;
namespace { juce::Colour gold(0xffb88a29),ink(0xff272723),cream(0xfff5efdf),silver(0xffdadbd9); }
BaakEditor::Theme::Theme(){refresh();}
void BaakEditor::Theme::refresh(){
    auto fg=dark?juce::Colour(0xffeee9dc):ink;
    setColour(Slider::textBoxTextColourId,fg);setColour(Slider::textBoxOutlineColourId,Colours::transparentBlack);
    setColour(Slider::textBoxBackgroundColourId,Colours::transparentBlack);
    setColour(Label::textColourId,fg);setColour(TextButton::textColourOffId,fg);setColour(TextButton::textColourOnId,ink);
    setColour(ComboBox::backgroundColourId,dark?juce::Colour(0xff33332f):cream);setColour(ComboBox::textColourId,fg);
    setColour(ComboBox::outlineColourId,gold.withAlpha(.5f));setColour(PopupMenu::backgroundColourId,dark?juce::Colour(0xff33332f):cream);setColour(PopupMenu::textColourId,fg);
}
void BaakEditor::Theme::drawButtonBackground(Graphics& g,Button& b,const juce::Colour&,bool hover,bool down){
    auto r=b.getLocalBounds().toFloat().reduced(1);
    auto c=b.getToggleState()?gold:(dark?juce::Colour(0xff343530):juce::Colour(0xffeee7d6));
    g.setGradientFill(ColourGradient(c.brighter(hover?.25f:.1f),0,0,c.darker(down?.3f:.1f),0,r.getBottom(),false));g.fillRoundedRectangle(r,5);
    g.setColour(gold.withAlpha(b.getToggleState()?.9f:.4f));g.drawRoundedRectangle(r,5,1);
}
void BaakEditor::Theme::drawRotarySlider(Graphics& g,int x,int y,int w,int h,float value,float a,float b,Slider&){
    auto r=Rectangle<float>((float)x,(float)y,(float)w,(float)h).reduced(9);float radius=jmin(r.getWidth(),r.getHeight())*.5f;
    auto c=r.getCentre();r={c.x-radius,c.y-radius,2*radius,2*radius};
    g.setColour(Colours::black.withAlpha(.13f));g.fillEllipse(r.translated(1,3).expanded(2));
    g.setGradientFill(ColourGradient(juce::Colour(0xfffafafa),r.getX(),r.getY(),juce::Colour(0xff777975),r.getRight(),r.getBottom(),false));g.fillEllipse(r);
    auto face=r.reduced(5);g.setGradientFill(ColourGradient(juce::Colour(0xff44443f),face.getX(),face.getY(),juce::Colour(0xff181916),face.getRight(),face.getBottom(),false));g.fillEllipse(face);
    Path arc;arc.addCentredArc(c.x,c.y,radius-2,radius-2,0,a,a+value*(b-a),true);g.setColour(gold);g.strokePath(arc,PathStrokeType(4));
    float angle=a+value*(b-a)-MathConstants<float>::halfPi;
    g.setColour(cream);g.drawLine(c.x+std::cos(angle)*radius*.35f,c.y+std::sin(angle)*radius*.35f,c.x+std::cos(angle)*radius*.78f,c.y+std::sin(angle)*radius*.78f,2.5f);
}
void BaakEditor::Theme::drawLinearSlider(Graphics& g,int x,int y,int w,int h,float pos,float,float,Slider::SliderStyle style,Slider&){
    bool vertical=style==Slider::LinearVertical;float cx=x+w*.5f,cy=y+h*.5f;
    g.setColour(Colours::black.withAlpha(.65f));
    if(vertical)g.fillRoundedRectangle(cx-2,(float)y,4,(float)h,2);else g.fillRoundedRectangle((float)x,cy-2,(float)w,4,2);
    Rectangle<float> thumb=vertical?Rectangle<float>(cx-9,pos-6,18,12):Rectangle<float>(pos-6,cy-9,12,18);
    g.setGradientFill(ColourGradient(juce::Colour(0xffffe8a5),thumb.getX(),thumb.getY(),gold,thumb.getRight(),thumb.getBottom(),false));g.fillRoundedRectangle(thumb,3);g.setColour(gold.darker());g.drawRoundedRectangle(thumb,3,1);
}
BaakEditor::Control::Control(BaakProcessor& p,int i,bool horizontal){
    addAndMakeVisible(slider);addAndMakeVisible(label);auto& s=specs[i];
    label.setText(s.name,dontSendNotification);label.setJustificationType(Justification::centred);label.setFont(Font(FontOptions(13.f,Font::bold)));
    slider.setSliderStyle(horizontal?Slider::LinearHorizontal:Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(Slider::TextBoxBelow,false,100,23);slider.setTextValueSuffix(s.unit);slider.setTooltip(s.help);slider.setTitle(s.name);
    slider.setColour(Slider::textBoxTextColourId,ink);
    slider.setColour(Slider::textBoxOutlineColourId,Colours::transparentBlack);
    slider.setColour(Slider::textBoxBackgroundColourId,Colours::transparentBlack);
    slider.setDoubleClickReturnValue(true,s.initial);slider.setNumDecimalPlacesToDisplay(i==MidFreq?0:1);
    attachment=std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.state,s.id,slider);
    if(i==baak::Colour) slider.textFromValueFunction=[](double v){return StringArray{"Soft","Rounded","Edge","Asymmetric"}[jlimit(0,3,roundToInt(v))];};
    if(i==Focus) slider.textFromValueFunction=[](double v){return StringArray{"Full","Bass","Mid","Top"}[jlimit(0,3,roundToInt(v))];};
    slider.setNumDecimalPlacesToDisplay(i==MidFreq?0:1);slider.updateText();
}
void BaakEditor::Control::resized(){auto r=getLocalBounds();label.setBounds(r.removeFromTop(21));slider.setBounds(r);}
void BaakEditor::saveUndo(){undoStates.push_back(processor.current());if(undoStates.size()>80)undoStates.erase(undoStates.begin());redoStates.clear();}
void BaakEditor::restore(bool forward){auto& from=forward?redoStates:undoStates;auto& to=forward?undoStates:redoStates;if(from.empty())return;to.push_back(processor.current());auto p=from.back();from.pop_back();for(int i=0;i<Count;++i)processor.setValue(i,p[i]);}
BaakEditor::BaakEditor(BaakProcessor& p):AudioProcessorEditor(p),processor(p){
    setLookAndFeel(&theme);loudHistory.fill(-90);inputSpectrum.fill(-100);outputSpectrum.fill(-100);
    for(int i=0;i<Count;++i){if(i==Bypass||i==DC||i==Unity)continue;bool horizontal=(i==Width||i==Calm||i==Clean||i==MidFreq||i==LowComp||i==MidComp||i==HighComp||i==Body||i==BassMono||i==baak::Colour||i==Focus||i==TapeTone||i==ToneColor||i==Silk||i==DeMud);controls[i]=std::make_unique<Control>(p,i,horizontal);addAndMakeVisible(*controls[i]);controls[i]->slider.onDragStart=[this]{saveUndo();};}
    controls[Mid]->label.setText("Mid",dontSendNotification);controls[MidFreq]->label.setText("Mid frequency",dontSendNotification);controls[baak::Colour]->label.setText("Color",dontSendNotification);controls[Warmth]->label.setText("Saturation",dontSendNotification);controls[Calm]->label.setText("De-harsh",dontSendNotification);
    for(auto* b:{&bypass,&unity,&dc,&mode,&themeButton,&undo,&redo,&reset,&calibrate,&load,&audition,&master})addAndMakeVisible(*b);
    bypass.onClick=[this]{saveUndo();processor.setValue(Bypass,1-processor.current()[Bypass]);};unity.onClick=[this]{saveUndo();processor.setValue(Unity,1-processor.current()[Unity]);};dc.onClick=[this]{saveUndo();processor.setValue(DC,1-processor.current()[DC]);};
    mode.onClick=[this]{advanced=!advanced;mode.setButtonText(advanced?"Main controls":"Advanced");resized();repaint();};themeButton.onClick=[this]{theme.dark=!theme.dark;theme.refresh();for(auto& c:controls)if(c){c->slider.setColour(Slider::textBoxTextColourId,theme.dark?cream:ink);c->label.setColour(Label::textColourId,theme.dark?cream:ink);}themeButton.setButtonText(theme.dark?"Light":"Dark");sendLookAndFeelChange();repaint();};
    undo.onClick=[this]{restore(false);};redo.onClick=[this]{restore(true);};reset.onClick=[this]{processor.resetMeters.store(true);loudHistory.fill(-90);reductionHistory.fill(0);};
    calibrate.setTooltip("Set a starting input gain near -18 LUFS short-term; play audio first.");calibrate.onClick=[this]{float level=processor.inShort.load();if(level>-65){saveUndo();processor.setValue(Input,std::clamp(-18-level,-24.f,24.f));}};
    for(int i=0;i<3;++i){addAndMakeVisible(slots[i]);slots[i].setButtonText(String::charToString((juce_wchar)('A'+i)));slots[i].setTooltip("Click recalls; Shift-click stores current settings.");slots[i].onClick=[this,i]{if(ModifierKeys::getCurrentModifiers().isShiftDown())processor.snapshot(i);else{saveUndo();processor.recall(i);}};}
    addAndMakeVisible(presets);presets.addItemList({"Clean Master","Gentle polish","Warm & intimate","Punch & space"},1);presets.setSelectedId(1,dontSendNotification);presets.onChange=[this]{saveUndo();auto p=defaults();int id=presets.getSelectedId();if(id==2){p[High]=.7f;p[Glue]=15;}if(id==3){p[Tape]=20;p[Warmth]=12;p[Low]=.5f;}if(id==4){p[Width]=110;p[Glue]=10;p[Cohesion]=12;}p[Loud]=processor.current()[Loud];for(int i=0;i<Count;++i)processor.setValue(i,p[i]);};
    addAndMakeVisible(views);views.addItemList({"Spectrum","Dynamics","EQ","Stereo","Reference","Ears only"},1);views.setSelectedId(1,dontSendNotification);views.onChange=[this]{resized();repaint();};
    load.onClick=[this]{chooser=std::make_unique<FileChooser>("Load reference audio",File{},"*.wav;*.aiff;*.aif;*.flac;*.mp3");Component::SafePointer<BaakEditor> safe(this);chooser->launchAsync(FileBrowserComponent::openMode|FileBrowserComponent::canSelectFiles,[safe](const FileChooser& fc){if(!safe)return;auto f=fc.getResult();if(f.existsAsFile()&&!safe->processor.loadReference(f))AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon,"Reference audio","This audio format could not be opened.");safe->repaint();});};
    audition.onClick=[this]{processor.playReference(true);};master.onClick=[this]{processor.playReference(false);};
    addAndMakeVisible(seek);seek.setSliderStyle(Slider::LinearHorizontal);seek.setTextBoxStyle(Slider::NoTextBox,false,0,0);seek.onDragEnd=[this]{processor.reference.setPosition(seek.getValue());};
    addAndMakeVisible(refGain);refGain.setRange(-36,0,.1);refGain.setValue(-6);refGain.setTextValueSuffix(" dB ref");refGain.onValueChange=[this]{processor.referenceGain.store((float)refGain.getValue());};
    setResizable(true,true);setResizeLimits(1120,720,1960,1260);getConstrainer()->setFixedAspectRatio(14.0/9.0);setSize(1400,900);startTimerHz(30);timerCallback();
}
BaakEditor::~BaakEditor(){stopTimer();setLookAndFeel(nullptr);}
void BaakEditor::resized(){
    float sx=getWidth()/1400.f,sy=getHeight()/900.f;auto place=[&](Component& c,int x,int y,int w,int h){c.setBounds(roundToInt(x*sx),roundToInt(y*sy),roundToInt(w*sx),roundToInt(h*sy));};
    place(presets,620,16,190,33);place(undo,825,16,55,33);place(redo,885,16,55,33);for(int i=0;i<3;++i)place(slots[i],960+i*40,16,34,33);place(dc,1095,16,45,33);place(themeButton,1150,16,60,33);place(views,1220,16,165,33);
    for(auto& c:controls)if(c)c->setVisible(false);
    auto knob=[&](int i,int x,int y,int w,int h){controls[i]->setVisible(true);place(*controls[i],x,y,w,h);};
    knob(Input,15,86,105,145);knob(Output,1280,86,105,145);place(bypass,22,250,92,31);place(unity,1286,250,92,31);place(calibrate,15,290,105,28);place(reset,1272,290,112,28);
    place(mode,20,857,130,28);
    if(!advanced){
        knob(Low,25,430,135,155);knob(Mid,180,430,135,155);knob(High,335,430,135,155);
        knob(LowComp,30,596,125,70);knob(MidComp,185,596,125,70);knob(HighComp,340,596,125,70);knob(MidFreq,60,680,370,70);
        knob(Loud,565,432,250,266);knob(Character,480,508,100,135);knob(Glue,815,508,100,135);knob(Body,535,700,315,65);
        knob(Warmth,930,430,140,170);knob(Tape,1080,430,140,170);knob(Cohesion,1230,430,140,170);
        knob(baak::Colour,930,605,210,65);knob(TapeTone,1160,605,210,65);knob(Focus,930,690,210,65);knob(Silk,1160,690,210,65);
        knob(Calm,30,778,190,65);knob(DeMud,245,778,190,65);knob(ToneColor,460,778,190,65);knob(Width,700,778,390,65);knob(BassMono,1140,778,210,65);
    }else{int ids[]={Clean,MidFreq,Threshold,Ratio,Attack,Release,LimitRelease,Ceiling,LowComp,MidComp,HighComp,Silk,TapeTone,baak::Colour,Focus,BassMono};for(int n=0;n<16;++n)knob(ids[n],25+(n%8)*172,450+(n/8)*185,155,155);}
    bool ref=views.getSelectedId()==5;for(auto* c:std::initializer_list<Component*>{&load,&audition,&master,&seek,&refGain})c->setVisible(ref);
    place(load,170,130,170,35);place(master,355,130,100,35);place(audition,470,130,190,35);place(refGain,850,130,330,35);place(seek,170,245,1030,35);
}
void BaakEditor::paint(Graphics& g){
    g.addTransform(AffineTransform::scale(getWidth()/1400.f,getHeight()/900.f));auto fg=theme.dark?cream:ink;
    g.fillAll(theme.dark?juce::Colour(0xff242520):silver);
    g.setGradientFill(ColourGradient(theme.dark?juce::Colour(0xff33342d):cream,0,65,theme.dark?juce::Colour(0xff22231f):juce::Colour(0xffe9dfc8),0,415,false));g.fillRect(0,65,1400,350);
    g.setColour(juce::Colour(0xff24231f));g.fillRect(0,0,1400,65);g.setColour(juce::Colour(0xfff2dca4));g.setFont(Font(FontOptions(13.f,Font::bold)));g.drawText("BAAKPRODUCTION",22,16,165,30,Justification::centredLeft);
    g.setFont(Font(FontOptions(29.f,Font::bold)));g.drawText("BAD-I Master Pro",207,12,400,40,Justification::centredLeft);
    g.setColour(gold.withAlpha(.35f));g.drawHorizontalLine(415,0,1400);g.drawHorizontalLine(767,20,1380);g.drawHorizontalLine(850,20,1380);
    if(!advanced){g.drawVerticalLine(475,435,751);g.drawVerticalLine(915,435,751);}
    drawGraph(g,{145,88,1110,245});
    const char* labels[]={"LUFS-I","LUFS-S","SAMPLE PEAK","GAIN REDUCTION","CORRELATION"};
    float values[]={processor.outIntegrated.load(),processor.outShort.load(),processor.outPeak.load(),processor.gainReduction.load(),processor.correlation.load()};
    for(int i=0;i<5;++i){int x=160+i*218;g.setColour(fg.withAlpha(.7f));g.setFont(12.f);g.drawText(labels[i],x,348,210,20,Justification::centred);g.setColour(fg);g.setFont(Font(FontOptions(25.f,Font::bold)));g.drawText(values[i]<-80?"—":String(values[i],i==4?2:1),x,370,210,35,Justification::centred);}
    g.setColour(fg.withAlpha(.6f));g.setFont(12.f);g.drawText("2x oversampling  /  Sample-peak ceiling  /  v0.2",900,858,480,25,Justification::centredRight);
    if(processor.referencePlaying.load()){g.setColour(gold);g.drawText("REFERENCE AUDITION — METERS SHOW MASTER",170,320,1060,22,Justification::centred);}
}
void BaakEditor::drawGraph(Graphics& g,Rectangle<float> r){
    auto fg=theme.dark?cream:ink;int view=views.getSelectedId();
    if(view==5){g.setColour(fg);g.setFont(18.f);g.drawText(processor.referenceName.isEmpty()?"Load a reference to compare against your master":processor.referenceName,r.withY(185).withHeight(35),Justification::centred);g.setFont(13.f);g.drawText("Full-track loop • drag to seek • reference level is independent",r.withY(290).withHeight(25),Justification::centred);return;}
    if(view==6){g.setColour(fg);g.setFont(30.f);g.drawText("Listen to the music.",r,Justification::centred);return;}
    auto graph=r.reduced(30,20);
    g.setColour(gold.withAlpha(.16f));for(int i=0;i<=6;++i)g.drawHorizontalLine(roundToInt(graph.getY()+i*graph.getHeight()/6),graph.getX(),graph.getRight());
    if(view==1||view==3){for(float hz:{30.f,60.f,120.f,250.f,500.f,1000.f,2000.f,5000.f,10000.f,20000.f}){float x=graph.getX()+std::log(hz/20.f)/std::log(1000.f)*graph.getWidth();g.setColour(gold.withAlpha(.16f));g.drawVerticalLine(roundToInt(x),graph.getY(),graph.getBottom());g.setColour(fg.withAlpha(.6f));g.setFont(10.f);g.drawText(hz>=1000?String(hz/1000,0)+"k":String(hz,0),roundToInt(x)-20,roundToInt(graph.getBottom()+3),40,14,Justification::centred);}}
    if(view==1){
        auto draw=[&](const std::array<float,1024>& bins,juce::Colour col,bool fill){Path p;for(int i=0;i<500;++i){float hz=20*std::pow(1000.f,i/499.f);int bin=jlimit(1,1023,roundToInt(hz*2048/processor.analysisRate.load()));float value=bins[(size_t)bin];float x=graph.getX()+i*graph.getWidth()/499;float y=graph.getBottom()-jlimit(0.f,1.f,(value+90)/90)*graph.getHeight();if(i==0)p.startNewSubPath(x,y);else p.lineTo(x,y);}g.setColour(col);g.strokePath(p,PathStrokeType(1.8f));if(fill){p.lineTo(graph.getBottomRight());p.lineTo(graph.getBottomLeft());p.closeSubPath();g.setGradientFill(ColourGradient(col.withAlpha(.65f),0,graph.getY(),col.withAlpha(.08f),0,graph.getBottom(),false));g.fillPath(p);}};
        draw(inputSpectrum,theme.dark?juce::Colour(0xffd5d5c4):juce::Colour(0xff8d846f),false);draw(outputSpectrum,gold,true);
        g.setColour(fg);g.setFont(11.f);g.drawText("INPUT / GOLD: OUTPUT",r.removeFromTop(15),Justification::centredRight);
    }else if(view==2){
        for(int j=0;j<2;++j){Path p;for(int i=0;i<240;++i){int k=(historyPos+i)%240;float value=j?reductionHistory[k]/12:(loudHistory[k]+60)/60;float x=graph.getX()+i*graph.getWidth()/239;float y=j?graph.getY()+jlimit(0.f,1.f,value)*graph.getHeight():graph.getBottom()-jlimit(0.f,1.f,value)*graph.getHeight();if(i==0)p.startNewSubPath(x,y);else p.lineTo(x,y);}g.setColour(j?juce::Colour(0xffc36a49):gold);g.strokePath(p,PathStrokeType(2));}g.setColour(fg);g.setFont(12.f);g.drawText("8 seconds • gold: loudness (-60 to 0 LUFS) • copper: reduction (0 to 12 dB)",r.removeFromTop(17),Justification::centred);
    }else if(view==3){
        auto p=processor.current();Biquad low,mid,high;low.configure(2,processor.analysisRate.load(),120,p[Low]);mid.configure(1,processor.analysisRate.load(),p[MidFreq],p[Mid],.7);high.configure(3,processor.analysisRate.load(),8000,p[High]);
        Path path;for(int i=0;i<500;++i){double hz=20*std::pow(1000.,i/499.);auto z=std::exp(std::complex<double>(0,-2*pi*hz/processor.analysisRate.load()));std::complex<double> h=1;for(auto* f:{&low,&mid,&high})h*=(f->b0+f->b1*z+f->b2*z*z)/(1.+f->a1*z+f->a2*z*z);float d=(float)(20*std::log10(std::max(1.e-12,std::abs(h))));float x=graph.getX()+i*graph.getWidth()/499;float y=graph.getCentreY()-jlimit(-12.f,12.f,d)*graph.getHeight()/24;if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}g.setColour(gold);g.strokePath(path,PathStrokeType(2));g.setColour(fg);g.setFont(12.f);g.drawText("Static three-band EQ response • ±12 dB",r.removeFromTop(17),Justification::centred);
    }else if(view==4){
        auto c=graph.getCentre();float radius=graph.getHeight()*.48f;g.setColour(gold.withAlpha(.2f));g.drawEllipse(c.x-radius,c.y-radius,radius*2,radius*2,1);g.drawLine(c.x-radius,c.y,c.x+radius,c.y,1);g.drawLine(c.x,c.y-radius,c.x,c.y+radius,1);g.setColour(gold.withAlpha(.5f));for(int i=0;i<2048;i+=3){float m=outputRing[i],s=sideRing[i];g.fillEllipse(c.x+jlimit(-1.f,1.f,s)*radius-1,c.y-jlimit(-1.f,1.f,m)*radius-1,2,2);}g.setColour(fg);g.setFont(12.f);g.drawText("Stereo scope • Mid / Side",r.removeFromTop(17),Justification::centred);
    }
}
void BaakEditor::timerCallback(){
    std::vector<float> in,out,side;processor.readAnalysis(in,out,side);
    for(size_t i=0;i<in.size();++i){inputRing[ringPos]=in[i];outputRing[ringPos]=out[i];sideRing[ringPos]=side[i];ringPos=(ringPos+1)%2048;}
    if(!in.empty())for(int pass=0;pass<2;++pass){auto& ring=pass?outputRing:inputRing;auto& bins=pass?outputSpectrum:inputSpectrum;fftData.fill(0);for(int i=0;i<2048;++i)fftData[i]=ring[(ringPos+i)%2048]*(.5f-.5f*std::cos(2*MathConstants<float>::pi*i/2047));fft.performFrequencyOnlyForwardTransform(fftData.data());for(int i=0;i<1024;++i){float v=Decibels::gainToDecibels(fftData[i]/512.f,-100.f);bins[i]+=(v-bins[i])*(v>bins[i]?.65f:.14f);}}
    loudHistory[historyPos]=processor.outShort.load();reductionHistory[historyPos]=processor.gainReduction.load();historyPos=(historyPos+1)%240;
    auto p=processor.current();bypass.setToggleState(p[Bypass]>.5f,dontSendNotification);unity.setToggleState(p[Unity]>.5f,dontSendNotification);dc.setToggleState(p[DC]>.5f,dontSendNotification);
    undo.setEnabled(!undoStates.empty());redo.setEnabled(!redoStates.empty());audition.setEnabled(!processor.referenceName.isEmpty());audition.setToggleState(processor.referencePlaying.load(),dontSendNotification);master.setToggleState(!processor.referencePlaying.load(),dontSendNotification);
    double length=processor.reference.getLengthInSeconds();if(length>0){seek.setRange(0,length,.01);if(!seek.isMouseButtonDown())seek.setValue(processor.reference.getCurrentPosition(),dontSendNotification);}repaint();
}
