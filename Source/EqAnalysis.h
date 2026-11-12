#pragma once
#include "DspCore.h"
#include <juce_dsp/juce_dsp.h>
#include <complex>

namespace baak {
// Message-thread analysis of latency-aligned audio. Sum mid/side powers so
// wide or opposite-polarity stereo material cannot disappear from the graph.
class EqAnalysis {
public:
    static constexpr int order = 13, size = 1 << order, hop = size / 4, points = 300;
    void reset(double rate) {
        sampleRate = std::max(8000., rate);
        for (auto& channel : ring) channel.fill(0);
        inputPower.fill(0); outputPower.fill(0); change.fill(0); valid.fill(false);
        position = filled = sinceFrame = 0; frames = 0;
    }
    void push(float inMid, float inSide, float outMid, float outSide, float releaseMs) {
        const float samples[] = {inMid, inSide, outMid, outSide};
        for (int c = 0; c < 4; ++c) ring[c][position] = sane(samples[c]);
        position = (position + 1) % size;
        filled = std::min(size, filled + 1);
        if (++sinceFrame >= hop && filled == size) {
            sinceFrame = 0;
            analyse(releaseMs);
        }
    }
    float frequency(int i) const { return 20.f * std::pow((float)(topFrequency() / 20.), i / float(points - 1)); }
    double topFrequency() const { return std::min(20000., sampleRate * .45); }
    int index(float hz) const {
        return std::clamp((int)std::round(std::log(std::max(20.f, hz) / 20.f) /
                     std::log(topFrequency() / 20.) * (points - 1)), 0, points - 1);
    }
    float at(float hz) const { return change[index(hz)]; }
    bool hasSignal() const { return std::any_of(valid.begin(), valid.end(), [](bool b) { return b; }); }
    std::array<float, points> change{};
    std::array<bool, points> valid{};
    uint64_t frames = 0;
private:
    juce::dsp::FFT fft{order};
    std::array<std::array<float, size>, 4> ring{};
    std::array<float, size * 2> scratch{};
    std::array<std::array<float, size / 2>, 2> power{};
    std::array<float, points> inputPower{}, outputPower{};
    double sampleRate = 48000;
    int position = 0, filled = 0, sinceFrame = 0;
    void analyse(float releaseMs) {
        for (auto& p : power) p.fill(0);
        for (int c = 0; c < 4; ++c) {
            scratch.fill(0);
            for (int n = 0; n < size; ++n)
                scratch[n] = ring[c][(position + n) % size] *
                    (.5f - .5f * std::cos(2.f * (float)pi * n / (size - 1)));
            fft.performFrequencyOnlyForwardTransform(scratch.data());
            for (int k = 1; k < size / 2; ++k) {
                float magnitude = scratch[k] / (size * .25f);
                power[c / 2][k] += magnitude * magnitude;
            }
        }
        float peak = *std::max_element(power[0].begin(), power[0].end());
        const float alpha = 1.f - std::exp(-hop / (float)(sampleRate * std::max(60.f, releaseMs) * .001));
        for (int i = 0; i < points; ++i) {
            float bin = frequency(i) * size / (float)sampleRate;
            // Sixth-octave energy average, at least three bins at low frequencies.
            int first = std::clamp((int)std::floor(bin / std::pow(2.f, 1.f / 12)) - 1, 1, size / 2 - 1);
            int last = std::clamp((int)std::ceil(bin * std::pow(2.f, 1.f / 12)) + 1, first, size / 2 - 1);
            float in = 0, out = 0;
            for (int k = first; k <= last; ++k) { in += power[0][k]; out += power[1][k]; }
            in /= last - first + 1; out /= last - first + 1;
            inputPower[i] += (in - inputPower[i]) * alpha;
            outputPower[i] += (out - outputPower[i]) * alpha;
            valid[i] = peak > 1.e-10f && in > std::max(1.e-12f, peak * 1.e-6f);
            change[i] = valid[i] ? std::clamp(10.f * std::log10(std::max(1.e-16f, outputPower[i]) /
                                   std::max(1.e-16f, inputPower[i])), -48.f, 48.f) : 0.f;
        }
        ++frames;
    }
};

// The white line/optional phase describe the static filters only. The measured
// pink trace also captures dynamics, saturation, stereo changes and level trims.
inline std::complex<double> eqFilterResponse(const Params& p, double fs, double hz) {
    if (p[Bypass] > .5f) return 1.;
    std::array<Biquad, 6> filters;
    filters[0].configure(0, fs, std::max(10.f, p[Clean]));
    filters[1].configure(2, fs, 120, p[Low]);
    filters[2].configure(1, fs, p[MidFreq], p[Mid], .7);
    filters[3].configure(3, fs, 8000, p[High]);
    filters[4].configure(2, fs, 250, -p[ToneColor] * .025f);
    filters[5].configure(3, fs, 4000, p[ToneColor] * .025f);
    const auto z = std::exp(std::complex<double>(0, -2 * pi * hz / fs));
    std::complex<double> h = 1;
    for (int i = p[Clean] > .01f ? 0 : 1; i < 6; ++i) {
        const auto& f = filters[i];
        h *= (f.b0 + f.b1 * z + f.b2 * z * z) / (1. + f.a1 * z + f.a2 * z * z);
    }
    if (p[DC] > .5f) h *= (1. - z) / (1. - std::exp(-2 * pi * 5 / fs) * z);
    return h;
}
}
