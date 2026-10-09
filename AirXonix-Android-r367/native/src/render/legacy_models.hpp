#pragma once
#include <cstdint>
#include <array>

// High-confidence procedural model identities/parameters recovered from the
// original x86 AirXonix executable.  Keep this independent from GLES so the
// original model recipes can be reconstructed before choosing GPU storage.
namespace LegacyModels {

struct XonixSpec {
    int bodySegmentsLow = 12;
    int bodySegmentsHigh = 16;
    float bodyScale0 = 0.0018f;
    float bodyScale1 = 0.126953125f;
    float bodyScale2 = 0.154296875f;
    float bodyScale3 = 0.001953125f;
    float bodyScale4 = 0.029296875f;

    float centralProfile[6] = {0.01f, 3.0f, 0.2f, 2.9f, 0.2f, 2.5f};
    int centralRings = 3;
    int centralSegmentsLow = 6;
    int centralSegmentsHigh = 8;
    float centralSweep = 0.7853981633974483f; // pi/4
    float centralOffsetA = 0.0075f;
    float centralOffsetB = 0.00505f;

    float shellProfileA[4] = {0.7f, 1.5f, 0.7f, 1.0f};
    float shellProfileB[4] = {0.7f, 1.0f, 1.02f, 1.0f};
    float shellSweep = 1.5707963267948966f; // pi/2
    float revolveStart = 0.5235987755982988f; // pi/6
    float revolveEndA = 3.1415926535897932f;
    float revolveEndB = 3.6651914291880923f; // 7*pi/6

    int rotorSegmentsLow = 8;
    int rotorSegmentsHigh = 12;
    float rotorThickness = 0.0018f;
    float rotorExtentA = 0.2421875f;
    float rotorExtentB = 0.0390625f;

