#pragma once

inline void runReferenceTests(const juce::File& folder,const juce::File& assets) {
    auto wav=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("badi-reference-mono-44100",".wav");
    {
        auto stream=wav.createOutputStream();assert(stream);
        const int rate=44100,frames=rate,bytes=frames*2;
        stream->write("RIFF",4);stream->writeInt(36+bytes);stream->write("WAVEfmt ",8);stream->writeInt(16);stream->writeShort(1);stream->writeShort(1);stream->writeInt(rate);stream->writeInt(rate*2);stream->writeShort(2);stream->writeShort(16);stream->write("data",4);stream->writeInt(bytes);
        for(int i=0;i<frames;++i)stream->writeShort((short)(10000*std::sin(2*baak::pi*(i<rate/2?440:880)*i/rate)));
    }
    auto p=std::make_unique<BaakProcessor>();
    assert(p->loadReference(wav));
    // Loading before the DAW starts its audio engine must retain duration/loop end.
    assert(std::abs(p->referenceLength.load()-1.)<.001);assert(p->loopEnd.load()>.99);
    p->seekReference(.25);p->setPlayConfigDetails(2,2,48000,256);p->prepareToPlay(48000,256);
    juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;
    auto process=[&](int blocks){double energy=0;for(int b=0;b<blocks;++b){audio.clear();p->processBlock(audio,midi);energy+=audio.getRMSLevel(0,0,256);for(int n=0;n<256;++n)assert(std::isfinite(audio.getSample(0,n)));juce::Thread::sleep(1);}return energy;};
    p->playReference(true);assert(process(75)>1); // Initial audition.
    assert(process(300)>1);assert(process(200)>1); // Audible after several EOF wraps.
    assert(p->referencePlaying.load()&&p->reference.getCurrentPosition()<1.01);
    p->loopStart.store(.2);p->loopEnd.store(.55);p->seekReference(.2);
    assert(process(300)>1);assert(p->referencePlaying.load());
    assert(p->referencePosition()>=.19&&p->referencePosition()<.57);
    p->playReference(false);process(100);assert(audio.getRMSLevel(0,0,256)<.0001f);
    p->playReference(true);assert(process(50)>1);
    p->loopEnabled.store(false);p->seekReference(.8);process(5);assert(p->referencePosition()>.79);
    process(100);assert(!p->referencePlaying.load());
    p->playReference(true);assert(process(50)>1); // Replay after EOF rewinds.
    p->playReference(false);
    p->prepareToPlay(96000,256);assert(std::abs(p->referenceLength.load()-1.)<.001);
    p->setPlayConfigDetails(2,2,48000,256);p->prepareToPlay(48000,256);
    auto invalid=wav.getParentDirectory().getNonexistentChildFile("badi-reference-invalid",".mp3");invalid.replaceWithText("not an audio file");
    auto originalName=p->referenceName;assert(!p->loadReference(invalid));assert(p->referenceError.isNotEmpty());assert(p->referenceName==originalName&&p->referenceReady());invalid.deleteFile();
    // FLAC round-trip exercises the compressed-file path with a different rate.
    auto flac=wav.withFileExtension(".flac");
    {
        juce::FlacAudioFormat format;auto stream=flac.createOutputStream();assert(stream);
        auto writer=std::unique_ptr<juce::AudioFormatWriter>(format.createWriterFor(stream.release(),96000,2,16,{},5));assert(writer);
        juce::AudioBuffer<float> data(2,96000);for(int i=0;i<96000;++i){float v=.2f*std::sin(2*baak::pi*550*i/96000);data.setSample(0,i,v);data.setSample(1,i,v);}
        assert(writer->writeFromAudioSampleBuffer(data,0,data.getNumSamples()));
    }
    assert(p->loadReference(flac));assert(p->referenceError.isEmpty());p->playReference(true);assert(process(60)>.5);
    if(assets.isDirectory())for(auto name: {"Notifications/sounds/served.mp3","singing.ogg"}){
        const auto file=assets.getChildFile(name);assert(file.existsAsFile());assert(p->loadReference(file));
        assert(p->referenceLength.load()>0);p->playReference(true);assert(process(100)>.01);
        std::cout<<"PASS: decoded reference "<<name<<"\n";
    }
    assert(p->loadReference(wav));p->loopEnabled.store(true);p->playReference(false);
    for(int wait=0;wait<500&&!p->referenceAnalysed.load();++wait)juce::Thread::sleep(5);
    assert(p->referenceAnalysed.load()&&!p->referenceAnalysisFailed.load());assert(p->referenceWave[100].load()>.1f);
    auto editor=std::make_unique<BaakEditor>(*p);editor->setVisible(true);editor->setViewForTest(5);editor->pumpAnalysisForTest();
    auto click=[&](const juce::String& name){for(auto* child:editor->getChildren())if(auto* b=dynamic_cast<juce::TextButton*>(child))if(b->getButtonText()==name){assert(b->isEnabled());b->onClick();editor->pumpAnalysisForTest();return true;}return false;};
    assert(click("Play reference"));assert(process(80)>1);assert(click("Pause reference"));assert(!p->referencePlaying.load());
    p->seekReference(.6);assert(click("Restart"));assert(p->referencePosition()<.01);
    editor->filesDropped({flac.getFullPathName()},0,0);assert(p->referenceName==flac.getFileName());
    for(int wait=0;wait<500&&!p->referenceAnalysed.load();++wait)juce::Thread::sleep(5);
    assert(p->referenceAnalysed.load());assert(click("Play reference"));assert(process(80)>.5);editor->pumpAnalysisForTest();
    if(folder!=juce::File{})for(auto size:{juce::Point<int>(1500,900),juce::Point<int>(1050,630)}){
        editor->setSize(size.x,size.y);auto stream=folder.getChildFile("Reference-playing-"+juce::String(size.x)+".png").createOutputStream();juce::PNGImageFormat png;assert(stream&&png.writeImageToStream(editor->createComponentSnapshot(editor->getLocalBounds()),*stream));
    }
    assert(click("Master"));assert(!p->referencePlaying.load());editor.reset();p.reset();wav.deleteFile();flac.deleteFile();
    std::cout<<"PASS: reference loading before prepare, sample-rate changes, mono/stereo, WAV/FLAC, multiple EOF/subrange loops, pause, seeking, replay, failed-file recovery, drag/drop and editor playback controls.\n";
}
