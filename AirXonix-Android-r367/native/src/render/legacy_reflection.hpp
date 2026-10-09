#pragma once
#include "legacy_mesh.hpp"
#include "legacy_transform.hpp"
#include <array>

// Native reconstruction of AirXonix.wrp.exe:0x40E710 / 0x40E960.
// The legacy path builds a second 24-byte render-vertex stream for shiny
// objects. Position is transformed normally, while texture coordinates are
// generated from a camera-oriented normal and rendered later with logical
// texture slot 6 (resource FourCC "1111") under alpha blending.
namespace LegacyReflection {

struct CameraState {
    float x=0.5f;
    float y=0.103f;
    float z=0.35f;
    // Radian globals used by 0x40E710. They are intentionally radians rather
    // than the game's separate 0..2047 camera-angle representation.
    float pitchRadians=0.f; // 0x025418F4
    float yawRadians=0.f;   // 0x025418F0
};

struct ReflectedVertex {
    float x=0.f,y=0.f,z=0.f;
    float u=0.f,v=0.f;
    float light=1.f;
};

// Exact 0x40E5C0 and 0x40E630 matrix operations, exposed for regression tests.
void rotateX(LegacyTransform::Matrix34& m,float radians);
void rotateY(LegacyTransform::Matrix34& m,float radians);
// 0x40E6A0: rotate X/Y columns by a radian angle (Z rotation).
void rotateZ(LegacyTransform::Matrix34& m,float radians);

// Builds the reflection/environment-map vertex stream corresponding to the
// master 32-byte vertices consumed by 0x40E710. Faces/topology are unchanged.
std::vector<ReflectedVertex> buildVertices(const LegacyMesh& mesh,
                                           const LegacyTransform::Matrix34& model,
                                           float tx,float ty,float tz,
                                           const CameraState& camera);

// 0x40E724 multiplies the queue-call intensity by literal 0.6 before the
// later 0x40C140 lighting-colour setup. Reflection vertices themselves store
// 1.0 in their trailing light field.
inline float queuedBrightness(float requestedIntensity){ return requestedIntensity*0.6f; }

// r67 DIRECT EXE: 0x417E12..0x417FE6. Pickup types 0..5 queue their
// master meshes with these exact requested intensities before 0x40E710 applies
// the common 0.6 queue-brightness factor.
inline constexpr std::array<float,6> PickupRequestedIntensity{{
    0.65f,0.95f,0.30f,0.65f,0.95f,0.95f
}};

// r68 DIRECT EXE: homing-special main compound body at 0x414488..0x41448D.
// Requested intensity is 0.18; the queue applies the shared *0.6 factor.
inline constexpr float HomingRequestedIntensity=0.18f;

// Normal gameplay ownership audit (r68): the Xonix helper 0x4209E0,
// crawler draw at 0x418840..0x41887A and airborne loops in 0x41A4xx use
// base 0x40C350 submissions only. Their 0x40E710 appearances belong to
// other screen/composition paths and must not be injected into gameplay.
inline constexpr bool GameplayXonixUsesReflection=false;
inline constexpr bool GameplayCrawlerUsesReflection=false;
inline constexpr bool GameplayAirEnemyUsesReflection=false;

} // namespace LegacyReflection