    float rotorYOffset = 0.006f;
    float rotorSafeRadius = 0.0047f;
    float rotorCutRadius = 0.0037f;
    float rotorSafePhasePerMs = 0.007f;
    float rotorCutPhasePerMs = 0.013f;
};
inline constexpr XonixSpec Xonix{};


struct LegacyInitialLightingTrace {
    std::uint32_t setupCall=0x00424C2Au;
    std::uint32_t setupRoutine=0x0040E380u;
    std::uint32_t preparedLightingRoutine=0x00401570u;
    std::uint32_t ambientGlobal=0x0053D8C0u;
    std::uint32_t dirXGlobal=0x0053D830u;
    std::uint32_t dirYGlobal=0x0053D834u;
    std::uint32_t dirZGlobal=0x0053D838u;
    float azimuth=0.7853981633974483f;      // +pi/4
    float elevation=-0.7853981633974483f;   // -pi/4
    float directionalIntensity=0.8f;
    float ambient=0.2f;
    // Exact 0x40E380 result for the startup/gameplay lighting state.
    float dirX=0.4f;
    float dirY=-0.565685424949238f;
    float dirZ=0.4f;
};
inline constexpr LegacyInitialLightingTrace InitialLighting{};

struct XonixPropellerTrace {
    std::uint32_t firstBuilderCall=0x00420898u;
    std::uint32_t secondBuilderCall=0x0042091Du;
    std::uint32_t arcBuilder=0x00402DD0u;
    std::uint32_t preparedHandleGlobal=0x025849D8u;
    float firstStart=0.f;
    float firstEnd=0.5235987901687622f;      // pi/6
    float secondStart=3.1415927410125732f;   // pi
    float secondEnd=3.665191650390625f;      // 7*pi/6
    // Both EXE calls use the same profile/UV/material tuple; only the angular
    // interval differs. The yellow vertical shape in the old diagnostic
    // screenshot was the provisional trail, not a third propeller blade.
};
inline constexpr XonixPropellerTrace XonixPropeller{};


struct LegacyGameplayModelTextureTrace {
    // 0x41A24D calls 0x4201E0. Its high-resolution exit selects logical
    // texture 3 at 0x420641/0x420643 for 0x4247E0 and leaves that state active.
    // The immediately following Xonix/auxiliary/enemy/pickup model calls then
    // consume atlas-3 UVs until an explicit later state change.
    std::uint32_t worldRenderCall=0x0041A24Du;
    std::uint32_t renderFrontend=0x004201E0u;
    std::uint32_t texture3SelectSite=0x00420641u;
    std::uint32_t textureSelectRoutine=0x004059F0u;
    std::uint32_t xonixDrawCall=0x0041A2B6u;
    std::uint32_t pickupDrawRoutine=0x004179B0u;
    int logicalTexture=3;
};
inline constexpr LegacyGameplayModelTextureTrace GameplayModelTexture{};

struct BuilderMapEntry {
    unsigned address;
    const char* object;
    int variants;
};

// Confirmed by tracing each serialized mesh pointer from startup construction
// into the gameplay renderer, not by screenshot resemblance.
inline constexpr BuilderMapEntry ConfirmedBuilders[] = {
    {0x420660u, "Xonix compound body + repeated rotor submesh", 1},
    {0x420C10u, "Ground/crawler enemy compound model", 1},
    {0x420F40u, "Homing special compound model", 2},
    {0x421190u, "Field-eraser special compound model", 2},
    {0x4213A0u, "Xonix active-trail normal/damaged segment models", 2},
    {0x421540u, "Airborne enemy subtype model", 4},
    {0x421730u, "Gameplay pickup/bonus model", 6},
    {0x422240u, "Six auxiliary effect/event models used by 0x415880", 6},
    {0x4223E0u, "Six per-type cinematic/sequence models used by level/final sequences", 6},
};

// 0x420F40 creates two serialized components.  They are consumed directly by
// the homing-special renderer in 0x415440..0x415693 (globals 0x257F568 and
// 0x2583940).  The first component starts from an eight-point radial profile;
// the second is a larger revolution/profile surface.
struct HomingModelSpec {
    int componentCount = 2;
    int profilePoints = 8;
    float profileSkin = 0.01f; // first profile begins 0.01,1.0,0.3,0.0
};
inline constexpr HomingModelSpec Homing{};

// 0x421190 creates two components used by 0x414F30, the field-eraser update /
// renderer.  One component is transformed repeatedly while the second is
// rendered as the main body; both are needed for the original appearance.
struct EraserModelSpec {
    int componentCount = 2;
    int radialPasses = 8;
};
inline constexpr EraserModelSpec Eraser{};

// 0x4213A0 creates two closely related meshes.  0x41A73F..0x41A80D walks the
// 4-byte TrailCell array and selects one by the per-segment damaged flag.
struct TrailModelSpec {
    int variantCount = 2;
};
inline constexpr TrailModelSpec Trail{};

struct AirEnemyModelSetSpec {
    int subtypeCount = 4;
    int radialSegments = 16;
    int profilePoints = 8;
};
inline constexpr AirEnemyModelSetSpec AirEnemies{};

struct PickupModelSetSpec { int typeCount = 6; };
inline constexpr PickupModelSetSpec Pickups{};

// Direct r34 re-trace of 0x415880 confirms six 16-byte auxiliary state slots,
// model/material pointer arrays and per-slot one-shot SFX for slots 0..4.
// r49 closes slot 5 functionally: direct id5 is spawned by timer expiration at
// 0x419797. Slots 0..4 keep their gameplay-effect meanings from 0x4156C0; the
// exact user-visible mesh artwork name remains deliberately neutral.
struct AuxiliaryEffectModelSetSpec {
    unsigned builderAddress = 0x422240u;
    unsigned consumerAddress = 0x415880u;
    unsigned stateBase = 0x0257DB00u;
    unsigned modelHandleBase = 0x0257F550u;
    unsigned materialHandleBase = 0x0257F588u;
    int stateStride = 16;
    int slotCount = 6;
    unsigned initAddress = 0x4156A0u;
    unsigned spawnAddress = 0x4156C0u;
    float inactiveY = 0.5f;
    float spawnYOffset = 0.01f;
    float activeYLimit = 0.11f;
    float oneShotYThreshold = 0.03f;
    std::array<float,6> risePerMs{{5.0e-5f,5.0e-5f,5.0e-5f,5.0e-5f,5.0e-5f,2.5e-5f}};
    // Direct 0x4158E4..0x41598E trace: slots 0..4 have one-shot SFX;
    // slot 5 follows a separate render/update path and has no matching branch.
    std::array<int,6> oneShotSfx{{0x21,0x24,0x20,0x25,0x26,-1}};
    std::array<float,6> oneShotScale{{1.2f,1.1f,1.4f,1.0f,1.0f,0.0f}};
    std::uint32_t timeoutSpawnCall=0x00419797u;
    int timeoutSlot=5;
    // Exact raw 10-float 0x4041A0 constructor tuple for slot 5 from
    // 0x422367..0x422396. Parameter semantics stay tied to the shared
    // 0x4041A0 contract; no invented mesh name is assigned.
    std::array<float,10> timeoutModelArgs{{
        0.748046875f,0.564453125f,0.998046875f,0.001953125f,
        0.0010000000474974513f,0.007000000216066837f,
        0.03999999910593033f,-0.0035000001080334187f,
        0.0f,-0.019999999552965164f
    }};
    std::uint32_t timeoutModelArgsBegin=0x00422367u;
    std::uint32_t timeoutModelBuildCall=0x00422396u;
};
inline constexpr AuxiliaryEffectModelSetSpec AuxiliaryEffects{};

struct CinematicModelSetSpec {
    unsigned builderAddress = 0x4223E0u;
    int slotCount = 6;
};
inline constexpr CinematicModelSetSpec CinematicEffects{};

// 0x421730 loops over six pickup types and writes finalized render pointers to
// 0x25835E4[6].  The normal gameplay renderer directly indexes this array while
// walking the six 24-byte pickup records, proving this is the pickup builder.
// 0x422240 is a different six-way builder used by the auxiliary event/effect
// system at 0x415880. r128 names slots 0..4 from their SFX cues and r156 binds
// slot 5 to the TOU2 / "ВРЕМЯ ВЫШЛО!" timeout presentation on texture 7.

// 0x420C10 finalizes directly into global 0x257F5B8.  The gameplay renderer
// uses that pointer for every 28-byte GroundEnemy record beginning near
// 0x254D8E8, proving this is the crawler/ground-enemy model builder.
struct GroundEnemyModelSpec {
    float profile0 = 0.01f;
    float profile1 = 1.25f;
    float profile2 = 0.315f;
    float profile3 = 0.0f;
    float baseScale = 0.002f;
    float uvOrShapeA = 0.029296875f;
    float uvOrShapeB = 0.001953125f;
    float extentA = 0.2265625f;
    float sweep = 0.5235987901687622f; // pi/6
    float ringStep = 0.39269909262657166f; // pi/8
    float quarterTurn = 1.5707963705062866f; // pi/2
    float childScale = 0.003f;
};
inline constexpr GroundEnemyModelSpec GroundEnemy{};

// r41 direct EXE trace of airborne-enemy presentation in the gameplay renderer.
// Record base 0x257E3D0, stride 0x50.  At 0x41A45B..0x41A49C the original
// constructs the model matrix as Y(-heading), Z(angularAccumulator), Y(heading)
// before transforming the subtype mesh.  The native r40 Y-only rotation was
// therefore incomplete and could make some enemies look quarter-turned.
struct AirEnemyPresentationSpec {
    std::uint32_t renderBegin = 0x0041A43Eu;
    std::uint32_t rotateYNegCall = 0x0041A465u;
    std::uint32_t rotateZCall = 0x0041A475u;
    std::uint32_t rotateYPosCall = 0x0041A485u;
    std::uint32_t modelTransformCall = 0x0041A49Cu;
    std::uint32_t rotateXHelper = 0x004012F0u;
    std::uint32_t rotateYHelper = 0x00401380u;
    std::uint32_t rotateZHelper = 0x00401400u;
};
inline constexpr AirEnemyPresentationSpec AirEnemyPresentation{};

// 0x422D20 hand-builds the flat 14-segment decal used below each airborne
// enemy.  0x422DE0 renders it at y=0 using the enemy x/z position.
struct AirEnemyShadowSpec {
    // r47 direct EXE: exact source count and literal face stream.  The six
    // records at 0x422D3D..0x422D6A store indices as byte offsets into a
    // 44-byte transformed workspace.  Normalized by 44 they are:
    // [1,0,13,12] [2,1,12,11] [3,2,11,10]
    // [4,3,10,9] [5,4,9,8] [6,5,8,7].
    int sourceVertexCount = 14;
    int segments = 14;
    int faceCount = 6;
    int faceArity = 4;
    std::uint32_t transformedIndexStride = 44u;
    std::uint32_t faceStreamBegin = 0x00422D3Du;
    std::uint32_t faceStreamEnd = 0x00422D6Au;
    std::array<std::array<int,4>,6> literalFaces{{
        {{1,0,13,12}}, {{2,1,12,11}}, {{3,2,11,10}},
        {{4,3,10,9}}, {{5,4,9,8}}, {{6,5,8,7}}
    }};
    float radius = 0.005f;
    float angleStep = 0.4487989544868469f; // 2*pi/14

