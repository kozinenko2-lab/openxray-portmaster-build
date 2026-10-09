#include "render/legacy_models.hpp"
#include <cassert>
#include <iostream>

int main(){
    using namespace LegacyModels;
    static_assert(AuxiliaryEffects.builderAddress==0x422240u);
    static_assert(AuxiliaryEffects.consumerAddress==0x415880u);
    static_assert(AuxiliaryEffects.slotCount==6);
    static_assert(AuxiliaryEffects.stateBase==0x0257DB00u);
    static_assert(AuxiliaryEffects.stateStride==16);
    static_assert(AuxiliaryEffects.initAddress==0x4156A0u);
    static_assert(AuxiliaryEffects.spawnAddress==0x4156C0u);
    static_assert(AuxiliaryEffects.modelHandleBase==0x0257F550u);
    static_assert(AuxiliaryEffects.materialHandleBase==0x0257F588u);
    static_assert(AuxiliaryEffects.oneShotSfx[0]==0x21);
    static_assert(AuxiliaryEffects.oneShotSfx[1]==0x24);
    static_assert(AuxiliaryEffects.oneShotSfx[2]==0x20);
    static_assert(AuxiliaryEffects.oneShotSfx[3]==0x25);
    static_assert(AuxiliaryEffects.oneShotSfx[4]==0x26);
    static_assert(AuxiliaryEffects.oneShotSfx[5]==-1);
    static_assert(AuxiliaryEffects.inactiveY==0.5f);
    static_assert(AuxiliaryEffects.spawnYOffset==0.01f);
    static_assert(AuxiliaryEffects.risePerMs[5]==2.5e-5f);
    static_assert(CinematicEffects.builderAddress==0x4223E0u);
    static_assert(CinematicEffects.slotCount==6);
    static_assert(Pickups.typeCount==6);
    assert(AuxiliaryEffects.builderAddress!=CinematicEffects.builderAddress);
    assert(AuxiliaryEffects.builderAddress!=0x421730u);
    
    assert(AuxiliaryEffects.timeoutSpawnCall==0x00419797u);
    assert(AuxiliaryEffects.timeoutSlot==5);
    assert(AuxiliaryEffects.timeoutModelArgsBegin==0x00422367u);
    assert(AuxiliaryEffects.timeoutModelBuildCall==0x00422396u);
std::cout << "auxiliary/cinematic model metadata ok\n";
}
