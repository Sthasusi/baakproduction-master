#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace baak;
juce::AudioProcessorValueTreeState::ParameterLayout BaakProcessor::layout() {
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    for (int i = 0; i < Count; ++i) {
        auto &s = specs[i];
        float step = (i == Colour || i == Focus || i == Bypass || i == DC || i == Unity || i == TruePeak || i == Quality || i == LowLatency || i == Monitor) ? 1.f : .01f;
        juce::NormalisableRange<float> range(s.min, s.max, step);
        if (i == MidFreq)
            range.setSkewForCentre(1200);
        l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(s.id, 1), s.name, range,
                                                          s.initial));
    }
    return l;
}
BaakProcessor::BaakProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Stereo input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Stereo output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "BaakMasterState", layout()) {
    formats.registerBasicFormats(); referenceThread.startThread();
    for(auto& value:referenceSpectrum)value.store(-100);
    for (int i = 0; i < Count; ++i)
        values[i] = state.getRawParameterValue(specs[i].id);
    for (auto &s : snapshots)
        s = state.copyState();
}
bool BaakProcessor::isBusesLayoutSupported(const BusesLayout &l) const {
    return l.getMainInputChannelSet() == juce::AudioChannelSet::stereo() &&
           l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}
Params BaakProcessor::current() const {
    Params p{};
    for (int i = 0; i < Count; ++i)
        p[i] = values[i]->load();
    return p;
}
void BaakProcessor::prepareToPlay(double sr, int block) {
    analysisRate.store(sr); analysisFifo.reset(); ++analysisEpoch; unityGain=1; referenceMix=0;
    reference.prepareToPlay(std::max(1,block),sr);
    referenceScratch.setSize(2,std::max(1,block));
    auto p = current();
    p[Bypass] = 0;
    engine.set(p);
    int quality=juce::jlimit(0,3,juce::roundToInt(p[Quality]));
    bool lowLatency=p[LowLatency]>.5f;if(lowLatency)quality=0;
    activeQuality.store(quality);activeLowLatency.store(lowLatency);
    engine.prepare(sr * (1<<quality),lowLatency);
    peakGuard.prepare(sr);peakMeter.reset();heldTruePeaks={};
    previousMonitor=-1;monitorMix=0;
    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        2, static_cast<size_t>(quality), juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    oversampling->initProcessing(static_cast<size_t>(std::max(1, block)));
    oversampling->reset();
    latency = engine.latency() / (1<<quality) + baak::PeakGuard::latency + static_cast<int>(std::round(oversampling->getLatencyInSamples()));
    setLatencySamples(latency);
    dryScratch.setSize(2, std::max(1, block));
    dryRing.setSize(2, latency + 1);
    dryRing.clear();
    dryPos = 0;
    bypassMix = values[Bypass]->load();
    inputMeter.prepare(sr);
    outputMeter.prepare(sr); wetMeter.prepare(sr);
}
void BaakProcessor::run(juce::AudioBuffer<float> &buffer, bool hostBypass) {
    juce::ScopedNoDenormals noDenormals;
    if (buffer.getNumChannels() < 2 || !oversampling)
        return;
    if (resetMeters.exchange(false)) {
        inputMeter.prepare(getSampleRate());
        outputMeter.prepare(getSampleRate()); wetMeter.prepare(getSampleRate());
        heldTruePeaks={};peakMeter.reset(); ++analysisEpoch;
    }
    auto p = current();
    float bypass = hostBypass ? 1.f : p[Bypass];
    p[Bypass] = 0;
    engine.set(p);
    float desiredUnity = 1;
    if(p[Unity]>.5f && inputMeter.shortLufs>-65 && wetMeter.shortLufs>-65)
        desiredUnity=std::clamp(gain(inputMeter.shortLufs-wetMeter.shortLufs),.01f,1.f);
    // Store raw input in preallocated ring while retaining its delayed version in a scratch buffer.
    // Scratch capacity comes from host maximum block size; split unusually large blocks below.
    const int maxBlock = std::max(1, getBlockSize());
    float maxGr = 0;
    for (int offset = 0; offset < buffer.getNumSamples(); offset += maxBlock) {
        int n = std::min(maxBlock, buffer.getNumSamples() - offset);
        for (int j = 0; j < n; ++j) {
            for (int c = 0; c < 2; ++c)
                buffer.setSample(c, offset + j, sane(buffer.getSample(c, offset + j)));
            inputMeter.push(buffer.getSample(0, offset + j), buffer.getSample(1, offset + j));
        }
        juce::dsp::AudioBlock<float> all(buffer);
        auto part = all.getSubBlock(static_cast<size_t>(offset), static_cast<size_t>(n));
        // A second delay ring is not needed: raw samples are held in a preallocated dry scratch.
        for (int j = 0; j < n; ++j)
            for (int c = 0; c < 2; ++c)
                dryScratch.setSample(c, j, buffer.getSample(c, offset + j));
        auto up = oversampling->processSamplesUp(part);
        for (size_t j = 0; j < up.getNumSamples(); ++j) {
            auto &l = up.getChannelPointer(0)[j];
            auto &r = up.getChannelPointer(1)[j];
            engine.process(l, r);
            maxGr = std::max(maxGr, engine.reduction);
        }
        oversampling->processSamplesDown(part);
        float smoothing = 1 - std::exp(-1.f / static_cast<float>(getSampleRate() * .015));
        for (int j = 0; j < n; ++j) {
            wetMeter.push(buffer.getSample(0,offset+j),buffer.getSample(1,offset+j));
            int read = (dryPos + 1) % dryRing.getNumSamples();
            bypassMix += (bypass - bypassMix) * smoothing;
            unityGain += (desiredUnity-unityGain)*(1-std::exp(-1.f/static_cast<float>(getSampleRate()*.7)));
            const float ceiling = gain(p[Ceiling] + p[Output]);
            float protectedL=buffer.getSample(0,offset+j),protectedR=buffer.getSample(1,offset+j);
            peakGuard.process(protectedL,protectedR,ceiling,p[TruePeak]>.5f);
            buffer.setSample(0,offset+j,protectedL);buffer.setSample(1,offset+j,protectedR);
            for (int c = 0; c < 2; ++c) {
                dryRing.setSample(c, dryPos, sane(dryScratch.getSample(c, j)));
                float dry = dryRing.getSample(c, read);
                float wet = std::clamp(buffer.getSample(c, offset + j), -ceiling, ceiling)*unityGain;
                buffer.setSample(c, offset + j, wet + (dry - wet) * bypassMix);
            }
            int start1,size1,start2,size2;
            analysisFifo.prepareToWrite(1,start1,size1,start2,size2);
            if(size1>0) {
                analysisInput[start1]=(dryRing.getSample(0,read)+dryRing.getSample(1,read))*.5f;
                analysisInputSide[start1]=(dryRing.getSample(0,read)-dryRing.getSample(1,read))*.5f;
                analysisOutput[start1]=(buffer.getSample(0,offset+j)+buffer.getSample(1,offset+j))*.5f;
                analysisSide[start1]=(buffer.getSample(0,offset+j)-buffer.getSample(1,offset+j))*.5f;
                analysisFifo.finishedWrite(1);
            }
            dryPos = read;
            outputMeter.push(buffer.getSample(0, offset + j), buffer.getSample(1, offset + j));
            auto tp=peakMeter.tick(buffer.getSample(0,offset+j),buffer.getSample(1,offset+j));
            for(int c=0;c<2;++c)heldTruePeaks[c]=std::max(heldTruePeaks[c],tp[c]);
        }
    }
    if(referencePlaying.load() || referenceMix>.0001f) {
        float step=1-std::exp(-1.f/static_cast<float>(getSampleRate()*.02));
        for(int offset=0;offset<buffer.getNumSamples();) {
            double start=loopStart.load(),end=loopEnd.load(),position=reference.getCurrentPosition();
            bool looping=loopEnabled.load()&&end>start+.05;
            if(looping&&(position>=end || position<start)){reference.setPosition(start);position=start;}
            int n=std::min(maxBlock,buffer.getNumSamples()-offset);
            if(looping)n=std::min(n,std::max(1,(int)std::ceil((end-position)*getSampleRate())));
            juce::AudioSourceChannelInfo info(&referenceScratch,0,n);reference.getNextAudioBlock(info);
            if(!looping&&position>=reference.getLengthInSeconds())referencePlaying.store(false);
            float target=referencePlaying.load()?1.f:0.f;
            for(int j=0;j<n;++j) {
                referenceMix+=(target-referenceMix)*step;
                double t=position+j/getSampleRate();
                float edge=looping?(float)std::clamp(std::min(t-start,end-t)/.003,0.,1.):1.f;
                for(int c=0;c<2;++c){
                    float original=buffer.getSample(c,offset+j);
                    float ref=sane(referenceScratch.getSample(c,j))*gain(referenceGain.load())*edge;
                    buffer.setSample(c,offset+j,original+(ref-original)*referenceMix);
                }
            }offset+=n;
        }
    }
    // Monitoring is deliberately downstream of master meters and reference switching.
    int monitor=juce::jlimit(0,5,juce::roundToInt(p[Monitor]));
    if(monitor!=previousMonitor){
        double low=monitor==3?200:monitor==4?450:80, high=monitor==3?5000:monitor==4?3400:12000;
        for(int c=0;c<2;++c){monitorHighpass[c].configure(0,getSampleRate(),low);monitorLowpass[c].configure(0,getSampleRate(),high);}
        previousMonitor=monitor;
    }
    for(int j=0;j<buffer.getNumSamples();++j){
        float l=buffer.getSample(0,j),r=buffer.getSample(1,j),m=(l+r)*.5f;
        for(int c=0;c<2;++c){float dry=c?r:l;float filtered=monitorHighpass[c].tick((monitor==4||monitor==5)?m:dry);filtered-=monitorLowpass[c].tick(filtered);
            float value=monitor==1?dry*gain(-12):monitor==2?m:monitor>=3?filtered:dry;
            buffer.setSample(c,j,sane(value));}
    }
    inIntegrated.store(inputMeter.integrated);outMomentary.store(outputMeter.momentary);
    truePeak.store(db(std::max(heldTruePeaks[0],heldTruePeaks[1])));
    rmsL.store(outputMeter.channelRms[0]);rmsR.store(outputMeter.channelRms[1]);
    peakL.store(db(heldTruePeaks[0]));peakR.store(db(heldTruePeaks[1]));
    inShort.store(inputMeter.shortLufs);
    outShort.store(outputMeter.shortLufs);
    outIntegrated.store(outputMeter.integrated);
    outPeak.store(outputMeter.peak);
    outRms.store(outputMeter.rms);
    correlation.store(outputMeter.correlation);
    gainReduction.store(maxGr);
    elapsed.store(static_cast<float>(outputMeter.seconds));
}
void BaakProcessor::processBlock(juce::AudioBuffer<float> &b, juce::MidiBuffer &) {
    run(b, false);
}
void BaakProcessor::processBlockBypassed(juce::AudioBuffer<float> &b, juce::MidiBuffer &) {
    run(b, true);
}
void BaakProcessor::setValue(int i, float v) {
    if (i < 0 || i >= Count)
        return;
    auto *p = state.getParameter(specs[i].id);
    p->beginChangeGesture();
    p->setValueNotifyingHost(p->convertTo0to1(std::clamp(v, specs[i].min, specs[i].max)));
    p->endChangeGesture();
}
void BaakProcessor::snapshot(int i) {
    if (i >= 0 && i < 3)
        snapshots[i] = state.copyState();
}
void BaakProcessor::recall(int i) {
    if (i < 0 || i >= 3 || !snapshots[i].isValid())
        return;
    for (auto child : snapshots[i]) {
        auto id = child.getProperty("id").toString();
        for (int p = 0; p < Count; ++p)
            if (id == specs[p].id)
                setValue(p, static_cast<float>(child.getProperty("value")));
    }
}
void BaakProcessor::getStateInformation(juce::MemoryBlock &d) {
    juce::ValueTree root("BaakProject");
    root.addChild(state.copyState(), -1, nullptr);
    root.setProperty("referencePath",referencePath,nullptr);root.setProperty("referenceGain",referenceGain.load(),nullptr);
    root.setProperty("loopStart",loopStart.load(),nullptr);root.setProperty("loopEnd",loopEnd.load(),nullptr);root.setProperty("loopEnabled",loopEnabled.load(),nullptr);
    for (int i = 0; i < 3; ++i) {
        juce::ValueTree slot("Snapshot");
        slot.setProperty("slot", i, nullptr);
        slot.addChild(snapshots[i].createCopy(), -1, nullptr);
        root.addChild(slot, -1, nullptr);
    }
    if (auto xml = root.createXml())
        copyXmlToBinary(*xml, d);
}
void BaakProcessor::setStateInformation(const void *d, int n) {
    if (auto xml = getXmlFromBinary(d, n)) {
        auto root = juce::ValueTree::fromXml(*xml);
        if (root.hasType("BaakProject")) {
            {juce::ScopedLock lock(restoreLock);pendingReference=root.getProperty("referencePath").toString();pendingStart=(double)root.getProperty("loopStart",0);pendingEnd=(double)root.getProperty("loopEnd",0);}
            referenceGain.store((float)root.getProperty("referenceGain",-6));loopEnabled.store((bool)root.getProperty("loopEnabled",true));triggerAsyncUpdate();
            auto v = root.getChildWithName("BaakMasterState");
            if (v.isValid())
                state.replaceState(v);
            for (auto child : root)
                if (child.hasType("Snapshot")) {
                    int i = child.getProperty("slot", -1);
                    if (i >= 0 && i < 3)
                        snapshots[i] = child.getChild(0).createCopy();
                }
        }
    }
}
juce::AudioProcessorEditor *BaakProcessor::createEditor() {
    return new BaakEditor(*this);
}
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new BaakProcessor();
}

