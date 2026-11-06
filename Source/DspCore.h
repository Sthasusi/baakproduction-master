#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace baak {
constexpr double pi = 3.14159265358979323846;
inline float db(float x) {
    return 20.f * std::log10(std::max(std::abs(x), 1.e-9f));
}
inline float gain(float x) {
    return std::pow(10.f, x / 20.f);
}
inline float sane(float x) {
    return std::isfinite(x) ? std::clamp(x, -64.f, 64.f) : 0.f;
}
enum Param {
    Input,
    Loud,
    Low,
    Mid,
    High,
    Width,
    Warmth,
    Calm,
    Glue,
    Ceiling,
    Clean,
    MidFreq,
    Threshold,
    Ratio,
    Attack,
    Release,
    LowComp,
    MidComp,
    HighComp,
    Body,
    BassMono,
    Colour,
    Focus,
    LimitRelease,
    Output,
    Bypass,
    Tape, TapeTone, Cohesion, ToneColor, Character, Silk, DeMud, DC, Unity,
    Count
};
struct Spec {
    const char *id;
    const char *name;
    const char *unit;
    float min, max, initial;
    const char *help;
};
inline constexpr std::array<Spec, Count> specs{
    {{"input", "Input", " dB", -24, 24, 0, "Level entering the tone and dynamics stages."},
     {"loud", "Loudness", " dB", 0, 18, 0,
      "Drive the final limiter. Watch gain reduction and listen for lost punch."},
     {"low", "Bass", " dB", -6, 6, 0, "Low shelf at 120 Hz: weight versus leanness."},
     {"mid", "Focus", " dB", -6, 6, 0, "Broad midrange equalizer. Set its frequency in Pro mode."},
     {"high", "Air", " dB", -6, 6, 0, "High shelf at 8 kHz: brightness versus softness."},
     {"width", "Width", " %", 0, 150, 100, "Mid/side width. 100% is unchanged; 0% folds down to mono."},
     {"warmth", "Warmth", " %", 0, 100, 0,
      "Blend in saturation. Pro mode selects its character and frequency region."},
     {"calm", "Soften", " %", 0, 100, 0,
      "Five-band adaptive harshness reduction from 1.5 to 11 kHz; linked stereo detection."},
     {"glue", "Glue", " %", 0, 100, 0, "Blend stereo-linked bus compression with the original signal."},
     {"ceiling", "Ceiling", " dBFS", -6, -0.1f, -1,
      "Sample-peak ceiling. This alpha does not guarantee true-peak compliance."},
     {"clean", "Low cut", " Hz", 0, 120, 0, "0 bypasses the high-pass filter; other values remove sub-bass."},
     {"midfreq", "Focus frequency", " Hz", 200, 5000, 1200, "Center frequency of the broad midrange bell."},
     {"threshold", "Glue threshold", " dB", -36, 0, -18, "Compression begins above this detector level."},
     {"ratio", "Glue ratio", ":1", 1, 6, 2, "How strongly levels above threshold are compressed."},
     {"attack", "Glue attack", " ms", 1, 100, 30, "Slower settings preserve more initial transient."},
     {"release", "Glue release", " ms", 30, 800, 180, "Recovery time after bus compression."},
     {"lowcomp", "Bass control", " %", 0, 100, 0, "Gentle linked low-band compression below 250 Hz."},
     {"midcomp", "Mid control", " %", 0, 100, 0, "Gentle linked mid-band compression from 250 Hz to 4 kHz."},
     {"highcomp", "Top control", " %", 0, 100, 0, "Gentle linked high-band compression above 4 kHz."},
     {"body", "Density", " %", 0, 100, 0,
      "Up to 3 dB of gated upward gain on quieter material; may lift noise."},
     {"bassmono", "Bass mono", " %", 0, 100, 0, "Remove side information below approximately 120 Hz."},
     {"colour", "Character", "", 0, 3, 0, "0 Soft, 1 Rounded, 2 Edge, 3 Asymmetric. Original waveshapers."},
     {"focus", "Colour region", "", 0, 3, 0, "0 Full, 1 Bass, 2 Mid, 3 Top. Targets saturation."},
     {"limitrelease", "Limiter release", " ms", 30, 500, 120,
      "Recovery time of the linked lookahead sample-peak limiter."},
     {"output", "Output trim", " dB", -18, 0, 0,
      "Attenuation after limiting. Does not increase the ceiling."},
     {"bypass", "Bypass", "", 0, 1, 0,
      "Latency-aligned dry path; the limiter does not protect bypassed audio."},
     {"tape", "Tape", " %", 0, 100, 0, "Blend gently driven tape-style saturation."},
     {"tapetone", "Tape tone", " %", -100, 100, 0, "Warm darkening to tighter bass response."},
     {"cohesion", "Cohesion", " %", 0, 100, 0, "Linked envelope-dependent saturation, not a model of another product."},
     {"tonecolor", "Tone color", " %", -100, 100, 0, "Broad warm-to-bright tonal tilt."},
     {"character", "Character", " %", 0, 100, 0, "Blend soft pre-clipping into the limiter input."},
     {"silk", "Sheen", " %", 0, 100, 0, "High-frequency harmonic enhancement."},
     {"demud", "De-mud", " %", 0, 100, 0, "Dynamic low-mid attenuation around 300 Hz."},
     {"dc", "DC filter", "", 0, 1, 0, "Remove DC offset before the limiter."},
     {"unity", "Unity", "", 0, 1, 0, "Attenuate the processed signal to match input loudness; never adds gain."}}};
