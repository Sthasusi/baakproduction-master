#include "PluginEditor.h"
#include <complex>
using namespace juce;
using namespace baak;
namespace {
const juce::Colour pink(0xffd46ca4), pale(0xffedc8db), textColour(0xfff0ebef), dim(0xffbc93a9), plum(0xff301323), button(0xff412033);
void text(Graphics& g,const String& s,Rectangle<float> r,float size,juce::Colour colour=pale,int justification=Justification::centred){g.setColour(colour);g.setFont(Font(FontOptions("Segoe UI",size,Font::plain)));g.drawText(s,r,justification);}
String reading(float n,int digits=1){return n<-80?String("--"):String(n,digits);}
void trace(Graphics& g,Path& p,juce::Colour c,float width=1.5f){g.setColour(c.withAlpha(.08f));g.strokePath(p,PathStrokeType(width+6));g.setColour(c);g.strokePath(p,PathStrokeType(width));}
}
BaakEditor::Theme::Theme(){
    setColour(TextButton::buttonColourId,button);setColour(TextButton::buttonOnColourId,pink.darker(.35f));
    setColour(TextButton::textColourOffId,pale);setColour(TextButton::textColourOnId,Colours::white);
    setColour(ComboBox::backgroundColourId,button);setColour(ComboBox::textColourId,pale);setColour(ComboBox::outlineColourId,pink.withAlpha(.2f));setColour(ComboBox::arrowColourId,pink);
    setColour(PopupMenu::backgroundColourId,plum);setColour(PopupMenu::textColourId,pale);setColour(PopupMenu::highlightedBackgroundColourId,pink.darker(.4f));
    setColour(TooltipWindow::backgroundColourId,plum);setColour(TooltipWindow::textColourId,pale);setColour(TooltipWindow::outlineColourId,pink);
    setColour(TextEditor::backgroundColourId,plum);setColour(TextEditor::textColourId,pale);setColour(TextEditor::outlineColourId,pink.withAlpha(.5f));setColour(TextEditor::focusedOutlineColourId,pink);
    setColour(Label::textColourId,pale);setColour(Slider::textBoxTextColourId,pale);setColour(Slider::textBoxOutlineColourId,Colours::transparentBlack);setColour(Slider::textBoxBackgroundColourId,Colours::transparentBlack);
}
void BaakEditor::Theme::drawButtonBackground(Graphics& g,Button& b,const juce::Colour&,bool over,bool down){
    if(b.getProperties().getWithDefault("label",false)){if(over){g.setColour(pink.withAlpha(.1f));g.fillRoundedRectangle(b.getLocalBounds().toFloat(),4);}return;}
    auto r=b.getLocalBounds().toFloat().reduced(.8f);auto c=b.getToggleState()?pink.darker(.3f):button;
    if(over)c=c.brighter(.12f);if(down)c=c.brighter(.2f);if(!b.isEnabled())c=c.withAlpha(.3f);
    g.setGradientFill(ColourGradient(c,r.getX(),r.getY(),c.darker(.15f),r.getX(),r.getBottom(),false));g.fillRoundedRectangle(r,5);
    if(b.getToggleState()){g.setColour(pink.withAlpha(.6f));g.drawRoundedRectangle(r,5,1);}
}
void BaakEditor::Theme::drawRotarySlider(Graphics& g,int x,int y,int w,int h,float value,float a,float b,Slider&){
    auto r=Rectangle<float>((float)x,(float)y,(float)w,(float)h).reduced(7);float radius=jmin(r.getWidth(),r.getHeight())*.5f;auto c=r.getCentre();r={c.x-radius,c.y-radius,radius*2,radius*2};
    g.setColour(Colours::black.withAlpha(.4f));g.fillEllipse(r.translated(0,3).expanded(2));
    g.setGradientFill(ColourGradient(juce::Colour(0xff353036),r.getX(),r.getY(),juce::Colour(0xff231f23),r.getRight(),r.getBottom(),false));g.fillEllipse(r);
    Path ring;ring.addCentredArc(c.x,c.y,radius-2,radius-2,0,a,b,true);g.setColour(pale.withAlpha(.85f));g.strokePath(ring,PathStrokeType(radius>90?5.f:3.f,PathStrokeType::curved,PathStrokeType::rounded));
    Path active;active.addCentredArc(c.x,c.y,radius-2,radius-2,0,a,a+value*(b-a),true);g.setColour(pink);g.strokePath(active,PathStrokeType(radius>90?5.f:3.f,PathStrokeType::curved,PathStrokeType::rounded));
    float angle=a+value*(b-a)-MathConstants<float>::halfPi;
    g.setColour(textColour);g.drawLine(c.x+std::cos(angle)*radius*.32f,c.y+std::sin(angle)*radius*.32f,c.x+std::cos(angle)*radius*.78f,c.y+std::sin(angle)*radius*.78f,radius>90?4.f:2.f);
}
void BaakEditor::Theme::drawLinearSlider(Graphics& g,int x,int y,int w,int h,float pos,float,float,Slider::SliderStyle style,Slider& s){
    bool vertical=style==Slider::LinearVertical;
    Rectangle<float> r=vertical?Rectangle<float>(x+w*.5f-7,(float)y,14,(float)h):Rectangle<float>((float)x,y+h*.5f-12,(float)w,24);
    g.setColour(juce::Colour(0xff3b3038));g.fillRoundedRectangle(r,vertical?7.f:6.f);g.setColour(pink.withAlpha(.65f));g.drawRoundedRectangle(r,vertical?7.f:6.f,1);
    auto fill=r.reduced(2);if(vertical)fill=fill.withTop(jlimit(fill.getY(),fill.getBottom(),pos));else fill=fill.withWidth(jlimit(0.f,fill.getWidth(),pos-fill.getX()));
    g.setGradientFill(ColourGradient(vertical?pale:pink,fill.getX(),fill.getY(),pink.darker(.3f),fill.getRight(),fill.getBottom(),false));g.fillRoundedRectangle(fill,4);
    g.setColour(pale);if(vertical)g.drawHorizontalLine(roundToInt(pos),r.getX()+2,r.getRight()-2);else g.drawVerticalLine(roundToInt(pos),r.getY()+3,r.getBottom()-3);
    if(s.getMinimum()<0&&s.getMaximum()>0){g.setColour(pale.withAlpha(.3f));g.drawVerticalLine(roundToInt(r.getCentreX()),r.getY(),r.getBottom());}
}
BaakEditor::Control::Control(BaakProcessor& p,int i,int type):style(type){
    auto& spec=specs[i];addAndMakeVisible(slider);addAndMakeVisible(label);
    label.setButtonText(spec.name);label.getProperties().set("label",true);label.setTooltip("Click to bypass this section; click again to restore it.");
    slider.setSliderStyle(type==1?Slider::LinearHorizontal:type==2?Slider::LinearVertical:Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(Slider::TextBoxBelow,false,type==2?35:88,type==2?14:20);slider.setTextValueSuffix(spec.unit);slider.setTooltip(String(spec.help)+" Alt-click resets. Shift-drag adjusts finely. Click the number to type a value.");slider.setTitle(spec.name);slider.initial=spec.initial;
    slider.setNumDecimalPlacesToDisplay(i==MidFreq||spec.max>=100?0:1);slider.setMouseDragSensitivity(250);
    attachment=std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.state,spec.id,slider);
    label.onClick=[this,&p,i]{float value=p.current()[i];if(std::abs(value-specs[i].initial)>.001f){saved=value;p.setValue(i,specs[i].initial);}else p.setValue(i,saved);};
    if(i==baak::Colour)slider.textFromValueFunction=[](double v){return StringArray{"Soft","Round","Edge","Asym"}[jlimit(0,3,roundToInt(v))];};
    if(i==Focus)slider.textFromValueFunction=[](double v){return StringArray{"Full","Low","Mid","High"}[jlimit(0,3,roundToInt(v))];};slider.setNumDecimalPlacesToDisplay(i==MidFreq||spec.max>=100?0:1);slider.updateText();
}
void BaakEditor::Control::resized(){auto r=getLocalBounds();bool compact=style==1&&getHeight()<65;label.setBounds(r.removeFromBottom(compact?16:style==2?18:27));if(compact){slider.setTextBoxStyle(Slider::NoTextBox,false,0,0);slider.setPopupDisplayEnabled(true,true,this);}else slider.setTextBoxStyle(Slider::TextBoxBelow,false,style==2?35:88,style==2?14:20);slider.setBounds(r);}
void BaakEditor::saveUndo(){undoStates.push_back(processor.current());if(undoStates.size()>100)undoStates.erase(undoStates.begin());redoStates.clear();}
void BaakEditor::restore(bool forward){auto& from=forward?redoStates:undoStates;auto& to=forward?undoStates:redoStates;if(from.empty())return;to.push_back(processor.current());auto p=from.back();from.pop_back();for(int i=0;i<Count;++i)processor.setValue(i,p[i]);}

