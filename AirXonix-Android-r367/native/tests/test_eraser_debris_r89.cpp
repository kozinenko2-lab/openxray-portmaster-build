#include "game/special_objects.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

struct SpecialObjectsTestProbe {
    static void emit(SpecialObjects& s,float x,float z,LegacyRandom& r){ s.emitEraserDebris(x,z,r); }
    static void step(SpecialObjects& s,int dt){ s.updateEraserDebris(dt); }
    static int head(const SpecialObjects& s){ return s.eraserDebrisHead_; }
};

static bool nearf(float a,float b,float eps=1e-10f){ return std::fabs(a-b)<=eps; }

int main(){
    SpecialObjects s;
    LegacyRandom rng(1u), expected(1u);
    SpecialObjectsTestProbe::emit(s,.5f,.55f,rng);
    assert(SpecialObjectsTestProbe::head(s)==16);
    int active=0;
    for(const auto& p:s.eraserDebris()) if(p.active) ++active;
    assert(active==16);

    const int ry=int(expected.mask(63))-32;
    const int rx=int(expected.mask(63))-32;
    const int rz=int(expected.mask(63))-32;
    const auto& p=s.eraserDebris()[16];
    assert(p.active);
    assert(nearf(p.x,.5f)); assert(nearf(p.y,.008f)); assert(nearf(p.z,.55f));
    assert(nearf(p.vy,float(ry)*.000002f+.0002f,1e-9f));
    assert(nearf(p.vx,float(rx)*.0000002f,1e-10f));
    assert(nearf(p.vz,float(rz)*.0000002f,1e-10f));

    const float x0=p.x,y0=p.y,z0=p.z,vx0=p.vx,vy0=p.vy,vz0=p.vz;
    SpecialObjectsTestProbe::step(s,10);
    const auto& zero=s.eraserDebris()[0];
    // Native 0x4152CF advances all 128 records, including zeroed slots.
    assert(nearf(zero.x,0.f)); assert(nearf(zero.y,0.f)); assert(nearf(zero.z,0.f));
    assert(nearf(zero.vy,-.00000035f*10.f,1e-10f));

    const auto& q=s.eraserDebris()[16];
    assert(nearf(q.x,x0+vx0*10.f,1e-8f));
    assert(nearf(q.y,y0+vy0*10.f,1e-8f));
    assert(nearf(q.z,z0+vz0*10.f,1e-8f));
    assert(nearf(q.vy,vy0-.00000035f*10.f,1e-9f));

    // Ring head advances in exact 16-record chunks and wraps modulo 128.
    for(int i=0;i<7;++i) SpecialObjectsTestProbe::emit(s,.5f,.55f,rng);
    assert(SpecialObjectsTestProbe::head(s)==0);
    std::cout << "eraser debris r89 ok\n";
}