using Params = std::array<float, Count>;
inline Params defaults() {
    Params p{};
    for (int i = 0; i < Count; ++i)
        p[i] = specs[i].initial;
    return p;
}
struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    float tick(float x) {
        double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return static_cast<float>(y);
    }
    void reset() { z1 = z2 = 0; }
    void configure(int type, double fs, double hz, double dB = 0, double q = .70710678) {
        const double A = std::pow(10., dB / 40.), w = 2 * pi * std::clamp(hz, 5., fs * .45) / fs;
        const double c = std::cos(w), s = std::sin(w), alpha = s / (2 * q), t = 2 * std::sqrt(A) * alpha;
        double den = 1;
        if (type == 0) {
            b0 = (1 + c) / 2;
            b1 = -(1 + c);
            b2 = b0;
            den = 1 + alpha;
            a1 = -2 * c;
            a2 = 1 - alpha;
        }
        if (type == 1) {
            b0 = 1 + alpha * A;
            b1 = -2 * c;
            b2 = 1 - alpha * A;
            den = 1 + alpha / A;
            a1 = -2 * c;
            a2 = 1 - alpha / A;
        }
        if (type == 2) {
            b0 = A * ((A + 1) - (A - 1) * c + t);
            b1 = 2 * A * ((A - 1) - (A + 1) * c);
            b2 = A * ((A + 1) - (A - 1) * c - t);
            den = (A + 1) + (A - 1) * c + t;
            a1 = -2 * ((A - 1) + (A + 1) * c);
            a2 = (A + 1) + (A - 1) * c - t;
        }
        if (type == 3) {
            b0 = A * ((A + 1) + (A - 1) * c + t);
            b1 = -2 * A * ((A - 1) + (A + 1) * c);
            b2 = A * ((A + 1) + (A - 1) * c - t);
            den = (A + 1) - (A - 1) * c + t;
            a1 = 2 * ((A - 1) - (A + 1) * c);
            a2 = (A + 1) - (A - 1) * c - t;
        }
        if(type==4) { b0=alpha; b1=0; b2=-alpha; den=1+alpha; a1=-2*c; a2=1-alpha; }
        b0 /= den;
        b1 /= den;
        b2 /= den;
        a1 /= den;
        a2 /= den;
    }
};
struct Split {
    float low = 0, belowHigh = 0;
    std::array<float, 3> tick(float x, float a, float b) {
        low += (x - low) * a;
        belowHigh += (x - belowHigh) * b;
        return {low, belowHigh - low, x - belowHigh}; // Exact reconstruction when unmodified.
    }
};
class Engine {
    double fs = 48000;
    Params p = defaults(), target = p;
    std::array<std::array<Biquad, 4>, 2> eq{};
    std::array<Split, 2> splits{};
    std::array<float,2> tapeLow{}, sheenLow{}, dcIn{}, dcOut{};
    std::array<Biquad,2> mudBand{}, tiltLow{}, tiltHigh{};
    float mudEnv = 0;
    std::array<std::array<Biquad,5>,2> harshFilters{};
    std::array<float,5> harshEnv{}; float broadEnv=0;
    std::array<float, 3> env{};
    float busEnv = 0, sideBass = 0, limitGain = 1, bypassMix = 0;
    float smooth = .002f, aLow = .01f, aHigh = .4f, aBass = .01f;
    int delay = 240, pos = 0, update = 0;
    uint64_t tickIndex = 0;
    std::vector<std::array<float, 2>> audioDelay, dryDelay;
    std::vector<float> maxValues;
    std::vector<uint64_t> maxIndices;
    size_t head = 0, tail = 0, queueSize = 0;
    float curve(float x, int mode) const {
        if (mode == 1)
            return std::atan(x * 1.7f) / 1.7f;
        if (mode == 2) {
            x = std::clamp(x, -1.5f, 1.5f);
            return x - x * x * x / 6.75f;
        }
        if (mode == 3)
            return (std::tanh(x + .25f) - std::tanh(.25f)) / .94001485f;
        return std::tanh(x);
    }
    void filters() {
        for(int c=0;c<2;++c) {
            mudBand[c].configure(1,fs,300,-6*p[DeMud]*.01f*std::clamp((db(mudEnv)+35)/20,0.f,1.f),.7);
            tiltLow[c].configure(2,fs,250,-p[ToneColor]*.025f);
            tiltHigh[c].configure(3,fs,4000,p[ToneColor]*.025f);
        }
        for (auto &c : eq) {
            c[0].configure(0, fs, std::max(10.f, p[Clean]));
            c[1].configure(2, fs, 120, p[Low]);
            c[2].configure(1, fs, p[MidFreq], p[Mid], .7);
            c[3].configure(3, fs, 8000, p[High]);
        }
    }