bool BaakProcessor::loadReference(const juce::File& file) {
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    if(!reader) return false;
    scanStop.store(true);if(scanThread.joinable())scanThread.join();
    playReference(false); reference.setSource(nullptr);
    double sr=reader->sampleRate;referenceRate.store(sr);
    referenceSource=std::make_unique<juce::AudioFormatReaderSource>(reader.release(),true);
    referenceSource->setLooping(false);
    reference.setSource(referenceSource.get(),32768,&referenceThread,sr);
    referenceName=file.getFileName();referencePath=file.getFullPathName();
    loopStart.store(0);loopEnd.store(reference.getLengthInSeconds());
    referenceAnalysed.store(false);referenceProgress.store(0);referenceTempo.store(0);
    for(auto& value:referenceWave)value.store(0);
    for(auto& value:referenceSpectrum)value.store(-100);
    std::unique_ptr<juce::AudioFormatReader> scan(formats.createReaderFor(file));
    scanStop.store(false);
    scanThread=std::thread([this,reader=std::move(scan),sr]() mutable {
        if(!reader)return;
        baak::Meter meter;meter.prepare(sr);baak::ReconstructedPeak tp;
        juce::AudioBuffer<float> scratch(2,4096);
        juce::dsp::FFT fft(11);std::array<float,4096> fftBlock{};std::array<double,1024> spectralEnergy{};
        std::vector<double> onsets;double onsetEnergy=0,previousEnergy=0,totalEnergy=0;
        int onsetSamples=0,hop=std::max(1,(int)(sr*.01)),fftCount=0;float maximum=0;
        auto total=reader->lengthInSamples;
        for(juce::int64 position=0;position<total&&!scanStop.load();position+=4096){
            int n=(int)std::min<juce::int64>(4096,total-position);scratch.clear();reader->read(&scratch,0,n,position,true,true);
            for(int j=0;j<n;++j){
                float l=sane(scratch.getSample(0,j)),r=sane(scratch.getSample(1,j));meter.push(l,r);
                auto peak=tp.tick(l,r);maximum=std::max({maximum,peak[0],peak[1]});
                int bin=juce::jlimit(0,511,(int)((position+j)*512/std::max<juce::int64>(1,total)));
                referenceWave[bin].store(std::max(referenceWave[bin].load(),std::max(std::abs(l),std::abs(r))));
                double energy=(l*l+r*r)*.5;totalEnergy+=energy;onsetEnergy+=energy;
                if(++onsetSamples==hop){double rms=std::sqrt(onsetEnergy/hop);if(onsets.size()<12000)onsets.push_back(std::max(0.,rms-previousEnergy));previousEnergy=rms;onsetEnergy=0;onsetSamples=0;}
            }
            if(n>=2048){
                fftBlock.fill(0);for(int j=0;j<2048;++j)fftBlock[j]=(scratch.getSample(0,j)+scratch.getSample(1,j))*.25f*(1-std::cos(2*pi*j/2047));
                fft.performFrequencyOnlyForwardTransform(fftBlock.data());for(int j=0;j<1024;++j)spectralEnergy[j]+=fftBlock[j]*fftBlock[j]/(512.*512.);++fftCount;
            }
            referenceProgress.store((float)(position+n)/(float)std::max<juce::int64>(1,total));
        }
        if(scanStop.load())return;
        double best=0;int bestLag=0;
        for(int lag=30;lag<=100;++lag){double dot=0,aa=0,bb=0;for(size_t i=lag;i<onsets.size();++i){dot+=onsets[i]*onsets[i-lag];aa+=onsets[i]*onsets[i];bb+=onsets[i-lag]*onsets[i-lag];}double score=dot/std::max(1.e-15,std::sqrt(aa*bb));if(score>best){best=score;bestLag=lag;}}
        if(best>.15&&bestLag>0)referenceTempo.store((float)(60*sr/(hop*bestLag)));
        for(int j=0;j<1024;++j)referenceSpectrum[j].store((float)(10*std::log10(std::max(1.e-10,spectralEnergy[j]/std::max(1,fftCount)))));
        referenceLufs.store(meter.integrated);referencePeak.store(db(maximum));referenceRms.store((float)(10*std::log10(std::max(1.e-12,totalEnergy/std::max<juce::int64>(1,total)))));
        referenceAnalysed.store(true);referenceProgress.store(1);
    });
    return true;
}
void BaakProcessor::playReference(bool play) {
    if(play && referenceSource) { reference.start(); referencePlaying.store(true); }
    else {referencePlaying.store(false);reference.stop();}
}
void BaakProcessor::readAnalysis(std::vector<float>& in,std::vector<float>& out,std::vector<float>& side,std::vector<float>* inputSide) {
    int a,n,b,m;analysisFifo.prepareToRead(analysisFifo.getNumReady(),a,n,b,m);
    for(int k=0;k<n+m;++k) {int i=k<n?a+k:b+k-n;in.push_back(analysisInput[i]);out.push_back(analysisOutput[i]);side.push_back(analysisSide[i]);if(inputSide)inputSide->push_back(analysisInputSide[i]);}
    analysisFifo.finishedRead(n+m);
}

void BaakProcessor::handleAsyncUpdate(){
    juce::String path;double start,end;
    {juce::ScopedLock lock(restoreLock);path=pendingReference;start=pendingStart;end=pendingEnd;pendingReference.clear();}
    if(path.isNotEmpty()&&juce::File(path).existsAsFile()&&loadReference(juce::File(path))){
        double length=reference.getLengthInSeconds();loopStart.store(juce::jlimit(0.,std::max(0.,length-.05),start));loopEnd.store(juce::jlimit(loopStart.load()+.05,std::max(.05,length),end>0?end:length));
    }
}