BaakEditor::BaakEditor(BaakProcessor& p):AudioProcessorEditor(p),processor(p){
    setLookAndFeel(&theme);inputSpectrum.fill(-100);outputSpectrum.fill(-100);
    for(int i=0;i<Count;++i){if(i==Bypass||i==DC||i==Unity||i>=TruePeak)continue;
        int style=(i==LowComp||i==MidComp||i==HighComp)?2:(i==Width||i==Calm||i==DeMud||i==MidFreq||i==Body||i==BassMono||i==Focus||i==TapeTone||i==ToneColor||i==Silk||i==Character||i==Glue)?1:0;
        controls[i]=std::make_unique<Control>(p,i,style);addAndMakeVisible(*controls[i]);controls[i]->slider.onDragStart=[this]{saveUndo();};
        auto action=controls[i]->label.onClick;controls[i]->label.onClick=[this,action]{saveUndo();action();};
    }
    for(auto item:std::initializer_list<std::pair<int,const char*>>{{Loud,"Loud"},{Input,"In"},{Output,"Out"},{Low,"Low"},{Mid,"Mid"},{High,"High"},{Warmth,"Thick"},{Cohesion,"Glow"},{Character,"Presence"},{Glue,"Smooth"},{Body,"Body"},{Calm,"Calm"},{DeMud,"Clean"},{ToneColor,"Aura"},{Width,"Wide"},{MidFreq,"Mid frequency"},{TapeTone,"Warm / tight"},{Silk,"Silk"},{Focus,"Saturation region"},{baak::Colour,"Color"},{LowComp,"C"},{MidComp,"C"},{HighComp,"C"}})controls[item.first]->label.setButtonText(item.second);
    for(auto* b:{&bypass,&unity,&dc,&ll,&tp,&settings,&undo,&redo,&reset,&calibrate,&presetButton,&previous,&next,&load,&audition,&master,&loop,&loopA,&loopB,&beatSnap,&normalize,&difference,&phase,&bipolar,&freeze})addAndMakeVisible(*b);
    auto toggle=[this](TextButton& b,int id){b.setTooltip(specs[id].help);b.onClick=[this,id]{saveUndo();processor.setValue(id,1-processor.current()[id]);};};toggle(bypass,Bypass);toggle(unity,Unity);toggle(dc,DC);toggle(tp,TruePeak);toggle(ll,LowLatency);
    settings.onClick=[this]{advanced=!advanced;settings.setToggleState(advanced,dontSendNotification);if(browser)browser->setVisible(false);resized();repaint();};
    undo.onClick=[this]{restore(false);};redo.onClick=[this]{restore(true);};
    reset.onClick=[this]{processor.resetMeters.store(true);waveHistory.fill(0);reductionHistory.fill(0);inputSpectrum.fill(-100);outputSpectrum.fill(-100);};
    calibrate.setTooltip("Set a starting input gain near -18 LUFS using current input short-term loudness.");calibrate.onClick=[this]{float level=processor.inShort.load();if(level>-65){saveUndo();processor.setValue(Input,std::clamp(-18-level,-24.f,24.f));}};
    for(int i=0;i<3;++i){addAndMakeVisible(slots[i]);slots[i].setButtonText(String::charToString((juce_wchar)('A'+i)));slots[i].setTooltip("Click switches snapshot and stores the slot you leave. Shift-click stores here without switching.");slots[i].onClick=[this,i]{if(ModifierKeys::getCurrentModifiers().isShiftDown())processor.snapshot(i);else if(i!=activeSlot){saveUndo();processor.snapshot(activeSlot);processor.recall(i);activeSlot=i;}};}
    addAndMakeVisible(views);views.addItemList({"Metering","Spectrum","Gain","EQ","Reference","Ears Only"},1);views.setSelectedId((int)processor.state.state.getProperty("view",1),dontSendNotification);views.onChange=[this]{processor.state.state.setProperty("view",views.getSelectedId(),nullptr);resized();repaint();};previous.onClick=[this]{views.setSelectedId((views.getSelectedId()+4)%6+1);};next.onClick=[this]{views.setSelectedId(views.getSelectedId()%6+1);};
    addAndMakeVisible(spectrumMode);spectrumMode.addItemList({"Full spectrum","1/3 octave","Bark bands"},1);spectrumMode.setSelectedId(2);
    for(auto* b:{&normalize,&difference,&phase,&bipolar,&freeze})b->setClickingTogglesState(true);bipolar.setToggleState(true,dontSendNotification);
    addAndMakeVisible(historySeconds);historySeconds.setRange(1,20,1);historySeconds.setValue(4);historySeconds.setTextValueSuffix(" s history");historySeconds.setTextBoxStyle(Slider::TextBoxRight,false,100,24);
    addAndMakeVisible(releaseTime);releaseTime.setRange(60,1000,20);releaseTime.setValue(240);releaseTime.setTextValueSuffix(" ms decay");releaseTime.setTextBoxStyle(Slider::TextBoxRight,false,100,24);
    addAndMakeVisible(quality);quality.addItemList({"1x / Eco","2x / Standard","4x / High","8x / Render"},1);quality.setSelectedId(roundToInt(p.current()[Quality])+1,dontSendNotification);quality.onChange=[this]{saveUndo();processor.setValue(Quality,(float)(quality.getSelectedId()-1));};quality.setTooltip(specs[Quality].help);
    addAndMakeVisible(monitor);monitor.addItemList({"Normal","Dim -12 dB","Mono","Band","Phone","Small speaker"},1);monitor.setSelectedId(roundToInt(p.current()[Monitor])+1,dontSendNotification);monitor.onChange=[this]{processor.setValue(Monitor,(float)(monitor.getSelectedId()-1));};
    load.onClick=[this]{chooser=std::make_unique<FileChooser>("Open reference audio",File{},"*.wav;*.aiff;*.aif;*.flac;*.mp3");Component::SafePointer<BaakEditor> safe(this);chooser->launchAsync(FileBrowserComponent::openMode|FileBrowserComponent::canSelectFiles,[safe](const FileChooser& fc){if(safe&&fc.getResult().existsAsFile())safe->openReference(fc.getResult());});};
    audition.onClick=[this]{processor.playReference(true);};master.onClick=[this]{processor.playReference(false);};loop.onClick=[this]{processor.loopEnabled.store(!processor.loopEnabled.load());};
    beatSnap.onClick=[this]{processor.snapToBeat.store(!processor.snapToBeat.load());};
    loopA.onClick=[this]{double t=processor.reference.getCurrentPosition();float bpm=processor.referenceTempo.load();if(processor.snapToBeat.load()&&bpm>0)t=std::round(t*bpm/60)*60/bpm;if(t<processor.loopEnd.load()-.05)processor.loopStart.store(t);};loopB.onClick=[this]{double t=processor.reference.getCurrentPosition();float bpm=processor.referenceTempo.load();if(processor.snapToBeat.load()&&bpm>0)t=std::round(t*bpm/60)*60/bpm;if(t>processor.loopStart.load()+.05)processor.loopEnd.store(t);};
    addAndMakeVisible(seek);seek.setSliderStyle(Slider::LinearHorizontal);seek.setTextBoxStyle(Slider::NoTextBox,false,0,0);seek.onDragEnd=[this]{processor.reference.setPosition(seek.getValue());};
    addAndMakeVisible(refGain);refGain.setRange(-36,6,.1);refGain.setValue(processor.referenceGain.load());refGain.setTextValueSuffix(" dB ref");refGain.setTextBoxStyle(Slider::TextBoxRight,false,90,24);refGain.onValueChange=[this]{processor.referenceGain.store((float)refGain.getValue());};
    buildPresets();browser=std::make_unique<PresetPanel>(*this);addChildComponent(*browser);presetButton.onClick=[this]{browser->setVisible(!browser->isVisible());browser->toFront(false);};
    setResizable(true,true);setResizeLimits(1050,630,2100,1260);getConstrainer()->setFixedAspectRatio(5./3.);setSize(1500,900);startTimerHz(30);timerCallback();
}
BaakEditor::~BaakEditor(){stopTimer();setLookAndFeel(nullptr);}
void BaakEditor::setViewForTest(int n){views.setSelectedId(n,dontSendNotification);resized();repaint();}
void BaakEditor::resized(){
    float sx=getWidth()/1500.f,sy=getHeight()/900.f;auto place=[&](Component& c,int x,int y,int w,int h){c.setBounds(roundToInt(x*sx),roundToInt(y*sy),roundToInt(w*sx),roundToInt(h*sy));};
    for(auto& c:controls)if(c)c->setVisible(false);auto control=[&](int i,int x,int y,int w,int h){controls[i]->setVisible(true);place(*controls[i],x,y,w,h);};
    control(Input,35,20,105,120);control(Output,1360,20,105,120);place(bypass,42,145,90,25);place(unity,1367,145,90,25);
    place(presetButton,172,42,190,42);place(undo,367,42,47,42);place(redo,419,42,47,42);for(int i=0;i<3;++i)place(slots[i],480+i*40,42,35,42);
    place(ll,970,42,40,42);place(dc,1016,42,40,42);place(tp,1062,42,40,42);place(previous,1108,42,28,42);place(views,1138,42,155,42);place(next,1295,42,28,42);place(settings,1327,42,35,42);settings.setButtonText("...");
    if(!advanced){
        control(Low,55,563,128,156);control(Mid,224,563,128,156);control(High,393,563,128,156);
        control(LowComp,184,573,35,143);control(MidComp,353,573,35,143);control(HighComp,522,573,35,143);
        control(MidFreq,247,518,108,47);control(Silk,413,518,100,47);
        control(Loud,603,383,294,329);control(Character,594,708,102,57);control(Glue,806,708,102,57);
        control(Warmth,977,563,128,156);control(baak::Colour,1108,621,70,97);control(Tape,1180,563,128,156);control(Cohesion,1322,563,128,156);
        control(Focus,985,518,112,47);control(TapeTone,1189,518,110,47);
        control(Calm,65,792,125,70);control(DeMud,230,792,125,70);control(ToneColor,395,792,125,70);control(Body,690,792,125,70);control(Width,984,792,460,70);
    }else{
        int ids[]={Clean,MidFreq,Threshold,Ratio,Attack,Release,LimitRelease,Ceiling,BassMono};for(int n=0;n<9;++n)control(ids[n],30+n*162,565,145,170);
    }
    place(calibrate,35,865,100,24);place(reset,1360,865,108,24);quality.setVisible(advanced);place(quality,520,793,210,32);
    bool ref=views.getSelectedId()==5;for(auto* c:std::initializer_list<Component*>{&load,&audition,&master,&seek,&refGain,&loop,&loopA,&loopB,&beatSnap})c->setVisible(ref);
    place(load,1248,191,90,36);place(master,615,328,100,32);place(audition,725,328,110,32);place(refGain,172,281,350,30);place(seek,185,242,1145,32);place(loop,1020,281,80,30);place(loopA,1110,281,80,30);place(loopB,1200,281,80,30);place(beatSnap,870,281,135,30);
    place(monitor,580,117,220,27);monitor.setVisible(ref||advanced);
    int v=views.getSelectedId();spectrumMode.setVisible(v==2||v==1);normalize.setVisible(v==2);difference.setVisible(v==2);releaseTime.setVisible(v==2);phase.setVisible(v==4);historySeconds.setVisible(v==3);bipolar.setVisible(v==3);freeze.setVisible(v==3);
    place(spectrumMode,v==1?45:330,v==1?187:124,v==1?260:170,27);place(normalize,512,124,105,27);place(difference,628,124,110,27);place(releaseTime,754,124,280,27);place(phase,570,124,130,27);place(bipolar,485,124,100,27);place(freeze,595,124,100,27);place(historySeconds,713,124,300,27);
    if(browser)place(*browser,172,94,1165,366);
}
void BaakEditor::paint(Graphics& g){
    g.addTransform(AffineTransform::scale(getWidth()/1500.f,getHeight()/900.f));
    g.setGradientFill(ColourGradient(juce::Colour(0xff160b13),750,0,juce::Colour(0xff4b223b),750,500,false));g.fillRect(0,0,1500,500);
    g.setGradientFill(ColourGradient(juce::Colour(0xff303032),0,500,juce::Colour(0xff232325),0,900,false));g.fillRect(0,500,1500,400);
    g.setColour(pale.withAlpha(.2f));g.drawHorizontalLine(500,0,1500);
    text(g,"BAD-I Master Pro",{604,32,350,55},35,pink);text(g,"BAAKPRODUCTION",{649,86,260,18},10,dim);
    drawGraph(g);
    if(advanced){text(g,"ENGINE & DYNAMICS",{50,520,1400,30},17);text(g,"Oversampling",{375,797,135,25},14);text(g,"Changes to quality or LL apply on the next audio-engine start.",{745,796,700,28},14,dim);}
    auto p=processor.current();bool pending=processor.activeQuality.load()!=(p[LowLatency]>.5f?0:roundToInt(p[Quality]))||processor.activeLowLatency.load()!=(p[LowLatency]>.5f);
    String status=String(1<<processor.activeQuality.load())+"x  |  "+String(processor.getLatencySamples())+" samples  |  "+(p[TruePeak]>.5f?"TP guard":"Sample ceiling")+"  |  v0.3";
    if(pending)status+="  |  Restart audio to apply quality";
    if(p[Monitor]>.5f)status+="  |  MONITOR FILTER ACTIVE";
    text(g,status,{210,869,1080,20},11,dim);
    if(processor.referencePlaying.load())text(g,"REFERENCE AUDITION - MASTER METERS",{870,95,475,20},11,pink);
}