    // r43/r46 direct EXE trace: all eight 0x422DE0 callers select legacy
    // texture handle 0.  Multiple paths explicitly disable alpha blending.
    int confirmedTextureHandle = 0;
    std::array<std::uint32_t,8> drawCalls{{
        0x0041A890u,0x0041B1B9u,0x0041BD3Eu,0x0041CE0Du,
        0x0041D44Cu,0x0041DA9Bu,0x0041E4C4u,0x0041EBEEu
    }};
    std::array<std::uint32_t,8> textureSelectCalls{{
        0x0041A851u,0x0041B17Au,0x0041BD01u,0x0041CDD0u,
        0x0041D40Fu,0x0041DA5Eu,0x0041E487u,0x0041EBB1u
    }};
    std::array<std::uint32_t,4> explicitAlphaDisableCalls{{
        0x0041B143u,0x0041CD46u,0x0041D3FAu,0x0041DA55u
    }};
    std::uint32_t builderRoutine = 0x00422D20u;
    std::uint32_t drawRoutine = 0x00422DE0u;

    // Exact 24-byte source vertex consumed by 0x40C350:
    //   +00/+04/+08 = XYZ
    //   +0C/+10     = U/V
    //   +14         = scalar light/intensity
    // Proof: 0x40C3FB..0x40C474 carries +0C/+10/+14 through clipping;
    // 0x40CCA6..0x40CD10 multiplies +14 by the three active light-colour
    // components and packs D3D diffuse, while +0C/+10 become TL u/v.
    std::uint32_t inputVertexStride = 24u;
    std::uint32_t textureUOffset = 12u;
    std::uint32_t textureVOffset = 16u;
    std::uint32_t lightScalarOffset = 20u;
    std::uint32_t transformedSubmitRoutine = 0x0040C350u;
    std::uint32_t diffusePackBegin = 0x0040CCA6u;
    std::uint32_t diffusePackEnd = 0x0040CD10u;

