#include "game/entities.hpp"
#include <iostream>
#include <string>
#include <utility>

struct EntitiesTestProbe {
    static void erode(Entities& e,Field& f,int x,int y,LegacyRandom& rng){e.erodeAirImpact(f,x,y,rng);}
    static void setOneAir(Entities& e,const AirEnemy& a){e.air_.clear();e.air_.push_back(a);}
};

static LevelRecord emptyLevel(){
    LevelRecord r{};
    return r;
}

static int activeDebris(const Entities& e){
    int n=0;
    for(const auto& p:e.fieldDebris()) if(p.active()) ++n;
    return n;
}

static bool check(bool condition,const std::string& what){
    if(condition) return true;
    std::cerr << "FAIL: " << what << "\n";
    return false;
}

int main(){
    bool ok=true;

    // 0x415FA0 stores all eight old neighbour bytes, clears all eight cells,
    // then calls 0x4175B0 only for saved non-zero bytes.
    {
        Entities e; Field f; f.build(emptyLevel()); LegacyRandom rng(1u);
        f.set(11,10,Field::Safe);
        f.set(9,10,Field::Trail);
        f.set(10,11,3u); // active capture marker is also non-zero
        EntitiesTestProbe::erode(e,f,10,10,rng);
        static constexpr int dx[8]={1,-1,0,0,1,-1,1,-1};
        static constexpr int dy[8]={0,0,1,-1,1,-1,-1,1};
        for(int i=0;i<8;++i)
            ok &= check(f.at(10+dx[i],10+dy[i])==Field::Empty,
                        "0x415FA0 must clear all eight neighbours");
        ok &= check(e.consumeAirErosionImpactEvents()==1,
                    "one erosion call must record one impact event");
        ok &= check(e.consumeAirErosionDebrisHelperCalls()==3,
                    "0x4175B0 must be called only for the three non-zero saved neighbours");
        ok &= check(activeDebris(e)==48,
                    "three debris-helper calls must activate 48 debris records");
    }
    {
        Entities e; Field f; f.build(emptyLevel()); LegacyRandom rng(1u);
        EntitiesTestProbe::erode(e,f,20,20,rng);
        ok &= check(e.consumeAirErosionImpactEvents()==1,
                    "empty impact still counts as one erosion impact");
        ok &= check(e.consumeAirErosionDebrisHelperCalls()==0,
                    "empty neighbours must not invoke 0x4175B0");
        ok &= check(activeDebris(e)==0,
                    "empty neighbours must not create debris");
    }

    // 0x4166CE..0x4166FC: subtypes 0/1 erode around the rounded pre-step
    // cell; subtypes 2/3 erode around the rounded candidate collision cell.
    auto run=[](int subtype){
        Entities e; Field f; f.build(emptyLevel()); LegacyRandom rng(1u);
        AirEnemy a{}; a.x=10.f; a.y=10.f; a.vx=1.f; a.vy=0.f; a.subtype=subtype;
        a.worldX=0.4f+10.f*0.003125f; a.worldZ=0.4f+10.f*0.003125f;
        EntitiesTestProbe::setOneAir(e,a);
        // Candidate is (11,10). This cell triggers candidate-neighbour contact.
        f.set(10,11,Field::Safe);
        // Unique old-centre neighbour and unique candidate-centre neighbour.
        f.set(9,10,Field::Safe);
        f.set(12,10,Field::Safe);
        e.update(1,f,rng,1.f,false,false);
        return std::pair<unsigned,unsigned>{f.at(9,10),f.at(12,10)};
    };
    const auto normal=run(0);
    ok &= check(normal.first==Field::Empty,
                "subtype 0/1 erosion must use the rounded pre-step centre");
    ok &= check(normal.second==Field::Safe,
                "subtype 0/1 erosion must not use the candidate-only neighbour");
    const auto destructive=run(2);
    ok &= check(destructive.first==Field::Safe,
                "subtype 2/3 erosion must not use the old-only neighbour");
    ok &= check(destructive.second==Field::Empty,
                "subtype 2/3 erosion must use the rounded candidate centre");

    if(!ok) return 1;
    std::cout << "air erosion literal r202 ok\n";
    return 0;
}
