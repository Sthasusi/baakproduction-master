#pragma once
#include "DspCore.h"

namespace baak {
// Windowed-sinc reconstruction, independent of the processing oversampler.
// Eight fractional phases, 64 taps, 31-sample group delay. No heap use in tick().
class ReconstructedPeak {
    std::array<std::array<float,64>,8> coefficients{};
    std::array<std::array<float,64>,2> history{};
    int position=0;
public:
    ReconstructedPeak() {
        for(int phase=0;phase<8;++phase) {
            double sum=0;
            for(int k=0;k<64;++k) {
                double x=k-31.-phase/8.;
                double sinc=std::abs(x)<1.e-10?1.:std::sin(pi*x)/(pi*x);
                double w=.42-.5*std::cos(2*pi*k/63.)+.08*std::cos(4*pi*k/63.);
                coefficients[phase][k]=static_cast<float>(sinc*w);sum+=sinc*w;
            }
            for(auto& v:coefficients[phase])v/=static_cast<float>(sum);
        }
    }
    void reset(){history={};position=0;}
    std::array<float,2> tick(float l,float r) {
        history[0][position]=l;history[1][position]=r;
        std::array<float,2> result{std::abs(l),std::abs(r)};
        for(int c=0;c<2;++c)for(int p=0;p<8;++p){
            float y=0;for(int k=0;k<64;++k)y+=history[c][(position-k+64)%64]*coefficients[p][k];
            result[c]=std::max(result[c],std::abs(y));
        }
        position=(position+1)%64;return result;
    }
};
class PeakGuard {
    ReconstructedPeak detector;
    std::array<std::array<float,2>,129> delay{};
    std::array<float,129> peaks{};
    int position=0;
    float envelope=1, release=.0002f;
public:
    static constexpr int latency=128;
    void prepare(double fs){detector.reset();delay={};peaks={};position=0;envelope=1;release=1-std::exp(-1.f/static_cast<float>(fs*.15));}
    void process(float& l,float& r,float ceiling,bool enabled){
        auto peak=detector.tick(l,r);
        peaks[position]=std::max(peak[0],peak[1]);delay[position]={l,r};
        float maximum=*std::max_element(peaks.begin(),peaks.end());
        // Reserve 0.6 dB for finite interpolation and gain modulation.
        float desired=enabled?std::min(1.f,ceiling*gain(-.6f)/std::max(maximum,1.e-9f)):1.f;
        envelope=desired<envelope?desired:envelope+(desired-envelope)*release;
        position=(position+1)%129;
        l=delay[position][0]*envelope;r=delay[position][1]*envelope;
    }
};
}