    // 0x422D20 source geometry.  The draw translation is y=0, but every
    // source vertex itself is raised by 0.0009 to avoid floor z-fighting.
    float builderY = 0.0009f;
    std::uint32_t builderYAddress = 0x00441760u;

    // 0x422DC0/CA globals used by 0x422DEE..0x422E1F.  Each frame:
    // u=(localX+enemyX-0.4)*20, v=(localZ+enemyZ-0.4)*20.
    // This is world-aligned texture mapping, not opacity or normals.
    float uvWorldBias = 0.4f;       // 0x02583928
    float uvWorldScale = 20.f;      // 0x0258392C
    std::uint32_t uvUpdateBegin = 0x00422DEEu;
    std::uint32_t uvUpdateEnd = 0x00422E1Fu;

    // +14 is initialized from 0x025B5B30 at 0x422D83/DBB.  During bootstrap,
    // 0x41ED90 computes this as (0x53D8C0-0x53D834)*0.9*0.5.  With the exact
    // 0x424C16..0x424C2A light setup (pi/4,-pi/4,0.8,0.2), the initial shadow
    // scalar is ~0.34455844.  This affects diffuse RGB; alpha remains disabled.
    std::uint32_t lightScalarGlobal = 0x025B5B30u;
    std::uint32_t lightScalarInit = 0x0041ED90u;
    float initialLightScalar = 0.3445584476f;
};
inline constexpr AirEnemyShadowSpec AirEnemyShadow{};

// r150 direct EXE caller-state audit around every 0x422DE0 submit.  Shadows are
// opaque diffuse decals: alpha blending is disabled, ZFUNC remains LEQUAL, and
// there is no surrounding ZWRITE disable.  The low builder Y=0.0009 keeps the
// decal above the floor while still writing depth before the airborne model.
struct AirEnemyShadowRenderStateSpec {
    std::uint32_t alphaBlendWrapper = 0x00405A60u;
    std::uint32_t zFuncWrapper = 0x00405A20u;
    int textureHandle = 0;
    bool alphaBlendEnabled = false;
    bool depthTestEnabled = true;
    bool depthWriteEnabled = true;
    int legacyZFuncValue = 1; // 0x405A20(1) -> D3DCMP_LESSEQUAL
    int d3dZFunc = 4;         // D3DCMP_LESSEQUAL
};
inline constexpr AirEnemyShadowRenderStateSpec AirEnemyShadowRenderState{};

// r348 DIRECT EXE correction: crawler presentation is split into one real
// texture-3 body draw plus two floor-projection passes. 0x418840/0x418A90 do
// NOT submit the crawler body directly: both call 0x41ECC0, which flattens the
// prepared crawler vertices onto fixed Y planes and regenerates world-aligned
// UVs. The visible pink body is submitted later by 0x41A3E7..0x41A428 through
// the normal 0x40C350 path while logical texture 3 is still selected.
struct GroundEnemyPresentationSpec {
    // Visible body: 28-byte crawler record [0]=vertical/respawn phase,
    // [4]=cached world X, [8]=cached world Z. No per-instance rotation.
    std::uint32_t bodyLoop = 0x0041A3E7u;
    std::uint32_t bodyRecord = 0x0254D8F8u;
    std::uint32_t bodySubmitCall = 0x0041A415u;
    std::uint32_t bodySubmitRoutine = 0x0040C350u;
    std::uint32_t preparedModel = 0x0257F5B8u;
    float bodyYOffset = 0.013000000268220901f; // 0x43B4BC
    int bodyTextureSlot = 3;
    bool perInstanceRotation = false;

