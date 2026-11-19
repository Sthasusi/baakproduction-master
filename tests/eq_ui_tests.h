#pragma once

inline void runEqAndHoverTests(const juce::File& folder) {
    // Known stereo power ratios, all supported rates, independent of the DSP.
    for(double rate : {44100.,48000.,96000.,192000.}) {
        auto a=std::make_unique<baak::EqAnalysis>();a->reset(rate);
        uint32_t random=12345;
        auto feed=[&](float level,bool sideOnly,int count){
            for(int n=0;n<count;++n){random=random*1664525u+1013904223u;float x=((random>>8)/8388608.f-1)*.15f;
                a->push(sideOnly?0:x,sideOnly?x:0,sideOnly?0:x*level,sideOnly?x*level:0,60);}
        };
        feed(2,false,(int)(rate*.4)+baak::EqAnalysis::size);
        assert(a->hasSignal());assert(std::abs(a->at(1000)-6.0206f)<.05f);
        a->reset(rate);feed(.5f,true,(int)(rate*.4)+baak::EqAnalysis::size);
        assert(a->hasSignal());assert(std::abs(a->at(1000)+6.0206f)<.05f);
        feed(1,true,(int)(rate*.7)+baak::EqAnalysis::size);
        assert(std::abs(a->at(1000))<.15f); // Updates while audio changes.
        for(int i=0;i<baak::EqAnalysis::size*2;++i)a->push(0,0,0,0,60);
        assert(!a->hasSignal());
        for(float v:a->change)assert(std::isfinite(v));
        a->reset(rate);assert(!a->hasSignal()&&a->frames==0);
    }
    auto params=baak::defaults();params[baak::Clean]=100;
    assert(20*std::log10(std::abs(baak::eqFilterResponse(params,96000,30))) < -15);
    params[baak::Bypass]=1;assert(std::abs(baak::eqFilterResponse(params,96000,30)-1.)<1.e-9);

    auto processor=std::make_unique<BaakProcessor>();
    processor->setPlayConfigDetails(2,2,48000,256);processor->prepareToPlay(48000,256);
    processor->setValue(baak::Low,4);
    auto editor=std::make_unique<BaakEditor>(*processor);editor->setVisible(true);editor->setViewForTest(4);
    juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;int64_t sample=0;
    auto play=[&](int blocks,bool sideOnly=false,bool silence=false){
        for(int b=0;b<blocks;++b){
            for(int n=0;n<256;++n,++sample){double t=sample/48000.;float v=silence?0.f:(float)(.24*std::sin(2*baak::pi*100*t)+.1*std::sin(2*baak::pi*1200*t)+.04*std::sin(2*baak::pi*7000*t));audio.setSample(0,n,v);audio.setSample(1,n,sideOnly?-v:v);}
            processor->processBlock(audio,midi);
            if(b%5==0)editor->pumpAnalysisForTest();
        }
        editor->pumpAnalysisForTest();
    };
    auto render=[&](const juce::String& name){if(folder==juce::File{})return;auto stream=folder.getChildFile(name+".png").createOutputStream();juce::PNGImageFormat png;assert(stream&&png.writeImageToStream(editor->createComponentSnapshot(editor->getLocalBounds()),*stream));};
    play(400);assert(editor->eqAnalysisForTest().hasSignal());
    const float before=editor->eqAnalysisForTest().at(100);
    assert(before>editor->eqAnalysisForTest().at(1200)+1.f);
    render("EQ-live-before");
    processor->setValue(baak::LowComp,100);play(400);
    assert(editor->eqAnalysisForTest().at(100)<before-.5f);
    render("EQ-live-dynamics");
    // Preserve the existing resize renders without informational overlays.
    for(auto size : {juce::Point<int>(1050,630),juce::Point<int>(1500,900),juce::Point<int>(2100,1260)}){
        editor->setSize(size.x,size.y);
        render("EQ-navy-"+juce::String(size.x));
    }
    editor->setSize(1500,900);
    juce::TextButton* settings=nullptr;juce::TextButton* phase=nullptr;juce::ComboBox* monitor=nullptr;
    for(auto* child:editor->getChildren()){
        if(auto* b=dynamic_cast<juce::TextButton*>(child)){if(b->getButtonText()=="...")settings=b;if(b->getButtonText()=="Show phase")phase=b;}
        if(auto* box=dynamic_cast<juce::ComboBox*>(child))if(box->getText()=="Normal")monitor=box;
    }
    assert(settings&&phase&&monitor);settings->onClick();
    assert(monitor->isVisible()&&phase->isVisible()&&!monitor->getBounds().intersects(phase->getBounds()));
    render("EQ-settings");settings->onClick();
    for(auto* child:editor->getChildren())if(auto* button=dynamic_cast<juce::TextButton*>(child))if(button->getButtonText()=="Show phase")button->setToggleState(true,juce::dontSendNotification);
    render("EQ-phase");
    processor->setValue(baak::Bypass,1);play(400);
    assert(std::abs(editor->eqAnalysisForTest().at(100))<.15f);
    render("EQ-bypass");
    processor->setValue(baak::Bypass,0);play(400,true);
    assert(editor->eqAnalysisForTest().hasSignal());render("EQ-side-only");
    play(100,false,true);assert(!editor->eqAnalysisForTest().hasSignal());render("EQ-silence");
    processor->prepareToPlay(96000,256);editor->pumpAnalysisForTest();
    assert(editor->eqAnalysisForTest().frames==0);
    // Broadband, stereo programme-like stimulus for a readable native UI render.
    processor->setPlayConfigDetails(2,2,48000,256);processor->prepareToPlay(48000,256);
    processor->setValue(baak::Mid,-1.5f);processor->setValue(baak::High,1.5f);
    processor->setValue(baak::Calm,40);processor->setValue(baak::DeMud,35);
    processor->setValue(baak::Tape,25);processor->setValue(baak::Loud,2);
    uint32_t noise=9451;float smooth=0;
    for(int b=0;b<500;++b){
        for(int n=0;n<256;++n,++sample){noise=noise*1664525u+1013904223u;float v=(noise>>8)/8388608.f-1;smooth+=(v-smooth)*.12f;float t=(float)(sample/48000.);float envelope=.6f+.4f*std::sin(2*baak::pi*1.7*t);float tone=.18f*std::sin(2*baak::pi*80*t)*envelope;audio.setSample(0,n,tone+.22f*smooth+.035f*v);audio.setSample(1,n,tone+.22f*smooth-.035f*v);}
        processor->processBlock(audio,midi);if(b%5==0)editor->pumpAnalysisForTest();
    }
    editor->pumpAnalysisForTest();assert(editor->eqAnalysisForTest().hasSignal());
    phase->setToggleState(false,juce::dontSendNotification);render("EQ-programme");
    std::cout<<"PASS: live EQ ratios at four rates, opposite-polarity stereo, dynamic response, silence, reset, real processor EQ/compression/bypass, native navy interface at three sizes.\n";
}
