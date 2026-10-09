#include "game/legacy_effects.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
    static_assert(sizeof(LegacyParticleRecordContract)==24);
    assert(kLegacyParticlePool.spawnAddress==0x004175B0u);
    assert(kLegacyParticlePool.recordCount==96);
    assert(kLegacyParticlePool.maxSpawnPerHelperCall==16);
    assert(std::fabs(kLegacyParticlePool.spawnY-0.006f)<1e-8f);
    assert(std::fabs(kLegacyParticlePool.freeSlotYThreshold-0.005f)<1e-8f);
    assert(std::fabs(kLegacyParticlePool.gravityPerMs-4.0e-7f)<1e-12f);
    assert(kLegacyPickupSmashParticles.spawnAddress==0x00418040u);
    assert(kLegacyPickupSmashParticles.recordsPerSlot==128);
    assert(kLegacyPickupSmashParticles.slotStride==128*24);
    assert(kLegacyPickupSmashParticles.sfxId==0x0E);
    static_assert(sizeof(LegacyTLVertexContract)==32);
    assert(kLegacyParticleBillboard.setupAddress==0x0040E0D0u);
    assert(kLegacyParticleBillboard.renderAddress==0x0040E120u);
    assert(kLegacyParticleBillboard.verticesPerParticle==4);
    assert(kLegacyParticleBillboard.bytesPerParticle==128);
    assert(kLegacyParticleBillboard.indicesPerParticle==6);
    assert(kLegacyParticleBillboard.staticIndexQuadCapacity==170);
    assert(kLegacyParticleBillboard.debrisDiffuse==0x00CFAF4Fu);
    assert(std::fabs(kLegacyParticleBillboard.debrisU0-0.939453125f)<1e-9f);
    assert(std::fabs(kLegacyParticleBillboard.debrisV0-0.126953125f)<1e-9f);
    assert(std::fabs(kLegacyParticleBillboard.debrisUvExtent-0.05859375f)<1e-9f);
    static_assert(sizeof(LegacyAuxiliaryEffectRecordContract)==16);
    assert(kLegacyAuxiliaryEffects.spawnAddress==0x004156C0u);
    assert(kLegacyAuxiliaryEffects.slotCount==6);
    assert(std::fabs(kLegacyAuxiliaryEffects.inactiveY-0.5f)<1e-9f);
    assert(std::fabs(kLegacyAuxiliaryEffects.spawnYOffset-0.01f)<1e-9f);
    assert(std::fabs(kLegacyAuxiliaryEffects.activeYLimit-0.11f)<1e-8f);
    assert(std::fabs(kLegacyAuxiliaryEffects.slot5RisePerMs-2.5e-5f)<1e-10f);
    assert(kLegacyTimeoutAuxiliary.directId5Call==0x00419797u);
    assert(kLegacyTimeoutAuxiliary.negativeTimerCompare==0x00419760u);
    assert(kLegacyTimeoutAuxiliary.timerGlobal==0x0257DA14u);
    assert(kLegacyTimeoutAuxiliary.auxiliarySlot==5);
    std::cout << "direct EXE particle contracts ok\n";
}