void BaakEditor::drawSpectrum(Graphics& g,Rectangle<float> r,bool compact){
    auto graph=r.reduced(30,22);int mode=spectrumMode.getSelectedId();
    for(int k=0;k<6;++k){float y=graph.getY()+k*graph.getHeight()/5;g.setColour(pale.withAlpha(.12f));g.drawHorizontalLine(roundToInt(y),graph.getX(),graph.getRight());text(g,String(-k*18),{r.getX(),y-8,27,16},10,dim);}
    for(float f:{20.f,50.f,100.f,200.f,500.f,1000.f,2000.f,5000.f,10000.f,20000.f}){float x=graph.getX()+std::log(f/20.f)/std::log(1000.f)*graph.getWidth();if(!compact){g.setColour(pale.withAlpha(.1f));g.drawVerticalLine(roundToInt(x),graph.getY(),graph.getBottom());}text(g,f>=1000?String(f/1000,0)+"k":String(f,0),{x-19,graph.getBottom()+4,38,15},9,dim);}
    float inMax=*std::max_element(inputSpectrum.begin(),inputSpectrum.end()),outMax=*std::max_element(outputSpectrum.begin(),outputSpectrum.end());
    auto value=[&](const std::array<float,1024>& bins,float hz,float band){
        int first=jlimit(1,1023,(int)(hz/std::pow(2.f,band/2)*2048/processor.analysisRate.load()));int last=jlimit(first,1023,(int)(hz*std::pow(2.f,band/2)*2048/processor.analysisRate.load()));
        double sum=0;for(int i=first;i<=last;++i)sum+=std::pow(10.,bins[i]/10.);return (float)(10*std::log10(std::max(1.e-10,sum/(last-first+1))));
    };
    int count=mode==1?420:mode==2?30:24;
    for(int pass=0;pass<2;++pass){Path p;for(int i=0;i<count;++i){
        float t=(float)i/(count-1),hz=mode==3?600*std::sinh(t*std::asinh(20000.f/600)):20*std::pow(1000.f,t);hz=jmax(20.f,hz);
        float in=value(inputSpectrum,hz,mode==1?.03f:mode==2?1.f/3:.4f),out=value(outputSpectrum,hz,mode==1?.03f:mode==2?1.f/3:.4f);
        if(normalize.getToggleState()&&!compact){in-=inMax+12;out-=outMax+12;}
        float level=pass?out:in;if(difference.getToggleState()&&!compact)level=pass?(out-in)*2-45:-45;
        float x=graph.getX()+std::log(hz/20.f)/std::log(1000.f)*graph.getWidth(),y=graph.getBottom()-jlimit(0.f,1.f,(level+90)/90)*graph.getHeight();
        if(mode>1){g.setColour(pass?pink.withAlpha(.8f):pale.withAlpha(.55f));float width=graph.getWidth()/count*.72f;if(pass)g.fillRect(x-width*.5f,y,width,graph.getBottom()-y);else g.fillRect(x-width*.5f,y,width,2.f);}
        if(i==0)p.startNewSubPath(x,y);else p.lineTo(x,y);
    }if(mode==1){trace(g,p,pass?pale:pink.withAlpha(.5f),pass?2.f:1.2f);if(pass){p.lineTo(graph.getBottomRight());p.lineTo(graph.getBottomLeft());p.closeSubPath();g.setGradientFill(ColourGradient(pink.withAlpha(.75f),0,graph.getY(),pink.withAlpha(.15f),0,graph.getBottom(),false));g.fillPath(p);}}}
}
void BaakEditor::drawScope(Graphics& g,Rectangle<float> r){
    auto c=r.getCentre();float rad=jmin(r.getWidth(),r.getHeight())*.43f;Path diamond;diamond.startNewSubPath(c.x,c.y-rad);diamond.lineTo(c.x+rad,c.y);diamond.lineTo(c.x,c.y+rad);diamond.lineTo(c.x-rad,c.y);diamond.closeSubPath();g.setColour(pink.withAlpha(.35f));g.strokePath(diamond,PathStrokeType(1));g.drawLine(c.x,c.y-rad,c.x,c.y+rad,1);
    g.setColour(pink.withAlpha(.55f));for(int i=0;i<2048;i+=4)g.fillEllipse(c.x+jlimit(-1.f,1.f,sideRing[i])*rad-1,c.y-jlimit(-1.f,1.f,outputRing[i])*rad-1,2,2);
    text(g,"L",{r.getX(),r.getBottom()-10,30,15},10,dim);text(g,"R",{r.getRight()-30,r.getBottom()-10,30,15},10,dim);
    text(g,"Correlation "+String(processor.correlation.load(),2),{r.getX(),r.getBottom()+10,r.getWidth(),18},11,dim);
}
void BaakEditor::drawWave(Graphics& g,Rectangle<float> r){
    g.setColour(pink.withAlpha(.25f));g.drawRect(r,1);bool bi=bipolar.getToggleState();float centre=bi?r.getCentreY():r.getBottom();
    int count=jlimit(30,600,roundToInt(historySeconds.getValue()*30));
    Path wave;wave.startNewSubPath(r.getX(),centre);for(int i=0;i<count;++i){int k=(historyPos-count+i+600)%600;float a=jlimit(0.f,1.f,waveHistory[k]);wave.lineTo(r.getX()+i*r.getWidth()/(count-1),centre-a*r.getHeight()*(bi?.48f:.95f));}
    for(int i=count-1;i>=0;--i){int k=(historyPos-count+i+600)%600;wave.lineTo(r.getX()+i*r.getWidth()/(count-1),centre+(bi?jlimit(0.f,1.f,waveHistory[k])*r.getHeight()*.48f:0));}wave.closeSubPath();g.setColour(pink.withAlpha(.65f));g.fillPath(wave);
    Path reduction;for(int i=0;i<count;++i){int k=(historyPos-count+i+600)%600;float x=r.getX()+i*r.getWidth()/(count-1),y=r.getY()+jlimit(0.f,1.f,reductionHistory[k]/18)*r.getHeight()*.45f;if(i==0)reduction.startNewSubPath(x,y);else reduction.lineTo(x,y);}trace(g,reduction,pale,1.2f);
}
void BaakEditor::drawMetrics(Graphics& g){
    float gr=processor.gainReduction.load();text(g,"REDUCTION",{65,425,120,20},12,dim);text(g,String(gr,1)+" dB",{65,448,120,30},21);
    text(g,gr>8?"HEAVY":gr>3?"MODERATE":"LIGHT",{208,425,180,20},12,dim);auto r=Rectangle<float>(208,454,180,12);g.setColour(pink.withAlpha(.2f));g.fillRoundedRectangle(r,3);g.setColour(gr>8?juce::Colour(0xffffae91):pink);g.fillRoundedRectangle(r.withWidth(jlimit(0.f,1.f,gr/12)*r.getWidth()),3);
    const char* labels[]={"LUFS-I","LUFS-S","TRUE PEAK","RMS","PLR"};float vals[]={processor.outIntegrated.load(),processor.outShort.load(),processor.truePeak.load(),processor.outRms.load(),processor.truePeak.load()-processor.outIntegrated.load()};int xs[]={435,965,1085,1205,1325};
    for(int i=0;i<5;++i){text(g,labels[i],{(float)xs[i],423,110,22},14,dim);text(g,reading(vals[i]),{(float)xs[i],448,110,32},25);}
}
void BaakEditor::drawGraph(Graphics& g){
    int view=views.getSelectedId();
    if(view==1){
        drawSpectrum(g,{25,214,365,176},true);text(g,"IMAGE",{403,190,170,25},17,dim);drawScope(g,{405,220,170,158});drawWave(g,{55,406,510,72});
        text(g,"LUFS-I",{620,174,250,30},20,dim);text(g,reading(processor.outIntegrated.load()),{625,207,240,65},51);
        text(g,"LUFS-S       LUFS-M       PLR",{605,304,300,24},13,dim);
        text(g,reading(processor.outShort.load())+"       "+reading(processor.outMomentary.load())+"       "+reading(processor.truePeak.load()-processor.outIntegrated.load()),{600,333,310,35},23);
        float values[]={processor.rmsL.load(),processor.rmsR.load(),processor.peakL.load(),processor.peakR.load()};
        text(g,"RMS",{947,190,180,28},18,dim);text(g,"TRUE PEAK",{1154,190,180,28},18,dim);text(g,"CREST",{1371,190,100,28},18,dim);
        for(int i=0;i<4;++i){float x=970+(i/2)*205+(i%2)*65;auto r=Rectangle<float>(x,250,25,180);g.setColour(pink.withAlpha(.17f));g.fillRect(r);float amount=jlimit(0.f,1.f,(values[i]+60)/66);g.setColour(pink);g.fillRect(r.withTop(r.getBottom()-amount*r.getHeight()));text(g,i%2?"R":"L",{x-5,228,35,16},10,dim);text(g,reading(values[i]),{x-19,443,64,25},20);}
        float crest=processor.truePeak.load()-processor.outRms.load();auto r=Rectangle<float>(1404,250,30,180);g.setColour(pink.withAlpha(.17f));g.fillRect(r);g.setColour(pink);g.fillRect(r.withTop(r.getBottom()-jlimit(0.f,1.f,crest/24)*180));text(g,String(crest,1),{1374,443,90,25},20);return;
    }
    if(view==2){drawSpectrum(g,{12,169,1475,248},false);drawMetrics(g);return;}
    if(view==3){drawWave(g,{40,204,1420,199});text(g,"Audio peak envelope / pale line: limiter reduction",{470,174,570,20},12,dim);drawMetrics(g);return;}
    if(view==4){
        Rectangle<float> r(40,190,1420,210);for(int i=0;i<9;++i){float y=r.getY()+i*r.getHeight()/8;g.setColour(pale.withAlpha(.12f));g.drawHorizontalLine(roundToInt(y),r.getX(),r.getRight());text(g,String(12-i*3),{10,y-7,28,14},10,dim);}
        auto p=processor.current();Biquad low,mid,high;low.configure(2,processor.analysisRate.load(),120,p[Low]);mid.configure(1,processor.analysisRate.load(),p[MidFreq],p[Mid],.7);high.configure(3,processor.analysisRate.load(),8000,p[High]);
        Path response,phasePath;for(int i=0;i<600;++i){double hz=20*std::pow(1000.,i/599.);auto z=std::exp(std::complex<double>(0,-2*pi*hz/processor.analysisRate.load()));std::complex<double> h=1;for(auto* f:{&low,&mid,&high})h*=(f->b0+f->b1*z+f->b2*z*z)/(1.+f->a1*z+f->a2*z*z);float d=(float)(20*std::log10(std::max(1.e-12,std::abs(h))));float x=r.getX()+i*r.getWidth()/599,y=r.getCentreY()-jlimit(-12.f,12.f,d)*r.getHeight()/24,py=r.getCentreY()-(float)std::arg(h)*r.getHeight()/(2*MathConstants<float>::pi);if(i==0){response.startNewSubPath(x,y);phasePath.startNewSubPath(x,py);}else{response.lineTo(x,y);phasePath.lineTo(x,py);}}
        trace(g,response,pale,2);if(phase.getToggleState())trace(g,phasePath,pink.withAlpha(.55f));
        text(g,"Static EQ response (+/-12 dB)"+String(phase.getToggleState()?"  /  pink: phase (+/-180 degrees)":""),{430,160,650,20},12,dim);drawMetrics(g);return;
    }
    if(view==5){
        auto r=Rectangle<float>(185,192,1040,48);g.setColour(pink.withAlpha(.4f));g.drawRoundedRectangle(r,5,1);
        if(processor.referenceName.isEmpty())text(g,"Drop reference audio here",r,16,dim);
        else{for(int i=0;i<512;++i){float x=r.getX()+i*r.getWidth()/512,h=processor.referenceWave[i].load()*r.getHeight()*.46f;g.setColour(pink.withAlpha(.65f));g.drawVerticalLine(roundToInt(x),r.getCentreY()-h,r.getCentreY()+h);}text(g,processor.referenceName,{185,164,1040,22},13,pale,Justification::centredLeft);}
        text(g,"A "+String(processor.loopStart.load(),2)+" s     B "+String(processor.loopEnd.load(),2)+" s",{540,281,430,30},13,dim);
        drawReferenceComparison(g);return;
    }
    text(g,"LUFS-I IN",{140,211,420,40},24,dim);text(g,"LUFS-I OUT",{940,211,420,40},24,dim);
    text(g,reading(processor.inIntegrated.load()),{140,266,420,125},91,pale);text(g,reading(processor.outIntegrated.load()),{940,266,420,125},91,pale);
}
void BaakEditor::timerCallback(){
    std::vector<float> in,out,side;processor.readAnalysis(in,out,side);float peak=0;
    for(size_t i=0;i<in.size();++i){inputRing[ringPos]=in[i];outputRing[ringPos]=out[i];sideRing[ringPos]=side[i];peak=std::max(peak,std::max(std::abs(out[i]+side[i]),std::abs(out[i]-side[i])));ringPos=(ringPos+1)%2048;}
    float decay=1-std::exp(-1000.f/(30*(float)releaseTime.getValue()));
    if(!in.empty())for(int pass=0;pass<2;++pass){auto& ring=pass?outputRing:inputRing;auto& bins=pass?outputSpectrum:inputSpectrum;fftData.fill(0);for(int i=0;i<2048;++i)fftData[i]=ring[(ringPos+i)%2048]*(.5f-.5f*std::cos(2*MathConstants<float>::pi*i/2047));fft.performFrequencyOnlyForwardTransform(fftData.data());for(int i=0;i<1024;++i){float v=Decibels::gainToDecibels(fftData[i]/512.f,-100.f);bins[i]+=(v-bins[i])*(v>bins[i]?.8f:decay);}}
    if(!freeze.getToggleState()){waveHistory[historyPos]=peak;reductionHistory[historyPos]=processor.gainReduction.load();historyPos=(historyPos+1)%600;}
    auto p=processor.current();bypass.setToggleState(p[Bypass]>.5f,dontSendNotification);unity.setToggleState(p[Unity]>.5f,dontSendNotification);dc.setToggleState(p[DC]>.5f,dontSendNotification);tp.setToggleState(p[TruePeak]>.5f,dontSendNotification);ll.setToggleState(p[LowLatency]>.5f,dontSendNotification);
    quality.setSelectedId(roundToInt(p[Quality])+1,dontSendNotification);monitor.setSelectedId(roundToInt(p[Monitor])+1,dontSendNotification);
    for(int i=0;i<3;++i)slots[i].setToggleState(i==activeSlot,dontSendNotification);
    undo.setEnabled(!undoStates.empty());redo.setEnabled(!redoStates.empty());audition.setEnabled(!processor.referenceName.isEmpty());audition.setToggleState(processor.referencePlaying.load(),dontSendNotification);master.setToggleState(!processor.referencePlaying.load(),dontSendNotification);loop.setToggleState(processor.loopEnabled.load(),dontSendNotification);beatSnap.setToggleState(processor.snapToBeat.load(),dontSendNotification);beatSnap.setEnabled(processor.referenceTempo.load()>0);
    double length=processor.reference.getLengthInSeconds();if(length>0){seek.setRange(0,length,.01);if(!seek.isMouseButtonDown())seek.setValue(processor.reference.getCurrentPosition(),dontSendNotification);}repaint();
}
bool BaakEditor::isInterestedInFileDrag(const StringArray& files){return files.size()==1&&File(files[0]).hasFileExtension("wav;aiff;aif;flac;mp3");}
void BaakEditor::filesDropped(const StringArray& files,int,int){if(files.size())openReference(File(files[0]));}
void BaakEditor::openReference(const File& file){if(!processor.loadReference(file))AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon,"Reference audio","This file could not be opened.");else views.setSelectedId(5);repaint();}

