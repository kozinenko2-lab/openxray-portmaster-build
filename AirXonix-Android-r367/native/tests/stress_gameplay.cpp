#include "game/level.hpp"
#include "game/field.hpp"
#include "game/entities.hpp"
#include "game/pickups.hpp"
#include "game/special_objects.hpp"
#include "game/player.hpp"
#include "game/game.hpp"
#include "render/legacy_mesh_builder.hpp"
#include "render/legacy_model_factory.hpp"
#include "render/legacy_runtime_blob.hpp"
#include "render/legacy_camera.hpp"
#include <algorithm>
#include <cmath>
#include <cassert>
#include <iostream>
#include <stdexcept>
int main(){
  {
    LegacyMeshBuilder mb;
    mb.addBeveledPlane(-0.02f, 0.0f, -0.02f, 0.04f, 0.04f, 0.001f, 0.f, 1.f, 0.f, 1.f);
    if (mb.vertexCount()!=8 || mb.faceCount()!=3) throw std::runtime_error("beveled-plane topology mismatch");
    if (mb.mesh().faces[0].index != std::vector<std::uint32_t>({7,0,1,2}) ||
        mb.mesh().faces[1].index != std::vector<std::uint32_t>({7,2,3,6}) ||
        mb.mesh().faces[2].index != std::vector<std::uint32_t>({6,3,4,5}))
      throw std::runtime_error("beveled-plane original face order mismatch");
    mb.clear();
    mb.addBeveledPrism(-0.02f, 0.001f, -0.02f, 0.04f, 0.04f, 0.005f, 0.f, 1.f, 0.f, 1.f);
    if (mb.faceCount()!=14) throw std::runtime_error("beveled-prism face count mismatch");
    for (const auto& v: mb.mesh().vertices) {
      if (!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z)||!std::isfinite(v.nx)||!std::isfinite(v.ny)||!std::isfinite(v.nz)||!std::isfinite(v.u)||!std::isfinite(v.v))
        throw std::runtime_error("beveled prism contains non-finite data");
    }
    mb.clear();
    const std::vector<LegacyProfilePoint> profile={{0.f,-1.f},{1.f,0.f},{0.f,1.f}};
    mb.addRevolvedProfile(profile, -1.57079632679f, 1.57079632679f, 8, 0.f, 1.f, 0.f, 1.f, 0.01f);
    if (mb.vertexCount()!=27 || mb.faceCount()!=16) throw std::runtime_error("full lathe topology mismatch");
    const auto fullLathe=mb.mesh();
    // Full revolution duplicates the seam ring at 2pi; positions/normals must match.
    for (std::size_t j=0;j<profile.size();++j) {
      const auto& a=fullLathe.vertices[j];
      const auto& b=fullLathe.vertices[8*profile.size()+j];
      if (std::fabs(a.x-b.x)>1e-5f || std::fabs(a.y-b.y)>1e-6f || std::fabs(a.z-b.z)>1e-5f)
        throw std::runtime_error("full lathe seam mismatch");
    }
    mb.clear();
    mb.addRevolvedProfileArc(0.f, 1.57079632679f, profile, -1.57079632679f, 1.57079632679f, 4, 0.f, 1.f, 0.f, 1.f, 0.01f);
    if (mb.vertexCount()!=15 || mb.faceCount()!=8) throw std::runtime_error("arc lathe topology mismatch");
    const auto crawlerLow=LegacyModelFactory::buildGroundEnemy(false);
    const auto crawlerHigh=LegacyModelFactory::buildGroundEnemy(true);
    if (crawlerLow.vertices.empty() || crawlerHigh.vertices.empty()) throw std::runtime_error("crawler model empty");
    if (crawlerHigh.faces.size()<=crawlerLow.faces.size()) throw std::runtime_error("crawler quality path mismatch");

    for (int subtype=0;subtype<4;++subtype) {
        const auto airLow=LegacyModelFactory::buildAirEnemySubtype(subtype,false);
        const auto airHigh=LegacyModelFactory::buildAirEnemySubtype(subtype,true);
        if (airLow.vertices.empty() || airHigh.vertices.empty()) throw std::runtime_error("air enemy model empty");
        if (airHigh.faces.empty() || airLow.faces.empty()) throw std::runtime_error("air enemy topology empty");
    }

    LegacyMeshBuilder hemi;
    hemi.addAdaptiveHemisphere(12,1.f,0.f,1.f,0.f,1.f);
    if (hemi.mesh().vertices.empty() || hemi.mesh().faces.empty()) throw std::runtime_error("hemisphere empty");
    float minY=1e9f,maxY=-1e9f;
    for (const auto& v:hemi.mesh().vertices) { minY=std::min(minY,v.y); maxY=std::max(maxY,v.y); }
    if (minY < -1e-5f || maxY < 0.999f) throw std::runtime_error("hemisphere range mismatch");

    for (int pickupType=0; pickupType<6; ++pickupType) {
        for (bool highQuality: {false,true}) {
            const auto pickup=LegacyModelFactory::buildPickupType(pickupType,highQuality);
            if (pickup.vertices.empty() || pickup.faces.empty())
                throw std::runtime_error("pickup model empty");
        }
    }

    const auto heart=LegacyModelFactory::buildPickupType(2,true);
    if (heart.vertices.empty() || heart.faces.empty()) throw std::runtime_error("heart pickup model empty");
    float heartMinY=1e9f,heartMaxY=-1e9f;
    for (const auto& v:heart.vertices) { heartMinY=std::min(heartMinY,v.y); heartMaxY=std::max(heartMaxY,v.y); }
    if (heartMinY > -0.0054f || heartMaxY < 0.0057f) throw std::runtime_error("heart pickup contour scale mismatch");

    const auto trailNormal=LegacyModelFactory::buildTrailSegment(false,true);
    const auto trailDamaged=LegacyModelFactory::buildTrailSegment(true,true);
    if (trailNormal.vertices.empty() || trailNormal.faces.empty() ||
        trailDamaged.vertices.empty() || trailDamaged.faces.empty())
      throw std::runtime_error("trail procedural models empty");
    float normalMaxY=-1e9f, damagedMaxY=-1e9f;
    for(const auto& v:trailNormal.vertices) normalMaxY=std::max(normalMaxY,v.y);
    for(const auto& v:trailDamaged.vertices) damagedMaxY=std::max(damagedMaxY,v.y);
    if (!(damagedMaxY>normalMaxY)) throw std::runtime_error("damaged trail profile mismatch");

    const auto homingLow=LegacyModelFactory::buildHoming(false);
    const auto homingHigh=LegacyModelFactory::buildHoming(true);
    assert(!homingLow.body.vertices.empty() && !homingLow.dome.vertices.empty());
    assert(!homingHigh.body.vertices.empty() && !homingHigh.dome.vertices.empty());
    assert(homingHigh.body.vertices.size() > homingLow.body.vertices.size());
    assert(homingHigh.dome.vertices.size() > homingLow.dome.vertices.size());

    const auto eraser=LegacyModelFactory::buildEraser(true);
    if (eraser.star.vertices.empty() || eraser.star.faces.empty() ||
        eraser.core.vertices.empty() || eraser.core.faces.empty())
      throw std::runtime_error("eraser procedural model incomplete");

    const auto xLow=LegacyModelFactory::buildXonix(false);
    const auto xHigh=LegacyModelFactory::buildXonix(true);
    if (xLow.body.vertices.empty() || xLow.propeller.vertices.empty() || xLow.rotorNode.vertices.empty())
        throw std::runtime_error("xonix low model incomplete");
    if (xHigh.body.vertices.empty() || xHigh.propeller.vertices.empty() || xHigh.rotorNode.vertices.empty())
        throw std::runtime_error("xonix high model incomplete");
    if (xHigh.body.faces.size()<=xLow.body.faces.size()) throw std::runtime_error("xonix quality path mismatch");
    if (crawlerLow.vertices.empty() || crawlerHigh.vertices.empty() || crawlerLow.faces.empty() || crawlerHigh.faces.empty())
      throw std::runtime_error("crawler procedural model is empty");
    if (crawlerLow.faces.size() >= crawlerHigh.faces.size())
      throw std::runtime_error("crawler quality switch did not increase spike geometry");
    mb.clear();
    mb.addAdaptiveSphere(16, 0.01f, 0.f, 1.f, 0.f, 1.f);
    if (mb.vertexCount()==0 || mb.faceCount()==0) throw std::runtime_error("adaptive sphere is empty");
    for (const auto& v: mb.mesh().vertices) {
      if (!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z)||!std::isfinite(v.nx)||!std::isfinite(v.ny)||!std::isfinite(v.nz)||!std::isfinite(v.u)||!std::isfinite(v.v))
        throw std::runtime_error("adaptive sphere contains non-finite data");
    }
    mb.clear();
    mb.addAnnularStrip(0.f,0.f,0.004f,0.005f,0.f,14,0.f,6.28318530718f,0.f,1.f,0.f,1.f,false);
    if (mb.vertexCount()!=30 || mb.faceCount()!=14) throw std::runtime_error("annular-strip topology mismatch");

    // 0x401010 exact finalized-memory partition.
    const auto lay=LegacyRuntimeBlob::layout(10,3);
    if (lay.allocationBytes != 0x40u + 880u + 60u ||
        lay.sourceVerticesOffset != 0x20u ||
        lay.transformedVerticesOffset != 0x160u ||
        lay.preparedBlockOffset != 0x2a0u ||
        lay.topologyOffset != 0x394u)
      throw std::runtime_error("0x401010 finalized layout mismatch");

    LegacyMesh topoMesh;
    topoMesh.vertices.resize(4);
    topoMesh.faces.push_back({{0,1,2}});
    topoMesh.faces.push_back({{0,1,2,3}});
    const auto stream=LegacyRuntimeBlob::serializeTopology(topoMesh);
    if (stream != std::vector<std::uint32_t>({3,0,1,2,4,0,1,2,3,0}))
      throw std::runtime_error("legacy topology serialization mismatch");
    const auto tri=LegacyRuntimeBlob::triangulateTopology(topoMesh);
    if (tri != std::vector<std::uint32_t>({0,1,2,0,1,2,0,2,3}))
      throw std::runtime_error("legacy quad triangulation mismatch");

    std::vector<LegacyMasterVertex> lit(2);
    lit[0].ny=1.f; lit[0].u=.25f; lit[0].v=.5f;
    lit[1].ny=-1.f;
    const auto prepared=LegacyRuntimeBlob::prepareVertices(lit,{.2f,0.f,-1.f,0.f});
    if (std::fabs(prepared[0].materialOrLight-1.2f)>1e-6f ||
        std::fabs(prepared[1].materialOrLight-.2f)>1e-6f ||
        prepared[0].u!=.25f || prepared[0].v!=.5f)
      throw std::runtime_error("0x401570 CPU lighting mismatch");

    const auto cb=LegacyCamera::basis(0,0,0);
    if (std::fabs(cb.m00-1.f)>1e-6f || std::fabs(cb.m11+1.f)>1e-6f ||
        std::fabs(cb.m22-1.f)>1e-6f)
      throw std::runtime_error("0x40C250 zero-angle basis mismatch");
    const float det = cb.m00*(cb.m11*cb.m22-cb.m12*cb.m21)
                    - cb.m01*(cb.m10*cb.m22-cb.m12*cb.m20)
                    + cb.m02*(cb.m10*cb.m21-cb.m11*cb.m20);
    if (std::fabs(det+1.f)>1e-5f)
      throw std::runtime_error("0x40C250 handedness mismatch");
    const auto ps=LegacyCamera::projectionState(640,480);
    if (std::fabs(ps.focal-320.f)>1e-6f || std::fabs(ps.rightSlope-319.f/320.f)>1e-6f ||
        std::fabs(ps.bottomSlope-239.f/320.f)>1e-6f)
      throw std::runtime_error("0x40C0C0 projection-state mismatch");
    const auto cp=LegacyCamera::transformRelative(cb,.1f,.2f,.5f);
    if (std::fabs(cp.x-.1f)>1e-6f || std::fabs(cp.y+.2f)>1e-6f || std::fabs(cp.depth-.5f)>1e-6f)
      throw std::runtime_error("0x40C350 camera transform mismatch");
    const auto sp=LegacyCamera::project(ps,{0.f,0.f,1.f});
    if (std::fabs(sp.x-320.f)>1e-6f || std::fabs(sp.y-240.f)>1e-6f || std::fabs(sp.rhw-1.f)>1e-6f)
      throw std::runtime_error("0x40C350 projection mismatch");
  }
  LevelDatabase db; LegacyRandom rng(1);
  size_t levels=0; int erasers=0, homings=0;
  for(size_t m=0;m<db.modes().size();++m){
    for(size_t li=0;li<db.modes()[m].levels.size();++li){
      const auto &r=db.level(m,li); Field f; f.build(r); Player p; p.reset(); Entities e; e.reset(r,f,rng); Pickups pk; pk.reset(f,rng,61440); SpecialObjects sp; sp.reset(r,rng);
      if(sp.eraser().active) ++erasers;
      if(sp.homing().active) ++homings;
      for(int frame=0;frame<200;++frame){ f.updateCaptureAnimations(16); e.update(16,f,rng,1.0f); sp.update(16,f,p,1.0f,rng); pk.prepareFinalSceneFrame(f,rng,1000000,255.f);
      pk.update(16,f,p,e,rng,1000000,3); if(sp.consumeTrailHit()) p.kill(); }
      ++levels;
    }
  }
  Game g; InputState in{}; for(int i=0;i<2000 && !g.wantsQuit();++i)g.update(in,16);
  std::cout<<"levels="<<levels<<" eraser="<<erasers<<" homing="<<homings<<" score="<<g.score()<<"\n";
}