    // Floor projections. Keep compatibility aliases used by older tests/tools,
    // but describe these as projections rather than crawler-body passes.
    std::uint32_t renderLoop = 0x00418840u;
    std::uint32_t firstPass = 0x00418840u;
    std::uint32_t secondPass = 0x00418A90u;
    std::uint32_t modelLoad = 0x00418864u;
    std::uint32_t firstModelLoad = 0x00418864u;
    std::uint32_t secondModelLoad = 0x00418AB4u;
    std::uint32_t submitCall = 0x0041887Au;
    std::uint32_t submitRoutine = 0x0041ECC0u;
    std::uint32_t postTransformYOffsetGlobal = 0x02585A64u;
    float firstTransformYOffset = 0.005f;
    float firstPostTransformYOffset = 0.00800000037997961f;
    float secondTransformYOffset = 0.013000000268220901f;
    float secondPostTransformYOffset = 0.0f;

    // 0x41ECC0 constants at 0x441760..0x44176C. Each source vertex is
    // projected as x'=x+.5*worldY+tx, y'=.0009+postY,
    // z'=z+.5*worldY+tz, u=x'*20, v=z'*20.
    float projectionPlaneY = 0.0008999999845400453f;
    float projectionSkewX = 0.5f;
    float projectionSkewZ = 0.5f;
    float projectionUvScale = 20.0f;

