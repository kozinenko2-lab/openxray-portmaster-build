#include "render/legacy_models.hpp"
#include "render/legacy_theme.hpp"
#include <cassert>
#include <cmath>
#include <cstring>

int main(){
    const auto& l=LegacyModels::InitialLighting;
    assert(l.setupCall==0x00424C2Au);
    assert(l.setupRoutine==0x0040E380u);
    assert(l.preparedLightingRoutine==0x00401570u);
    assert(std::fabs(l.ambient-0.2f)<1e-6f);
    assert(std::fabs(l.dirX-0.4f)<1e-6f);
    assert(std::fabs(l.dirY+0.565685425f)<1e-6f);
    assert(std::fabs(l.dirZ-0.4f)<1e-6f);

    const auto& p=LegacyModels::XonixPropeller;
    assert(p.arcBuilder==0x00402DD0u);
    assert(std::fabs(p.firstStart-0.f)<1e-6f);
    assert(std::fabs(p.firstEnd-0.52359879f)<1e-6f);
    assert(std::fabs(p.secondStart-3.14159274f)<1e-6f);
    assert(std::fabs(p.secondEnd-3.66519165f)<1e-6f);

    assert(kLegacyMenuEnvironmentThemeTrace.tableAddress==0x00441830u);
    assert(kLegacyMenuEnvironmentThemeTrace.selectRoutine==0x00422F40u);
    assert(kLegacyMenuEnvironmentThemeTrace.themeCount==8);
    assert(std::strcmp(kLegacyMenuEnvironmentThemes[0].slot0,"VOL4")==0);
    assert(std::strcmp(kLegacyMenuEnvironmentThemes[0].slot1,"SKY3")==0);
    assert(std::strcmp(kLegacyMenuEnvironmentThemes[2].slot0,"VOL0")==0);
    assert(std::strcmp(kLegacyMenuEnvironmentThemes[2].slot1,"SK01")==0);

    const auto& t=LegacyModels::GameplayModelTexture;
    assert(t.logicalTexture==3);
    assert(t.texture3SelectSite==0x00420641u);
    assert(t.xonixDrawCall==0x0041A2B6u);
    return 0;
}