  public:
    float reduction = 0;
    int latency() const { return delay; }
    void prepare(double sampleRate) {
        fs = std::clamp(sampleRate, 8000., 768000.);
        delay = static_cast<int>(std::round(fs * .005 / 2) * 2);
        audioDelay.assign(delay + 1, {});
        dryDelay.assign(delay + 1, {});
        maxValues.assign(delay + 2, 0);
        maxIndices.assign(delay + 2, 0);
        pos = update = 0;
        tickIndex = 0;
        head = tail = queueSize = 0;
        harshEnv={};broadEnv=0;
        const double centers[]={1500,2800,4500,7200,11000};
        for(auto& channel:harshFilters) for(int i=0;i<5;++i){channel[i].reset();channel[i].configure(4,fs,centers[i],0,2.5);}
        splits = {}; tapeLow={}; sheenLow={}; dcIn={}; dcOut={}; mudEnv=0;
        for(auto* group : {&mudBand,&tiltLow,&tiltHigh}) for(auto& f:*group) f.reset();
        env = {};
        busEnv = sideBass = 0;
        limitGain = 1;
        reduction = 0;
        p = target;
        bypassMix = p[Bypass];
        smooth = 1 - std::exp(-1.f / static_cast<float>(fs * .015));
        aLow = 1 - std::exp(-2 * pi * 250 / fs);
        aHigh = 1 - std::exp(-2 * pi * 4000 / fs);
        aBass = 1 - std::exp(-2 * pi * 120 / fs);
        for (auto &c : eq) {
            for (auto &f : c)
                f.reset();
        }
        filters();
    }
    void set(const Params &values) {
        for (int i = 0; i < Count; ++i)
            target[i] = std::isfinite(values[i]) ? std::clamp(values[i], specs[i].min, specs[i].max)
                                                 : specs[i].initial;
    }
    void process(float &left, float &right) {
        for (int i = 0; i < Count; ++i)
            p[i] += (target[i] - p[i]) * smooth;
        if (++update == 32) {
            filters();
            update = 0;
        }
        const std::array<float, 2> raw{sane(left), sane(right)};
        std::array<float, 2> x = raw;
        const float inputGain = gain(p[Input]);
        for (int c = 0; c < 2; ++c) {
            x[c] *= inputGain;
            float hp = eq[c][0].tick(x[c]);
            if (p[Clean] > .01f)
                x[c] = hp;
            for (int f = 1; f < 4; ++f)
                x[c] = eq[c][f].tick(x[c]);
        }
        auto l = splits[0].tick(x[0], aLow, aHigh), r = splits[1].tick(x[1], aLow, aHigh);
        float attack = 1 - std::exp(-1.f / static_cast<float>(fs * .008)),
              release = 1 - std::exp(-1.f / static_cast<float>(fs * .12));
        const int colour = static_cast<int>(std::round(target[Colour])),
                  focus = static_cast<int>(std::round(target[Focus]));
        const float wet = p[Warmth] * .01f, drive = 1 + 3 * wet;
        for (int b = 0; b < 3; ++b) {
            float peak = std::max(std::abs(l[b]), std::abs(r[b]));
            env[b] += (peak - env[b]) * (peak > env[b] ? attack : release);
            float amount = p[LowComp + b] * .01f;
            float gr = std::max(0.f, db(env[b]) + 22.f) * .5f * amount;
            float g = gain(-gr);
            l[b] *= g;
            r[b] *= g;
            if (focus == b + 1) {
                l[b] += (curve(l[b] * drive, colour) / drive - l[b]) * wet;
                r[b] += (curve(r[b] * drive, colour) / drive - r[b]) * wet;
            }
        }
        x = {l[0] + l[1] + l[2], r[0] + r[1] + r[2]};
        if (focus == 0)
            for (auto &v : x)
                v += (curve(v * drive, colour) / drive - v) * wet;
        float peak = std::max(std::abs(x[0]), std::abs(x[1]));
        broadEnv += (peak-broadEnv)*(peak>broadEnv?attack:release);
        std::array<float,2> harshReduction{};
        for(int band=0;band<5;++band) {
            float bl=harshFilters[0][band].tick(x[0]), br=harshFilters[1][band].tick(x[1]);
            float bp=std::max(std::abs(bl),std::abs(br));
            harshEnv[band]+=(bp-harshEnv[band])*(bp>harshEnv[band]?attack:release);
            float prominence=std::clamp((harshEnv[band]/std::max(.0001f,broadEnv)-.23f)*1.3f,0.f,.65f);
            float amount=prominence*p[Calm]*.01f*std::clamp((db(broadEnv)+65)/20,0.f,1.f);
            harshReduction[0]+=bl*amount;harshReduction[1]+=br*amount;
        }
        x[0]-=harshReduction[0];x[1]-=harshReduction[1];
        mudEnv += (peak-mudEnv)*(1-std::exp(-1.f/static_cast<float>(fs*(peak>mudEnv?.015:.18))));
        for(int c=0;c<2;++c) {
            float v=tiltHigh[c].tick(tiltLow[c].tick(mudBand[c].tick(x[c])));
            tapeLow[c] += (v-tapeLow[c])*(1-std::exp(-2*pi*2500/fs));
            float tc=p[TapeTone]*.01f, tw=p[Tape]*.01f;
            float tapeInput=v+(tc<0 ? (tapeLow[c]-v)*(-tc)*.35f : -tapeLow[c]*tc*.12f);
            v += (std::tanh(tapeInput*1.8f)/1.8f-v)*tw;
            sheenLow[c] += (v-sheenLow[c])*(1-std::exp(-2*pi*6000/fs));
            float hi=v-sheenLow[c]; v += (std::tanh(hi*3.f)/3.f-hi)*p[Silk]*.01f;
            float drive=1+2*p[Cohesion]*.01f*std::clamp(mudEnv*2,0.f,1.f);
            v += (std::tanh(v*drive)/drive-v)*p[Cohesion]*.01f;
            float hp=v-dcIn[c]+static_cast<float>(std::exp(-2*pi*5/fs))*dcOut[c];
            dcIn[c]=v; dcOut[c]=hp;
            x[c]=v+(hp-v)*p[DC];
        }
        peak = std::max(std::abs(x[0]), std::abs(x[1]));
        float ca = 1 - std::exp(-1.f / static_cast<float>(fs * p[Attack] * .001)),
              cr = 1 - std::exp(-1.f / static_cast<float>(fs * p[Release] * .001));
        busEnv += (peak - busEnv) * (peak > busEnv ? ca : cr);
        float over = db(busEnv) - p[Threshold], knee = 6.f;
        float compression =
            over <= -knee / 2
                ? 0
                : (over >= knee / 2 ? over : (over + knee / 2) * (over + knee / 2) / (2 * knee));
        float cg = gain(-compression * (1 - 1 / p[Ratio]));
        cg = 1 + (cg - 1) * p[Glue] * .01f;
        float bodyDb = std::clamp((-18 - db(busEnv)) * .15f, 0.f, 3.f) * p[Body] * .01f *
                       std::clamp((db(busEnv) + 65) / 15, 0.f, 1.f);
        cg *= gain(bodyDb);
        for (auto &v : x)
            v *= cg;
        float mid = (x[0] + x[1]) * .5f, side = (x[0] - x[1]) * .5f;
        sideBass += (side - sideBass) * aBass;
        side = (side - sideBass * p[BassMono] * .01f) * p[Width] * .01f;
        x = {mid + side, mid - side};
        float loud = gain(p[Loud]);
        for (auto &v : x)
            v *= loud;
        float ceiling = gain(p[Ceiling]);
        for(auto& v:x) v += (std::tanh(v/ceiling)*ceiling-v)*p[Character]*.01f;
        peak = std::max(std::abs(x[0]), std::abs(x[1]));
        const size_t cap = maxValues.size();
        while (queueSize && maxIndices[head] + static_cast<uint64_t>(delay) < tickIndex) {
            head = (head + 1) % cap;
            --queueSize;
        }
        while (queueSize) {
            size_t back = (tail + cap - 1) % cap;
            if (maxValues[back] > peak)
                break;
            tail = back;
            --queueSize;
        }
        maxValues[tail] = peak;
        maxIndices[tail] = tickIndex;
        tail = (tail + 1) % cap;
        ++queueSize;
        const float wanted = std::min(1.f, ceiling / std::max(maxValues[head], 1.e-9f));
        float lr = 1 - std::exp(-1.f / static_cast<float>(fs * p[LimitRelease] * .001));
        limitGain = wanted < limitGain ? wanted : limitGain + (wanted - limitGain) * lr;
        audioDelay[pos] = x;
        dryDelay[pos] = raw;
        int read = (pos + 1) % static_cast<int>(audioDelay.size());
        x = audioDelay[read];
        auto dry = dryDelay[read];
        pos = read;
        ++tickIndex;
        bypassMix += (target[Bypass] - bypassMix) * smooth;
        float outGain = gain(p[Output]);
        for (int c = 0; c < 2; ++c) {
            float wetOut = std::clamp(x[c] * limitGain, -ceiling, ceiling) * outGain;
            x[c] = wetOut + (dry[c] - wetOut) * bypassMix;
        }
        left = x[0];
        right = x[1];
        reduction = -db(limitGain);
    }
};

