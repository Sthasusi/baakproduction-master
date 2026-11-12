#include "PeakGuard.h"
#include <cassert>
#include <iostream>
#include <random>
#undef assert
#define assert(x) do{if(!(x)){std::cerr<<"FAIL "<<#x<<" line "<<__LINE__<<"\n";return 1;}}while(0)
// Independent 16-phase / 96-tap reconstruction, longer than the production detector.
double reconstructed(const std::vector<float>& x){
    double maximum=0;
    for(int phase=0;phase<16;++phase){
        std::array<double,96> coefficients{};double sum=0;
        for(int k=0;k<96;++k){double t=k-47.-phase/16.;double sinc=std::abs(t)<1e-10?1:std::sin(baak::pi*t)/(baak::pi*t);double w=.42-.5*std::cos(2*baak::pi*k/95)+.08*std::cos(4*baak::pi*k/95);coefficients[k]=sinc*w;sum+=coefficients[k];}
        for(auto& c:coefficients)c/=sum;
        for(size_t n=96;n<x.size();++n){double value=0;for(size_t k=0;k<96;++k)value+=x[n-k]*coefficients[k];maximum=std::max(maximum,std::abs(value));}
    }return maximum;
}
int main(){
    std::mt19937 random(42);std::uniform_real_distribution<float> noise(-1,1);double worst=-100;
    for(double fs:{44100.,48000.,96000.,192000.})for(int signal=0;signal<8;++signal){
        baak::PeakGuard guard;guard.prepare(fs);std::vector<float> output(8192+256);
        for(size_t i=0;i<output.size();++i){float x=0;if(i<8192){
            if(signal==0)x=.99f*std::sin(2*baak::pi*.25*i+baak::pi/4);
            else if(signal==1)x=noise(random);
            else if(signal==2)x=std::sin(2*baak::pi*.41*i);
            else if(signal==3)x=(i%400<200?1.f:-1.f);
            else if(signal==4)x=i%97==0?1.f:0.f;
            else if(signal==5)x=.55f*std::sin(2*baak::pi*.04*i)+.4f*std::sin(2*baak::pi*.37*i);
            else if(signal==6)x=(i%1024<512?.95f:.005f)*std::sin(2*baak::pi*.23*i);
            else x=std::sin(2*baak::pi*(.01*i+.000025*i*i));
        }float r=x;guard.process(x,r,baak::gain(-1),true);output[i]=x;assert(std::isfinite(x));}
        double peak=reconstructed(output),db=20*std::log10(std::max(1e-10,peak));worst=std::max(worst,db);
        if(db>-.99){std::cerr<<"rate "<<fs<<" signal "<<signal<<" dBTP "<<db<<"\n";return 1;}
    }
    baak::PeakGuard guard;guard.prepare(48000);for(int n=0;n<200;++n){float l=n==0?.5f:0,r=l;guard.process(l,r,1,false);assert(std::abs(l-(n==128?.5f:0.f))<1e-6);}
    std::cout<<"PASS: 32 reconstructed-peak cases at 4 rates; worst "<<worst<<" dBTP at -1 dB ceiling; bypass delay 128 samples.\n";
}
