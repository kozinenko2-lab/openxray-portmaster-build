#include "field.hpp"
#include <algorithm>
#include <utility>

uint8_t Field::at(int x,int y) const { return inside(x,y) ? cell_[y*W+x] : Safe; }
void Field::set(int x,int y,uint8_t v) { if (inside(x,y)) cell_[y*W+x]=v; }

void Field::build(const LevelRecord& r) {
    cell_.fill(Empty); markerTimer_.fill(0.0f); startedCaptureMarker_=0;
    for(int x=0;x<W;++x){ set(x,0,Safe); set(x,H-1,Safe); }
    for(int y=0;y<H;++y){ set(0,y,Safe); set(W-1,y,Safe); }
    // 0x418EFA..0x418FE2: five optional primitive masks, interior cells 1..62.
    for (int s=0;s<5;++s) {
        const int type=r.shapeType[s], cx=r.shapeX[s], cy=r.shapeY[s], rr=r.shapeRadius[s];
        if (!type || rr<=0) continue;
        for (int y=1;y<63;++y) for(int x=1;x<63;++x) {
            bool hit=false;
            if(type==1) hit=(x>=cx-rr && x<=cx+rr && y>=cy-rr && y<=cy+rr);
            else if(type==2) { const int dx=x-cx,dy=y-cy; hit=(dx*dx+dy*dy)<rr*rr; }
            if(hit) set(x,y,Safe);
        }
    }
}

void Field::floodClear(int sx,int sy,uint8_t marker) {
    if(!inside(sx,sy) || at(sx,sy)!=marker) return;
    std::vector<std::pair<int,int>> q; q.reserve(4096); q.emplace_back(sx,sy); set(sx,sy,Empty);
    for(std::size_t i=0;i<q.size();++i){
        const auto [x,y]=q[i];
        const int nx[4]={x+1,x-1,x,x}, ny[4]={y,y,y+1,y-1};
        for(int k=0;k<4;++k) if(inside(nx[k],ny[k]) && at(nx[k],ny[k])==marker){
            set(nx[k],ny[k],Empty); q.emplace_back(nx[k],ny[k]);
        }
    }
}

uint8_t Field::beginCapture(const std::vector<GridSeed>& seeds) {
    uint8_t marker=0;
    for(uint8_t m=1;m<16;++m) if(markerTimer_[m]<=0.0f){marker=m;break;}
    if(!marker) return 0;
    markerTimer_[marker]=0.0001f;
    for(auto& c:cell_) if(c==Empty)c=marker;
    for(const auto& s:seeds) floodClear(s.x,s.y,marker);
    startedCaptureMarker_=marker;
    return marker;
}

int Field::markerCellCount(std::uint8_t marker) const{
    if(marker==0)return 0;
    int n=0;
    for(const auto c:cell_)if(c==marker)++n;
    return n;
}

void Field::updateCaptureAnimations(int dtMs) {
    for(uint8_t m=1;m<16;++m){
        if(markerTimer_[m]<=0.0f) continue;
        markerTimer_[m]+=static_cast<float>(dtMs)*0.00002f;
        // DIRECT EXE 0x418630..0x418644: fcomp 0x43B49C followed by
        // test(C0|C3) means only phase > 0.008 finalizes. Equality remains
        // an active marker until a later update.
        if(markerTimer_[m]>0.00800000037997961f){
            for(auto& c:cell_) if(c==m)c=Safe;
            markerTimer_[m]=0.0f;
        }
    }
}
int Field::occupiedMaskedCount() const { int n=0; for(auto c:cell_) if((c&0x3f)!=0)++n; return n; }
int Field::nonzeroCount() const { int n=0; for(auto c:cell_) if(c!=0)++n; return n; }
bool Field::markerActive(uint8_t m) const { return m<16 && markerTimer_[m]>0.0f; }
float Field::markerPhase(uint8_t m) const { return m<16?markerTimer_[m]:0.0f; }
