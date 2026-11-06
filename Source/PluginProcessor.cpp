#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace baak;
juce::AudioProcessorValueTreeState::ParameterLayout BaakProcessor::layout() {
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    for (int i = 0; i < Count; ++i) {
        auto &s = specs[i];
        float step = (i == Colour || i == Focus || i == Bypass || i == DC || i == Unity) ? 1.f : .01f;
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
    analysisRate.store(sr); analysisFifo.reset(); unityGain=1; referenceMix=0;
    reference.prepareToPlay(std::max(1,block),sr);
    referenceScratch.setSize(2,std::max(1,block));
    auto p = current();
    p[Bypass] = 0;
    engine.set(p);
    engine.prepare(sr * 2);
    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    oversampling->initProcessing(static_cast<size_t>(std::max(1, block)));
    oversampling->reset();
    latency = engine.latency() / 2 + static_cast<int>(std::round(oversampling->getLatencyInSamples()));
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
            for (int c = 0; c < 2; ++c) {
                dryRing.setSample(c, dryPos, sane(dryScratch.getSample(c, j)));
                float dry = dryRing.getSample(c, read);
                float wet = std::clamp(buffer.getSample(c, offset + j), -ceiling, ceiling)*unityGain;
                buffer.setSample(c, offset + j, wet + (dry - wet) * bypassMix);
            }
            int start1,size1,start2,size2;
            analysisFifo.prepareToWrite(1,start1,size1,start2,size2);
            if(size1>0) {
                analysisInput[start1]=(dryScratch.getSample(0,j)+dryScratch.getSample(1,j))*.5f;
                analysisOutput[start1]=(buffer.getSample(0,offset+j)+buffer.getSample(1,offset+j))*.5f;
                analysisSide[start1]=(buffer.getSample(0,offset+j)-buffer.getSample(1,offset+j))*.5f;
                analysisFifo.finishedWrite(1);
            }
            dryPos = read;
            outputMeter.push(buffer.getSample(0, offset + j), buffer.getSample(1, offset + j));
        }
    }
    if(referencePlaying.load() || referenceMix>.0001f) {
        float target=referencePlaying.load()?1.f:0.f;
        float step=1-std::exp(-1.f/static_cast<float>(getSampleRate()*.02));
        for(int offset=0;offset<buffer.getNumSamples();offset+=maxBlock) {
            int n=std::min(maxBlock,buffer.getNumSamples()-offset);
            juce::AudioSourceChannelInfo info(&referenceScratch,0,n);
            reference.getNextAudioBlock(info);
            for(int j=0;j<n;++j) {
                referenceMix+=(target-referenceMix)*step;
                for(int c=0;c<2;++c) {
                    float original=buffer.getSample(c,offset+j);
                    float ref=sane(referenceScratch.getSample(c,j))*gain(referenceGain.load());
                    buffer.setSample(c,offset+j,original+(ref-original)*referenceMix);
                }
            }
        }
    }
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
    playReference(false); reference.setSource(nullptr);
    double sr=reader->sampleRate;
    referenceSource=std::make_unique<juce::AudioFormatReaderSource>(reader.release(),true);
    referenceSource->setLooping(true);
    reference.setSource(referenceSource.get(),32768,&referenceThread,sr);
    referenceName=file.getFileName();return true;
}
void BaakProcessor::playReference(bool play) {
    if(play && referenceSource) { reference.start(); referencePlaying.store(true); }
    else {referencePlaying.store(false);reference.stop();}
}
void BaakProcessor::readAnalysis(std::vector<float>& in,std::vector<float>& out,std::vector<float>& side) {
    int a,n,b,m;analysisFifo.prepareToRead(analysisFifo.getNumReady(),a,n,b,m);
    for(int k=0;k<n+m;++k) {int i=k<n?a+k:b+k-n;in.push_back(analysisInput[i]);out.push_back(analysisOutput[i]);side.push_back(analysisSide[i]);}
    analysisFifo.finishedRead(n+m);
}