    int firstTextureSlot = 2;
    int secondTextureSlot = 0;
    bool firstDepthAlways = true;
    bool secondDepthLessEqual = true;
    float firstLightScalar = 0.5f;
    float secondLightScalar = 0.3499999940395355f;
    std::uint32_t lightScalarGlobal = 0x025B5B30u;
    std::uint32_t secondLightScalarGlobal = 0x025849A8u;
    bool extraTexture3Pass = false;
    bool extraReflectionPass = false;

    constexpr float firstProjectionY() const { return projectionPlaneY + firstPostTransformYOffset; }
    constexpr float secondProjectionY() const { return projectionPlaneY + secondPostTransformYOffset; }
    // Legacy aliases retained for source compatibility. These are NOT body Y.
    constexpr float firstFinalYOffset() const { return firstTransformYOffset + firstPostTransformYOffset; }
    constexpr float secondFinalYOffset() const { return secondTransformYOffset + secondPostTransformYOffset; }
};
inline constexpr GroundEnemyPresentationSpec GroundEnemyPresentation{};

// r45 DIRECT EXE TRACE: 0x41CEA0 selects one of two presentation render paths
// before the main gameplay loop in 0x4195D0. The selector 0x0045000C is the
// same capability flag consulted by legacy D3D SetRenderState wrappers such as
// 0x405A60. The capable path 0x41D4D0 uses atlas handles 7 then 4 near the end
// of its scene; the fallback path 0x41DBC0 uses a different texture-3 prepared
// geometry path. Therefore the 0x41DAC6 texture-7 call is not evidence that
// normal InterLevel renders LEV2/CNT3.
struct StartupPresentationBackendSpec {
    std::uint32_t sessionRoutine = 0x004195D0u;
    std::uint32_t dispatchCall = 0x00419602u;
    std::uint32_t dispatcherRoutine = 0x0041CEA0u;
    std::uint32_t capabilityFlag = 0x0045000Cu;
    std::uint32_t capablePathCall = 0x0041CEBCu;
    std::uint32_t capablePath = 0x0041D4D0u;
    std::uint32_t fallbackPathCall = 0x0041CEC3u;
    std::uint32_t fallbackPath = 0x0041DBC0u;
    std::uint32_t capableTexture7Call = 0x0041DAC6u;
    std::uint32_t capableTexture4Call = 0x0041DB0Fu;
    std::uint32_t fallbackTexture3Call = 0x0041DD82u;
    std::uint32_t renderStateCapabilityGuard = 0x00405A60u;
};
inline constexpr StartupPresentationBackendSpec StartupPresentationBackend{};

// r46 DIRECT EXE TRACE: the six prepared handles at 0x0257F5CC..0x0257F5E0
// are not HUD objects. 0x4223E0 builds six cinematic procedural meshes and,
// after 0x401010/0x401570 finalization, stores render-blob pointers at those
// addresses (0x422584).  This removes 0x0257F5D0 from the CNT/LEV HUD search.
struct CinematicPreparedModelSetSpec {
    std::uint32_t builderRoutine = 0x004223E0u;
    std::uint32_t preparedStore = 0x00422584u;
    std::uint32_t preparedBase = 0x0257F5CCu;
    int count = 6;
    int pointerStride = 4;
    std::array<std::uint32_t,6> prepared{{
        0x0257F5CCu,0x0257F5D0u,0x0257F5D4u,
        0x0257F5D8u,0x0257F5DCu,0x0257F5E0u
    }};
    std::array<std::uint32_t,8> confirmedUses{{
        0x0041AE4Cu,0x0041AE9Fu,0x0041BA1Eu,0x0041BA2Du,
        0x0041CA92u,0x0041DAF8u,0x0041DD9Fu,0x0041E7C6u
    }};
};
inline constexpr CinematicPreparedModelSetSpec CinematicPreparedSet{};




// r63 DIRECT EXE TRACE: the records/high-score screen reuses the compound
// ground/crawler model in two passes. 0x420C10 stores the master render object
// at 0x02585A5C and publishes its prepared child handle at master+0x0C into
// 0x0257F5B8. The records renderer first submits six child/base passes via
// 0x40C350, then queues six master-object reflection/environment-map passes via
// 0x40E710. 0x40E960 later flushes that queue after enabling alpha blend and
// selecting logical texture 6, which is resource 1111 in this constructor family.
struct RecordsCrawlerDecorationTrace {
    std::uint32_t builderRoutine = 0x00420C10u;
    std::uint32_t masterStore = 0x00420F1Du;
    std::uint32_t masterObject = 0x02585A5Cu;
    std::uint32_t preparedChildPublish = 0x00420F28u;
    std::uint32_t preparedChild = 0x0257F5B8u;
    std::uint32_t directSubmitRoutine = 0x0040C350u;
    std::array<std::uint32_t,6> directSubmitCalls{{
        0x0040FD22u,0x0040FD3Cu,0x0040FD57u,
        0x0040FD72u,0x0040FD92u,0x0040FDADu
    }};
    std::uint32_t reflectionQueueRoutine = 0x0040E710u;
    std::array<std::uint32_t,6> reflectionQueueCalls{{
        0x0040FDD1u,0x0040FDF9u,0x0040FE24u,
        0x0040FE4Cu,0x0040FE74u,0x0040FE9Fu
    }};
    std::uint32_t alphaEnableCall = 0x0040FF29u;
    std::uint32_t texture6SelectCall = 0x0040FF30u;
    std::uint32_t reflectionFlushCall = 0x0040FF37u;
    std::uint32_t reflectionFlushRoutine = 0x0040E960u;
    int reflectionTextureHandle = 6;
    const char* reflectionTextureResource = "1111";

