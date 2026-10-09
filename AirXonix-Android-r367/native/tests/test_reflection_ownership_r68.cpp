#include "render/legacy_reflection.hpp"
#include <cassert>
#include <cmath>
static bool near(float a,float b,float e=1e-6f){return std::fabs(a-b)<=e;}
int main(){
    assert(near(LegacyReflection::HomingRequestedIntensity,0.18f));
    assert(near(LegacyReflection::queuedBrightness(LegacyReflection::HomingRequestedIntensity),0.108f));
    assert(!LegacyReflection::GameplayXonixUsesReflection);
    assert(!LegacyReflection::GameplayCrawlerUsesReflection);
    assert(!LegacyReflection::GameplayAirEnemyUsesReflection);
    return 0;
}