File BaakEditor::presetDirectory() const{return File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("BaakProduction/BAD-I Master Pro/Presets");}
void BaakEditor::buildPresets(){
    factory.clear();auto add=[this](const char* name,const char* category,std::initializer_list<std::pair<int,float>> changes){auto p=defaults();for(auto c:changes)p[c.first]=c.second;factory.push_back({name,category,p,{}});};
    add("Default","Versatile",{});add("Balanced master","Versatile",{{Glue,12},{Loud,2}});add("Clean and open","Versatile",{{DeMud,18},{High,.6f},{Loud,2}});add("Gentle dynamics","Acoustic",{{Glue,18},{Loud,1},{Calm,12}});add("Warm room","Acoustic",{{Tape,18},{Low,.5f},{ToneColor,-12}});add("Natural detail","Acoustic",{{Body,14},{High,.7f},{Loud,1}});
    add("Punch and space","Rock / Alt",{{Width,110},{Glue,18},{Loud,3}});add("Dense guitars","Rock / Alt",{{Calm,30},{Mid,-.8f},{MidFreq,2800},{Loud,3}});add("Wide chorus","Rock / Alt",{{Width,120},{BassMono,60},{High,.4f},{Loud,2}});add("Low end control","Hip-hop / Rap",{{LowComp,28},{DeMud,20},{Loud,3}});add("Warm drums","Hip-hop / Rap",{{Tape,24},{Warmth,12},{Loud,3}});add("Vocal focus","Hip-hop / Rap",{{Mid,.6f},{MidFreq,1600},{Calm,22},{Loud,2}});
    add("Club balance","House / EDM",{{BassMono,75},{LowComp,20},{HighComp,12},{Loud,4}});add("Tight lows","House / EDM",{{Tape,14},{TapeTone,60},{DeMud,25},{Loud,3}});add("Bright motion","House / EDM",{{High,.9f},{Width,112},{Silk,20},{Loud,3}});add("Intimate jazz","Jazz",{{Glue,10},{Tape,10},{Loud,1}});add("Soft top","Jazz",{{Calm,20},{High,-.4f}});add("Air and room","Jazz",{{Width,106},{High,.6f},{Body,10}});
    add("Bright pop","Pop",{{High,.8f},{Glue,18},{Loud,3}});add("Soft vocal pop","Pop",{{Calm,24},{Tape,12},{Loud,2}});add("Silky R&B","R&B",{{Silk,22},{DeMud,15},{Tape,12},{Loud,2}});add("Low and warm","R&B",{{Low,.5f},{Warmth,16},{Tape,14},{Loud,2}});add("Film dynamics","Film",{{Glue,6},{Width,108}});add("Quiet detail","Film",{{Body,20},{Calm,10},{Loud,1}});
    auto directory=presetDirectory();if(directory.isDirectory())for(auto f:directory.findChildFiles(File::findFiles,false,"*.badipreset")){if(auto xml=XmlDocument::parse(f)){auto v=ValueTree::fromXml(*xml);if(v.hasType("BADIPreset")){auto p=defaults();for(int i=0;i<Count;++i)p[i]=(float)v.getProperty(specs[i].id,p[i]);factory.push_back({f.getFileNameWithoutExtension(),"User",p,f});}}}
}
void BaakEditor::applyPreset(int index){if(index<0||index>=(int)factory.size())return;saveUndo();auto& preset=factory[(size_t)index];for(int i=0;i<Count;++i){if(i==Bypass||i==Monitor||i==Quality||i==LowLatency)continue;processor.setValue(i,preset.values[i]);}presetButton.setButtonText(preset.name);browser->setVisible(false);}
void BaakEditor::savePreset(){
    auto dir=presetDirectory();dir.createDirectory();chooser=std::make_unique<FileChooser>("Save BAD-I preset",dir.getChildFile("My master.badipreset"),"*.badipreset");Component::SafePointer<BaakEditor> safe(this);
    chooser->launchAsync(FileBrowserComponent::saveMode|FileBrowserComponent::canSelectFiles|FileBrowserComponent::warnAboutOverwriting,[safe](const FileChooser& fc){if(!safe)return;auto f=fc.getResult();if(f==File{})return;ValueTree v("BADIPreset");auto p=safe->processor.current();for(int i=0;i<Count;++i)v.setProperty(specs[i].id,p[i],nullptr);if(auto xml=v.createXml()){if(!xml->writeTo(f.withFileExtension("badipreset")))AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon,"Preset","The preset could not be saved.");}safe->buildPresets();safe->browser->update();});
}
BaakEditor::PresetPanel::PresetPanel(BaakEditor& o):owner(o){
    for(auto* c:std::initializer_list<Component*>{&search,&category,&save,&folder,&close,&pagePrev,&pageNext})addAndMakeVisible(*c);search.setTextToShowWhenEmpty("Search presets...",dim);search.onTextChange=[this]{update();};category.addItemList({"All","Favorites","Versatile","Acoustic","Rock / Alt","Hip-hop / Rap","House / EDM","Jazz","Pop","R&B","Film","User"},1);category.setSelectedId(1);category.onChange=[this]{update();};save.onClick=[this]{owner.savePreset();};folder.onClick=[this]{auto dir=owner.presetDirectory();dir.createDirectory();dir.revealToUser();};close.onClick=[this]{setVisible(false);};pagePrev.onClick=[this]{page=std::max(0,page-1);repaint();};pageNext.onClick=[this]{page=std::min((int)(filtered.size()?filtered.size()-1:0)/24,page+1);repaint();};update();
}
void BaakEditor::PresetPanel::update(){page=0;filtered.clear();String favorites=owner.processor.state.state.getProperty("favorites").toString();for(size_t i=0;i<owner.factory.size();++i){auto& p=owner.factory[i];if((category.getSelectedId()==1||category.getText()==p.category||(category.getText()=="Favorites"&&favorites.contains("\n"+p.name+"\n")))&&p.name.containsIgnoreCase(search.getText()))filtered.push_back((int)i);}repaint();}
void BaakEditor::PresetPanel::resized(){float sx=getWidth()/1165.f,sy=getHeight()/366.f;auto place=[&](Component& c,int x,int y,int w,int h){c.setBounds(roundToInt(x*sx),roundToInt(y*sy),roundToInt(w*sx),roundToInt(h*sy));};place(search,10,10,210,35);place(category,10,56,210,35);place(save,10,178,210,32);place(folder,10,220,210,32);place(close,10,262,210,32);place(pagePrev,700,320,180,30);place(pageNext,900,320,180,30);}
void BaakEditor::PresetPanel::paint(Graphics& g){
    g.addTransform(AffineTransform::scale(getWidth()/1165.f,getHeight()/366.f));g.setColour(juce::Colour(0xff210d19));g.fillRoundedRectangle(0,0,1165,366,5);g.setColour(pink.withAlpha(.6f));g.drawRoundedRectangle(1,1,1163,364,5,1);
    String favorites=owner.processor.state.state.getProperty("favorites").toString();
    for(int n=0;n<24&&page*24+n<(int)filtered.size();++n){int x=233+(n%4)*230,y=11+(n/4)*47;auto& preset=owner.factory[(size_t)filtered[(size_t)(page*24+n)]];g.setColour(button);g.fillRoundedRectangle((float)x,(float)y,224,40,3);text(g,preset.name,{(float)x+8,(float)y,186,40},15,pale,Justification::centredLeft);text(g,favorites.contains("\n"+preset.name+"\n")?"*":"+",{(float)x+195,(float)y,25,40},20,pink);}
    if(filtered.empty())text(g,"No presets match this search",{250,100,870,80},18,dim);text(g,"Original BAD-I settings",{15,105,205,35},12,dim);text(g,"+ adds a favorite",{15,136,205,25},12,dim);text(g,"Page "+String(page+1)+" / "+String(std::max(1,((int)filtered.size()+23)/24)),{245,320,400,30},13,dim);
}
void BaakEditor::PresetPanel::mouseDown(const MouseEvent& e){
    int x=roundToInt(e.x*1165.f/getWidth()),y=roundToInt(e.y*366.f/getHeight());if(x<233||y<11)return;int col=(x-233)/230,row=(y-11)/47,index=page*24+row*4+col;
    if(col<4&&row<6&&index<(int)filtered.size()){int selected=filtered[(size_t)index];if((x-233)%230>=195){String key="\n"+owner.factory[(size_t)selected].name+"\n",favorites=owner.processor.state.state.getProperty("favorites").toString();if(favorites.contains(key))favorites=favorites.replace(key,"");else favorites+=key;owner.processor.state.state.setProperty("favorites",favorites,nullptr);update();}else owner.applyPreset(selected);}
}

