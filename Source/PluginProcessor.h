#pragma once
#include "DspCore.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
class BaakProcessor : public juce::AudioProcessor {
  public:
    BaakProcessor();
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    void processBlockBypassed(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    bool isBusesLayoutSupported(const BusesLayout &) const override;
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "BaakProduction Master"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Custom"; }
    void changeProgramName(int, const juce::String &) override {}
    void getStateInformation(juce::MemoryBlock &) override;
    void setStateInformation(const void *, int) override;
    juce::AudioProcessorValueTreeState state;
    std::atomic<float> inShort{-90}, outShort{-90}, outIntegrated{-90}, outPeak{-90}, outRms{-90},
        correlation{1}, gainReduction{0}, elapsed{0};
    std::atomic<bool> resetMeters{false};
    void setValue(int, float);
    baak::Params current() const;
    void snapshot(int);
    void recall(int);

  private:
    void run(juce::AudioBuffer<float> &, bool);
    baak::Engine engine;
    baak::Meter inputMeter, outputMeter;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::AudioBuffer<float> dryRing, dryScratch;
    int dryPos = 0, latency = 0;
    float bypassMix = 0;
    std::array<juce::ValueTree, 3> snapshots;
    std::array<std::atomic<float> *, baak::Count> values{};
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaakProcessor)
};
