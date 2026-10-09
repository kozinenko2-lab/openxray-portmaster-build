#include "game/pickups.hpp"
#include <cassert>
#include <iostream>

struct PickupsTestProbe {
    static std::array<Pickup,Pickups::Count>& items(Pickups& p){ return p.items_; }
    static void respawn(Pickups& p,std::size_t i,const Field& f,LegacyRandom& r,int timer){ p.respawn(i,f,r,timer); }
};

int main(){
    Field field;
    Pickups p;
    auto& items=PickupsTestProbe::items(p);

    // Arrange the first candidate from seed 1 to collide with a *later* slot's
    // old position. Native 0x4177C0 sees that old record and rerolls slot 0.
    LegacyRandom preview(1u);
    const int firstX=preview.mask(63);
    const int firstY=preview.mask(63);
    for(std::size_t i=0;i<items.size();++i){
        items[i].gridX=50+int(i); items[i].gridY=50;
    }
    items[5].gridX=firstX;
    items[5].gridY=firstY;

    LegacyRandom rng(1u);
    p.reset(field,rng,60000);
    assert(!(p.items()[0].gridX==firstX && p.items()[0].gridY==firstY));

    // First-process level starts from BSS-style zero records, not sentinels.
    Pickups fresh;
    for(const auto& q:fresh.items()){
        assert(q.gridX==0 && q.gridY==0);
    }


    // DIRECT EXE 0x417919..0x417934: slot 1 delay is 12000 + 1100*(rand&7),
    // not the old reconstructed 1300 multiplier.
    Pickups timerProbe;
    auto& tp=PickupsTestProbe::items(timerProbe);
    for(std::size_t i=0;i<tp.size();++i){ tp[i].gridX=-1000-int(i)*16; tp[i].gridY=-1000; }
    LegacyRandom timerPreview(123u);
    (void)timerPreview.mask(63); // accepted X
    (void)timerPreview.mask(63); // accepted Y
    const int delayRoll=int(timerPreview.mask(7));
    LegacyRandom timerRng(123u);
    PickupsTestProbe::respawn(timerProbe,1,field,timerRng,60000);
    assert(timerProbe.items()[1].timerMs==12000+1100*delayRoll);

    std::cout << "pickup reset history r362 ok\n";
}
