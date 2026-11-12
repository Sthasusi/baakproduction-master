#pragma once
#include "DspCore.h"
#include "PeakGuard.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
class BaakProcessor : public juce::AudioProcessor {
  public:
    BaakProcessor();
    ~BaakProcessor() override { reference.stop(); reference.setSource(nullptr); referenceThread.stopThread(2000); }
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    void processBlockBypassed(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    bool isBusesLayoutSupported(const BusesLayout &) const override;
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "BAD-I Master Pro"; }
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
    std::atomic<float> inIntegrated{-90}, outMomentary{-90}, truePeak{-90}, rmsL{-90}, rmsR{-90}, peakL{-90}, peakR{-90};
    std::atomic<int> activeQuality{1};
    std::atomic<bool> activeLowLatency{false};
    std::atomic<float> inShort{-90}, outShort{-90}, outIntegrated{-90}, outPeak{-90}, outRms{-90},
        correlation{1}, gainReduction{0}, elapsed{0};
    std::atomic<bool> resetMeters{false};
    void setValue(int, float);
    baak::Params current() const;
    void snapshot(int);
    void recall(int);
    bool loadReference(const juce::File&);
    void playReference(bool);
    juce::TimeSliceThread referenceThread{"BAD-I reference reader"};
    juce::AudioTransportSource reference;
    juce::String referenceName;
    std::atomic<bool> referencePlaying{false};
    std::atomic<float> referenceGain{-6};
    std::atomic<double> loopStart{0},loopEnd{0};
    std::atomic<bool> loopEnabled{true};
    std::array<float,512> referenceWave{};
    std::atomic<float> referenceLufs{-90},referencePeak{-90},referenceRms{-90};
    juce::String referencePath;
    std::atomic<double> analysisRate{48000};
    juce::AbstractFifo analysisFifo{16384};
    std::array<float,16384> analysisInput{}, analysisOutput{}, analysisSide{};
    void readAnalysis(std::vector<float>&, std::vector<float>&, std::vector<float>&);

  private:
    void run(juce::AudioBuffer<float> &, bool);
    juce::AudioFormatManager formats;
    std::unique_ptr<juce::AudioFormatReaderSource> referenceSource;
    juce::AudioBuffer<float> referenceScratch;
    float unityGain=1,referenceMix=0;
    baak::Engine engine;
    baak::PeakGuard peakGuard;
    baak::ReconstructedPeak peakMeter;
    std::array<float,2> heldTruePeaks{};
    std::array<baak::Biquad,2> monitorHighpass,monitorLowpass;
    float monitorMix=0;int previousMonitor=0;
    baak::Meter inputMeter, outputMeter, wetMeter;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::AudioBuffer<float> dryRing, dryScratch;
    int dryPos = 0, latency = 0;
    float bypassMix = 0;
    std::array<juce::ValueTree, 3> snapshots;
    std::array<std::atomic<float> *, baak::Count> values{};
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaakProcessor)
};