    // r63 additional xref closure: the final symmetric pair after the crawler
    // lanes comes from the airborne-enemy builder 0x421540. Arrays
    // 0x257F574[4] / 0x2583748[4] are written at 0x421700 / 0x421713. The
    // records path selects entries 0 and 2.
    std::uint32_t airEnemyBuilder = 0x00421540u;
    std::uint32_t airEnemyMasterStore = 0x00421700u;
    std::uint32_t airEnemyPreparedStore = 0x00421713u;
    std::array<int,2> finalAirEnemySubtypes{{0,2}};
    std::array<std::uint32_t,2> finalAirEnemyMasters{{0x0257F574u,0x0257F57Cu}};
    std::array<std::uint32_t,2> finalAirEnemyPrepared{{0x02583748u,0x02583750u}};
    std::array<std::uint32_t,2> finalAirEnemySubmitCalls{{0x0040FEF6u,0x0040FF24u}};

    // r63 exact six-lane phase state from 0x40F255..0x40F285 and
    // 0x40FC91..0x40FD08. The six initial positions are staggered; three move
    // upward and three downward at 4e-5 world units/ms, wrapping by 0.16 when
    // crossing +0.08 / -0.08. X is +0.08 for the first three and -0.08 for the
    // second three; Z is 0.1 for all six.
    std::array<float,6> initialLaneY{{
        0.08f,0.02666f,-0.02666f,0.05333f,0.0f,-0.05333f
    }};
    float laneXPositive = 0.08f;
    float laneXNegative = -0.08f;
    float laneZ = 0.1f;
    float laneRatePerMs = 0.00004f;
    float laneWrapMin = -0.08f;
    float laneWrapMax = 0.08f;
    float laneWrapSpan = 0.16f;
    std::uint32_t laneInitBegin = 0x0040F255u;
    std::uint32_t laneInitEnd = 0x0040F285u;
    std::uint32_t laneUpdateBegin = 0x0040FC91u;
    std::uint32_t laneUpdateEnd = 0x0040FD08u;
};
inline constexpr RecordsCrawlerDecorationTrace RecordsCrawlerDecoration{};


// r42 direct EXE trace of 0x401160 and 0x4155BE..0x4155F6. 0x401160
// applies an in-place Y-axis rotation to the prepared homing mesh using a
// radians argument (fsincos over X/Z pairs). Native object-space rotateY is
// therefore semantically equivalent when fed the same radians phase.
struct HomingPresentationSpec {
    std::uint32_t vertexRotateHelper = 0x00401160u;
    std::uint32_t helperCall = 0x004155CBu;
    float radiansPerMs = 0.003f;
    bool rotatesAroundY = true;
};
inline constexpr HomingPresentationSpec HomingPresentation{};

// r42 direct EXE trace 0x41535C..0x415428. Eraser has two independent
// animation phases: a radians phase for scale/bob and a legacy angle for Y.
struct EraserPresentationSpec {
    std::uint32_t radiansPhaseGlobal = 0x0254A2A4u;
    std::uint32_t legacyAngleGlobal = 0x0254A2A0u;
    std::uint32_t angleUpdate = 0x004153A1u;
    std::uint32_t rotateYCall = 0x004153EFu;
    std::uint32_t scaleCall = 0x004153FCu;
    float radiansPerMs = 0.018f;
    int angleUnitsPerMs = 2;
    int angleMask = 0x7fb;
    float scaleBase = 1.0f;
    float scaleAmplitude = 0.4f;
    float bobBaseY = 0.005f;
    float bobAmplitudeY = 0.002f;
};
inline constexpr EraserPresentationSpec EraserPresentation{};

// r42 DIRECT EXE TRACE: normal pickup presentation in 0x417D4C..0x417FA4.
// P0/P1/P3/P4/P5 share X(+0x200 = +90 deg) then Y(0x0257DA9C).
// P2/heart is intentionally excluded: it is rebuilt with Y(angle/2) and a
// pulsating scale 0.7 + 0.5*sin(((angle*3)>>2)&0x3ff).
struct PickupPresentationSpec {
    std::array<int,6> pitchX2048{{512,512,0,512,512,512}};
    std::uint32_t drawFunction = 0x004179B0u;
    std::uint32_t sharedAngleGlobal = 0x0257DA9Cu;
    std::uint32_t angleAdvanceCallsite = 0x00417D55u;
    int angleAdvancePerMs = 2;
    std::uint32_t rotateXCall = 0x00417D70u;
    std::uint32_t rotateYCall = 0x00417D80u;
    int fixedPitch2048 = 0x200;
    std::uint32_t heartModelHandle = 0x025849C8u;
    std::uint32_t heartRotateYCall = 0x00417F5Cu;
    std::uint32_t heartScaleCall = 0x00417F8Fu;
    float heartScaleBase = 0.7f;
    float heartScaleAmplitude = 0.5f;

};
inline constexpr PickupPresentationSpec PickupPresentation{};

// Exact gameplay render-submit heights recovered in r80 from the original
// draw call sites. These are presentation offsets only; collision heights stay
// in game-space records.
inline constexpr float SafeFieldTopY=0.008f;          // 0x41F090 vertex Y
inline constexpr float NonSafeFieldY=0.0f;            // 0x41F270 vertex Y
inline constexpr float FieldBoundaryLowY=0.0005f;      // 0x41F980/0x41FB30 transition Y
inline constexpr float AirEnemySubmitY=0.005f;          // 0x41A4A1..0x41A4B6
inline constexpr float GroundEnemySubmitYOffset=0.013f; // record+0x10 + 0x43B4BC
inline constexpr float PickupSubmitYOffset=0.007f;      // record height + 0x43B40C


} // namespace LegacyModels
