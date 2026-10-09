#include "game/legacy_effects.hpp"
#include "game/legacy_menu.hpp"
#include "render/legacy_models.hpp"
#include <cassert>
#include <iostream>

int main(){
    static_assert(kLegacyParticlePool.spawnAddress==0x004175B0u);
    static_assert(kLegacyParticlePool.updateAddress==0x00417700u);
    static_assert(kLegacyParticlePool.renderAddress==0x0040E120u);
    static_assert(kLegacyParticlePool.baseAddress==0x0254CFE8u);
    static_assert(kLegacyParticlePool.recordCount==96);
    static_assert(kLegacyParticlePool.recordStride==24);
    static_assert(kLegacyParticlePool.maxSpawnPerHelperCall==16);
    static_assert(kLegacyPickupSmashParticles.spawnAddress==0x00418040u);
    static_assert(kLegacyPickupSmashParticles.recordsPerSlot==128);
    using namespace LegacyModels;
    static_assert(AuxiliaryEffects.stateBase==0x0257DB00u);
    static_assert(AuxiliaryEffects.stateStride==16);
    assert(AuxiliaryEffects.oneShotSfx[0]==0x21);
    assert(AuxiliaryEffects.oneShotSfx[4]==0x26);
    assert(kLegacyMainMenuEntries.size()==5);
    std::cout<<"direct EXE metadata contracts ok\n";
}
