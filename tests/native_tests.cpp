#include "PluginProcessor.h"
#include <cassert>
#include <iostream>
#include <limits>
#undef assert
#define assert(expr) do { if (!(expr)) { std::cerr << "Check failed: " << #expr << " at " << __LINE__ << std::endl; std::abort(); } } while (0)
int main(int argc, char **argv) {
    std::cerr << "init GUI\n";
    juce::ScopedJuceInitialiser_GUI gui;
    std::cerr << "init processor\n";
    BaakProcessor processor;
    std::cerr << "prepare processor\n";
    processor.setPlayConfigDetails(2, 2, 48000, 256);
    processor.prepareToPlay(48000, 256);
    processor.setValue(baak::Loud, 9);
    processor.setValue(baak::Warmth, 12);
    processor.setValue(baak::Glue, 15);
    juce::AudioBuffer<float> block(2, 256);
    juce::MidiBuffer midi;
    std::cerr << "process audio\n";
    for (int k = 0; k < 1500; ++k) {
        for (int n = 0; n < 256; ++n) {
            float t = (k * 256 + n) / 48000.f;
            float s = .49f * std::sin(2 * baak::pi * 110 * t) + .13f * std::sin(2 * baak::pi * 880 * t);
            block.setSample(0, n, s);
            block.setSample(1, n, s * .95f);
        }
        processor.processBlock(block, midi);
        for (int c = 0; c < 2; ++c)
            for (int n = 0; n < 256; ++n)
                assert(std::isfinite(block.getSample(c, n)) &&
                       std::abs(block.getSample(c, n)) <= baak::gain(-1) + 1.e-5);
    }
    assert(processor.elapsed.load() > 7.9);
    assert(processor.outShort.load() > -50);
    assert(processor.getLatencySamples() > 0);
    juce::MemoryBlock state;
    processor.getStateInformation(state);
    processor.setValue(baak::Loud, 0);
    processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    assert(std::abs(processor.current()[baak::Loud] - 9) < .01);
    processor.snapshot(0);
    processor.setValue(baak::Low, 3);
    processor.recall(0);
    assert(std::abs(processor.current()[baak::Low]) < .01);
    std::cerr << "construct editor\n";
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    juce::Thread::sleep(50);
    juce::Timer::callPendingTimersSynchronously();
    if (argc > 1) {
        auto image = editor->createComponentSnapshot(editor->getLocalBounds(), true, 1.5f);
        juce::File file(juce::String::fromUTF8(argv[1]));
        auto stream = file.createOutputStream();
        if (!stream)
            return 2;
        juce::PNGImageFormat png;
        if (!png.writeImageToStream(image, *stream))
            return 3;
    }
    auto click = [&](const juce::String& label) {
        for (auto* child : editor->getChildren()) {
            if (auto* button = dynamic_cast<juce::TextButton*>(child)) {
                if (button->getButtonText() == label) { button->onClick(); return true; }
            }
        }
        return false;
    };
    assert(click("Advanced"));
    assert(click("Main controls"));
    assert(click("Dark"));
    if(argc>1) {auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1.5f);juce::File file(juce::String::fromUTF8(argv[1]));auto stream=file.getSiblingFile("Interface-Dark.png").createOutputStream();assert(stream);juce::PNGImageFormat png;assert(png.writeImageToStream(image,*stream));}
    assert(click("Light"));
    assert(!click("Apply suggestion"));
    processor.processBlock(block,midi);
    std::vector<float> ai,ao,as; processor.readAnalysis(ai,ao,as);
    assert(!ai.empty() && ai.size()==ao.size() && ao.size()==as.size());
    processor.setValue(baak::Unity,1);
    for(int k=0;k<4000;++k) {
        for(int n=0;n<256;++n) { float v=.1f*std::sin(2*baak::pi*1000*(k*256+n)/48000.);block.setSample(0,n,v);block.setSample(1,n,v); }
        processor.processBlock(block,midi);
    }
    assert(std::abs(processor.inShort.load()-processor.outShort.load())<1.5f);
    processor.setValue(baak::Unity,0);
    processor.setValue(baak::Bypass, 1);
    processor.prepareToPlay(48000, 256);
    juce::AudioBuffer<float> impulse(2, 2048);
    impulse.clear();
    impulse.setSample(0, 0, .5f);
    impulse.setSample(1, 0, .5f);
    processor.processBlock(impulse, midi);
    for (int i = 0; i < 2048; ++i)
        for (int c = 0; c < 2; ++c)
            assert(std::abs(impulse.getSample(c, i) - (i == processor.getLatencySamples() ? .5f : 0.f)) <
                   1.e-6f);
    processor.setValue(baak::Bypass, 0);
    processor.prepareToPlay(48000, 256);
    impulse.clear();
    impulse.setSample(0, 0, std::numeric_limits<float>::quiet_NaN());
    impulse.setSample(1, 0, std::numeric_limits<float>::infinity());
    processor.processBlock(impulse, midi);
    for (int i = 0; i < 2048; ++i)
        for (int c = 0; c < 2; ++c)
            assert(std::isfinite(impulse.getSample(c, i)));
    // Reference must actually play decoded audio and survive switching back.
    auto file=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("badi-reference-test",".wav");
    {
        auto stream=file.createOutputStream();assert(stream);
        const int frames=48000, bytes=frames*4;
        stream->write("RIFF",4);stream->writeInt(36+bytes);stream->write("WAVEfmt ",8);stream->writeInt(16);stream->writeShort(1);stream->writeShort(2);stream->writeInt(48000);stream->writeInt(192000);stream->writeShort(4);stream->writeShort(16);stream->write("data",4);stream->writeInt(bytes);
        for(int i=0;i<frames;++i){short value=(short)(8000*std::sin(2*baak::pi*440*i/48000));stream->writeShort(value);stream->writeShort(value);}
    }
    assert(processor.loadReference(file));assert(processor.reference.getLengthInSeconds()>.99);
    processor.playReference(true);juce::Thread::sleep(100);
    double energy=0;
    for(int k=0;k<100;++k){block.clear();processor.processBlock(block,midi);energy+=block.getRMSLevel(0,0,256);}
    assert(energy>1);processor.playReference(false);
    file.deleteFile();
    std::cout << "PASS: native processor sample ceiling, finite output, telemetry, latency reporting, state "
                 "restore, snapshots, editor construction, aligned dry bypass, oversized blocks and "
                 "non-finite input.\n";
}
