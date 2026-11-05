#ifdef NDEBUG
#undef NDEBUG
#endif
#include "DspCore.h"
#include <cassert>
#include <iostream>
#include <limits>
#include <random>
using namespace baak;
int main() {
    int checks = 0;
    for (double fs : {44100., 48000., 96000., 192000.}) {
        Engine e;
        auto p = defaults();
        e.set(p);
        e.prepare(fs);
        std::vector<float> input(12000), output(12000);
        for (int i = 0; i < 12000; ++i) {
            float l = .1f * std::sin(2 * pi * 1000 * i / fs), r = l;
            input[i] = l;
            e.process(l, r);
            output[i] = l;
            assert(std::isfinite(l));
            assert(l == r);
        }
        for (int i = e.latency() + 64; i < 12000; ++i)
            assert(std::abs(output[i] - input[i - e.latency()]) < 1.e-5);
        ++checks;
        e.prepare(fs);
        for (int i = 0; i < 12000; ++i) {
            float l = 0, r = 0;
            e.process(l, r);
            assert(l == 0 && r == 0);
        }
        ++checks;
        p[Loud] = 18;
        p[Ceiling] = -1;
        e.set(p);
        e.prepare(fs);
        std::mt19937 gen(42);
        std::uniform_real_distribution<float> d(-2, 2);
        for (int i = 0; i < 30000; ++i) {
            float l = d(gen), r = d(gen);
            e.process(l, r);
            assert(std::abs(l) <= gain(-1) + 1.e-6);
            assert(std::abs(r) <= gain(-1) + 1.e-6);
        }
        ++checks;
        p = defaults();
        p[Width] = 0;
        e.set(p);
        e.prepare(fs);
        for (int i = 0; i < 12000; ++i) {
            float l = d(gen) * .1f, r = d(gen) * .1f;
            e.process(l, r);
            assert(std::abs(l - r) < 1.e-6);
        }
        ++checks;
        p = defaults();
        p[Bypass] = 1;
        p[Loud] = 18;
        e.set(p);
        e.prepare(fs);
        for (int i = 0; i < 12000; ++i) {
            float l = input[i], r = l;
            e.process(l, r);
            if (i >= e.latency())
                assert(std::abs(l - input[i - e.latency()]) < 1.e-6);
        }
        ++checks;
        p = defaults();
        for (int k = 0; k < Count; ++k)
            p[k] = specs[k].max;
        p[Bypass] = 0;
        e.set(p);
        e.prepare(fs);
        for (int i = 0; i < 30000; ++i) {
            float l = d(gen), r = d(gen);
            e.process(l, r);
            assert(std::isfinite(l) && std::isfinite(r));
            assert(std::abs(l) <= 1 && std::abs(r) <= 1);
        }
        ++checks;
        p[Input] = std::numeric_limits<float>::quiet_NaN();
        e.set(p);
        float l = std::numeric_limits<float>::infinity(), r = std::numeric_limits<float>::quiet_NaN();
        e.process(l, r);
        assert(std::isfinite(l) && std::isfinite(r));
        ++checks;
    }
    Meter m;
    m.prepare(48000);
    for (int i = 0; i < 48000 * 6; ++i) {
        float x = .1f * std::sin(2 * pi * 1000 * i / 48000);
        m.push(x, x);
    }
    // Stereo 1 kHz sine at -20 dBFS peak is approximately -20 LUFS.
    assert(m.shortLufs > -20.3 && m.shortLufs < -19.7);
    assert(std::abs(m.integrated - m.shortLufs) < .1);
    assert(m.correlation > .999);
    ++checks;
    m.prepare(48000);
    for (int i = 0; i < 48000; ++i)
        m.push(0, 0);
    assert(m.integrated == -90);
    ++checks;
    std::cout << "PASS: " << checks
              << " DSP checks: neutral reconstruction, silence, sample ceiling, mono, bypass latency, "
                 "extreme settings, non-finite input, K-weighted sine, silence gate.\n";
}
