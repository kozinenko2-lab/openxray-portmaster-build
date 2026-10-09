#pragma once
#include "legacy_mesh.hpp"


struct EraserModelParts {
    LegacyMesh star;
    LegacyMesh core;
};

struct HomingModelParts {
    // 0x25B5AE0 / prepared 0x257F568: compound spinning hunter body.
    LegacyMesh body;
    // 0x2585A70 / prepared 0x2583940: separately bobbing hemisphere/dome.
    LegacyMesh dome;
};

struct LegacyMainMenuLogoParts {
    LegacyMesh logo; // 0x02585A60 / prepared 0x0257F5C4, logical texture 8 LOGO
    LegacyMesh laxy; // 0x02585AD8 / prepared 0x025B5B24, logical texture 2 LAXY
};

struct XonixModelParts {
    // 0x2585A98: compound body/capsule and central shell.
    LegacyMesh body;
    // 0x25849D8: two opposite 30-degree radial fan/propeller sectors.
    LegacyMesh propeller;
    // 0x25B5B28: small sphere rendered four times around the body.
    LegacyMesh rotorNode;
};

namespace LegacyModelFactory {
// Reconstructs 0x420C10 from its original procedural recipe. `highQuality`
// corresponds to the legacy quality global at 0x43F0AC (4 vs 3 spike-lathe
// radial segments). The body sphere always uses 12 base segments.
LegacyMesh buildGroundEnemy(bool highQuality);

// Reconstructs the four 0x421540 airborne-enemy subtype meshes. High quality
// uses the original 8-point lathed profile; low quality deliberately switches
// to the game's 0x401750 adaptive-sphere fallback. Subtype only selects UVs.
LegacyMesh buildAirEnemySubtype(int subtype, bool highQuality);

// 0x422D20: exact 14-vertex / 6-quad shadow decal geometry. 0x422DE0 draws
// one copy at Y=0 below each airborne enemy with logical texture handle 0.
// r47 decodes the literal face stream; the exact resource uploaded into slot 0
// remains a separate theme/resource-loading question.
LegacyMesh buildAirEnemyShadow();

// Reconstructs 0x420660 into the three meshes consumed by 0x4209E0.
XonixModelParts buildXonix(bool highQuality);
// Reconstructs one of the six gameplay pickup meshes built by 0x421730.
// r14 contains the exact heart/life contour path (type 2) and preserves the
// remaining types for incremental recipe completion.
LegacyMesh buildPickupType(int type, bool highQuality);

// 0x4213A0: the two active Xonix trail segment meshes. The damaged variant
// uses a slightly larger profile and higher vertical offset at draw time.
LegacyMesh buildTrailSegment(bool damaged, bool highQuality);

// 0x420F40: both procedural components used by the homing special renderer.
HomingModelParts buildHoming(bool highQuality);

// 0x421190: both procedural components used by the field-eraser renderer.
EraserModelParts buildEraser(bool highQuality);

// 0x422CBB..0x422D0C: dedicated flat flying-mine model used by
// the 0x414170 additional-enemies information screen.
LegacyMesh buildInformationFlyingMine();

// 0x4220F9..0x422215: exact two models consumed by 0x411EF0.
LegacyMainMenuLogoParts buildMainMenuLogoParts(bool highQuality);
// 0x4225A0: exact seven-model procedural family consumed by 0x412150.
LegacyMesh buildMainMenuDecorationSlot(int slot,bool highQuality);

// r116: literal 0x4223E0 cinematic message meshes. Slots 0 and 5 are the
// mirrored inter-level pair (COMP/CMP2 and GAME/GAM2 on atlas4).
LegacyMesh buildCinematicSlot(int slot);
// 0x422240: six auxiliary effect/event plaques consumed by 0x415880.
LegacyMesh buildAuxiliarySlot(int slot);

}