void BaakEditor::drawReferenceComparison(Graphics& g){
    bool ready=processor.referenceAnalysed.load();
    auto graph=Rectangle<float>(50,358,500,115);
    for(int pass=0;pass<2;++pass){Path path;for(int i=0;i<240;++i){float hz=20*std::pow(1000.f,i/239.f);int bin=jlimit(1,1023,roundToInt(hz*2048/processor.analysisRate.load()));float value=pass?processor.referenceSpectrum[jlimit(1,1023,roundToInt(hz*2048/processor.referenceRate.load()))].load()+processor.referenceGain.load():outputSpectrum[bin];float x=graph.getX()+i*graph.getWidth()/239,y=graph.getBottom()-jlimit(0.f,1.f,(value+90)/90)*graph.getHeight();if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}trace(g,path,pass?pink:pale);}
    text(g,"Pale: master / pink: reference average",{50,331,500,22},12,dim);
    const char* labels[]={"LUFS-I","TRUE PEAK","RMS","PLR"};
    float m[]={processor.outIntegrated.load(),processor.truePeak.load(),processor.outRms.load(),processor.truePeak.load()-processor.outIntegrated.load()};
    float trim=processor.referenceGain.load();
    float r[]={processor.referenceLufs.load()+trim,processor.referencePeak.load()+trim,processor.referenceRms.load()+trim,processor.referencePeak.load()-processor.referenceLufs.load()};
    for(int i=0;i<4;++i){float x=992+i*117.f;text(g,labels[i],{x,348,112,21},12,dim);text(g,reading(m[i]),{x,376,112,23},19);text(g,ready?reading(r[i]):"--",{x,407,112,23},19,pink);text(g,ready?String(m[i]-r[i],1):"--",{x,438,112,23},17,dim);}
    text(g,"M",{954,376,30,23},15,dim);text(g,"R",{954,407,30,23},15,dim);text(g,"Delta",{933,438,50,23},12,dim);
    String status=ready?"Reference: full-file measurements":"Analysing reference: "+String(roundToInt(processor.referenceProgress.load()*100))+"%";
    if(processor.referenceName.isEmpty())status="Load audio to compare";
    if(processor.referenceTempo.load()>0)status+="  /  estimated "+String(processor.referenceTempo.load(),1)+" BPM";
    text(g,status,{913,473,563,18},11,dim);
}

