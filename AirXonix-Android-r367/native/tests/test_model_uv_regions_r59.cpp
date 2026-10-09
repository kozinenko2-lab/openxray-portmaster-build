#include "render/legacy_model_factory.hpp"
#include <cassert>
#include <cmath>
#include <limits>
#include <utility>

static std::pair<float,float> rangeU(const LegacyMesh& m){float lo=1e9f,hi=-1e9f;for(const auto&f:m.faces)for(auto i:f.index){const auto&v=m.vertices.at(i);lo=std::min(lo,v.u);hi=std::max(hi,v.u);}return {lo,hi};}
static std::pair<float,float> rangeV(const LegacyMesh& m){float lo=1e9f,hi=-1e9f;for(const auto&f:m.faces)for(auto i:f.index){const auto&v=m.vertices.at(i);lo=std::min(lo,v.v);hi=std::max(hi,v.v);}return {lo,hi};}
static bool near(float a,float b){return std::fabs(a-b)<2e-6f;}

int main(){
    // Direct-builder audit: all gameplay procedural models sample only the
    // top-left 64x64 material quadrant of atlas #3. This is the region built
    // from BALL/XONI/SPEE/MONY at 0x423350 and selected as logical texture 3.
    const auto x=LegacyModelFactory::buildXonix(true);
    for(const LegacyMesh* m:{&x.body,&x.propeller,&x.rotorNode}){
        const auto u=rangeU(*m),v=rangeV(*m);assert(u.first>=0.f&&u.second<.25f);assert(v.first>=0.f&&v.second<.25f);
    }
    const auto xu=rangeU(x.body),xv=rangeV(x.body);
    assert(near(xu.first,32.5f/256.f)); assert(near(xu.second,47.5f/256.f));
    assert(near(xv.first,.5f/256.f)); assert(near(xv.second,15.5f/256.f));
    const auto pu=rangeU(x.propeller),pv=rangeV(x.propeller);
    assert(near(pu.first,32.5f/256.f)); assert(near(pu.second,39.5f/256.f));
    assert(near(pv.first,8.5f/256.f)); assert(near(pv.second,15.5f/256.f));

    // Four airborne subtypes occupy four 16x16 texel-centred quadrants.
    const float lo[4][2]={{.5f,.5f},{.5f,16.5f},{16.5f,.5f},{16.5f,16.5f}};
    const float hi[4][2]={{15.5f,15.5f},{15.5f,31.5f},{31.5f,15.5f},{31.5f,31.5f}};
    for(int i=0;i<4;++i){
        const auto m=LegacyModelFactory::buildAirEnemySubtype(i,true);const auto u=rangeU(m),v=rangeV(m);
        assert(near(u.first,lo[i][0]/256.f));assert(near(v.first,lo[i][1]/256.f));
        assert(near(u.second,hi[i][0]/256.f));assert(near(v.second,hi[i][1]/256.f));
    }

    // All six pickups stay inside the same model-material quadrant; P5 is a
    // literal single-texel material sample at (60,15), not a sprite blit.
    for(int i=0;i<6;++i){const auto m=LegacyModelFactory::buildPickupType(i,true);const auto u=rangeU(m),v=rangeV(m);assert(u.first>=0.f&&u.second<.25f);assert(v.first>=0.f&&v.second<.25f);}
    const auto p5=LegacyModelFactory::buildPickupType(5,true);const auto p5u=rangeU(p5),p5v=rangeV(p5);
    assert(near(p5u.first,60.f/256.f)&&near(p5u.second,60.f/256.f));
    assert(near(p5v.first,15.f/256.f)&&near(p5v.second,15.f/256.f));
    return 0;
}