// K-weighted stereo meter. Histogram gating is quantized to 0.1 LU; not certified.
class Meter {
    std::array<Biquad, 2> pre{}, hp{};
    std::array<double, 30> chunks{};
    std::array<double, 1401> energies{};
    std::array<uint64_t, 1401> counts{};
    double sum = 0, rawEnergy = 0, cross = 0, ll = 0, rr = 0;
    int inChunk = 0, chunkSize = 4800, position = 0, filled = 0;
    float heldPeak = 0;
    uint64_t samples = 0;

  public:
    float shortLufs = -90, integrated = -90, rms = -90, correlation = 1, peak = -90;
    double seconds = 0;
    void prepare(double fs) {
        *this = Meter{};
        chunkSize = std::max(1, static_cast<int>(std::round(fs * .1)));
        for (int c = 0; c < 2; ++c) {
            double K = std::tan(pi * 1681.974450955533 / fs), Q = .7071752369554196,
                   Vh = std::pow(10., 3.999843853973347 / 20), Vb = std::pow(Vh, .4996667741545416),
                   a = 1 + K / Q + K * K;
            pre[c].b0 = (Vh + Vb * K / Q + K * K) / a;
            pre[c].b1 = 2 * (K * K - Vh) / a;
            pre[c].b2 = (Vh - Vb * K / Q + K * K) / a;
            pre[c].a1 = 2 * (K * K - 1) / a;
            pre[c].a2 = (1 - K / Q + K * K) / a;
            K = std::tan(pi * 38.13547087602444 / fs);
            Q = .5003270373238773;
            a = 1 + K / Q + K * K;
            hp[c].b0 = 1;
            hp[c].b1 = -2;
            hp[c].b2 = 1;
            hp[c].a1 = 2 * (K * K - 1) / a;
            hp[c].a2 = (1 - K / Q + K * K) / a;
        }
    }
    void push(float l, float r) {
        l = sane(l);
        r = sane(r);
        float kl = hp[0].tick(pre[0].tick(l)), kr = hp[1].tick(pre[1].tick(r));
        sum += kl * kl + kr * kr;
        rawEnergy += (l * l + r * r) * .5;
        ll += l * l;
        rr += r * r;
        cross += l * r;
        heldPeak = std::max({heldPeak, std::abs(l), std::abs(r)});
        ++samples;
        if (++inChunk < chunkSize)
            return;
        chunks[position] = sum / chunkSize;
        position = (position + 1) % 30;
        filled = std::min(30, filled + 1);
        double e = 0;
        for (double x : chunks)
            e += x;
        shortLufs = static_cast<float>(-.691 + 10 * std::log10(std::max(1.e-12, e / filled)));
        if (filled >= 4) {
            double four = 0;
            for (int i = 1; i <= 4; ++i)
                four += chunks[(position + 30 - i) % 30];
            four /= 4;
            float level = static_cast<float>(-.691 + 10 * std::log10(std::max(1.e-12, four)));
            if (level >= -70) {
                int b = std::clamp(static_cast<int>(std::floor((level + 100) * 10)), 0, 1400);
                energies[b] += four;
                ++counts[b];
            }
            double total = 0;
            uint64_t n = 0;
            for (int i = 300; i < 1401; ++i) {
                total += energies[i];
                n += counts[i];
            }
            double gate = n ? std::max(-70., -.691 + 10 * std::log10(total / n) - 10) : -70;
            total = 0;
            n = 0;
            for (int i = std::max(300, static_cast<int>(std::ceil((gate + 100) * 10))); i < 1401; ++i) {
                total += energies[i];
                n += counts[i];
            }
            integrated = n ? static_cast<float>(-.691 + 10 * std::log10(total / n)) : -90;
        }
        rms = static_cast<float>(10 * std::log10(std::max(1.e-12, rawEnergy / chunkSize)));
        peak = db(heldPeak);
        correlation = static_cast<float>(cross / std::max(1.e-15, std::sqrt(ll * rr)));
        seconds = samples / (chunkSize * 10.);
        sum = rawEnergy = ll = rr = cross = 0;
        heldPeak = 0;
        inChunk = 0;
    }
};
} // namespace baak