void BaakEditor::Theme::drawButtonText(Graphics& g,TextButton& b,bool,bool){
    float size=(bool)b.getProperties().getWithDefault("label",false)?(b.getHeight()<22?11.f:19.f):14.f;
    g.setFont(Font(FontOptions("Segoe UI",size,Font::plain)));g.setColour(b.isEnabled()?pale:dim.withAlpha(.4f));g.drawFittedText(b.getButtonText(),b.getLocalBounds().reduced(2),Justification::centred,1);
}
Label* BaakEditor::Theme::createSliderTextBox(Slider& slider){
    auto* label=LookAndFeel_V4::createSliderTextBox(slider);
    label->setColour(Label::outlineColourId,Colours::transparentBlack);label->setColour(Label::backgroundColourId,Colours::transparentBlack);label->setColour(Label::textColourId,dim);label->setFont(Font(FontOptions(12.f)));return label;
}
void BaakEditor::ResetSlider::mouseDoubleClick(const MouseEvent&){
    entry=std::make_unique<AlertWindow>(getTitle(),"Enter a value",MessageBoxIconType::NoIcon);
    entry->addTextEditor("value",String(getValue(),2));entry->addButton("Apply",1,KeyPress(KeyPress::returnKey));entry->addButton("Cancel",0,KeyPress(KeyPress::escapeKey));
    Component::SafePointer<ResetSlider> safe(this);entry->enterModalState(true,ModalCallbackFunction::create([safe](int result){if(safe&&result==1){if(safe->onDragStart)safe->onDragStart();safe->setValue(safe->entry->getTextEditorContents("value").getDoubleValue());if(safe->onDragEnd)safe->onDragEnd();}}),false);
}
