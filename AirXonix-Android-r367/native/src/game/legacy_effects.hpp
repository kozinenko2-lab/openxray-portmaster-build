#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string_view>

// Direct AirXonix.wrp.exe particle/debris contract.
//
// Shared gameplay-debris pool:
//   0x004175B0 spawn helper -> 0x0254CFE8
//   0x00417700 CPU update    -> 0x0040E120 renderer
//
// The record semantics are now directly proven by writer/update flow rather
// than inferred from naming: 0x4175B0 writes the first three floats from the
// impact position and randomises the last three; 0x417700 subtracts gravity
// from +0x10 and integrates +0x0C/+0x10/+0x14 into +0x00/+0x04/+0x08.
struct LegacyParticleRecordContract {
    float x;
    float y;
    float z;
    float vx;
    float vy;
    float vz;
};
static_assert(sizeof(LegacyParticleRecordContract)==0x18);

struct LegacyParticlePoolTrace {
    std::uint32_t spawnAddress=0x004175B0u;
    std::uint32_t updateAddress=0x00417700u;
    std::uint32_t renderAddress=0x0040E120u;
    std::uint32_t baseAddress=0x0254CFE8u;
    int recordCount=0x60;
    int recordStride=0x18;
    int maxSpawnPerHelperCall=0x10;
    float spawnY=0.006f;            // 0x3BC49BA6 at 0x004175D6
    float freeSlotYThreshold=0.005f;// 0x3BA3D70A at 0x004175DD
    float horizontalRandomScale=2.0e-7f; // 0x3456BF95
    float verticalRandomScale=5.0e-7f;   // 0x350637BD
    float verticalBias=7.0e-5f;          // 0x3892CCF7
    float gravityPerMs=4.0e-7f;          // 0x33D6BF95 in 0x0041773D
    float deadY=-0.5f;                   // 0xBF000000 in 0x00417744
    std::uint32_t lastSfxTick=0x0257F538u; // GetTickCount cadence state
    std::uint32_t cowSelector=0x0257F4E4u; // zero-init, then (selector+1)&3
    int sfxMinimumGapMs=20;               // strict current-last > 20
    std::array<std::uint32_t,4> cowLogicalIds{{0x1Bu,0x1Cu,0x1Du,0x1Eu}};
    std::array<std::string_view,4> cowFourcc{{"cow2","cow4","cow1","cow3"}};
};
inline constexpr LegacyParticlePoolTrace kLegacyParticlePool{};

// 0x00418040 uses a separate 128-record block per one of the six pickup slots.
// Each block is exactly 0xC00 bytes = 128 * 24 and is initialized in full on
// a pickup smash before SFX 0x0E is emitted.
struct LegacyPickupSmashParticleTrace {
    std::uint32_t spawnAddress=0x00418040u;
    std::uint32_t poolBaseAddress=0x025459BCu;
    int slotCount=6;
    int recordsPerSlot=0x80;
    int recordStride=0x18;
    int slotStride=0xC00;
    int sfxId=0x0E;
};
inline constexpr LegacyPickupSmashParticleTrace kLegacyPickupSmashParticles{};

// r366 DIRECT EXE 0x418040/0x418110 + level init 0x41916F..0x419196.
// Six fixed slot-local pools have independent integer ages. A smash overwrites
// its slot and sets age=0; level init sets all ages to 5000. 0x418110 tests
// age<2500 before adding dt, so a slot may render once with post-step age>2500.
struct LegacyPickupSmashRuntimeTrace {
    static constexpr int slotCount=6;
    static constexpr int recordsPerSlot=128;
    static constexpr int lifetimeMs=2500;
    static constexpr int levelResetAgeMs=5000;
    static constexpr bool activeAtFrameStart(int ageMs){ return ageMs<lifetimeMs; }
    static constexpr int steppedAge(int ageMs,int dtMs){
        return activeAtFrameStart(ageMs)?ageMs+dtMs:ageMs;
    }
};
// 0x0040E0D0 + 0x0040E120 form the legacy CPU billboard batcher.
// 0x40E0D0 stores atlas UV rectangle, projected half-size and diffuse colour;
// 0x40E120 projects each 24-byte world record and emits four D3DTLVERTEX-like
// vertices (32 bytes each) into a locked dynamic vertex buffer.  A static
// 6-index quad pattern, built by 0x40E040, is then submitted as triangles.
struct LegacyTLVertexContract {
    float sx;
    float sy;
    float sz;
    float rhw;
    std::uint32_t diffuse;
    std::uint32_t specular;
    float u;
    float v;
};
static_assert(sizeof(LegacyTLVertexContract)==0x20);

struct LegacyParticleBillboardTrace {
    std::uint32_t indexInitAddress=0x0040E040u;
    std::uint32_t setupAddress=0x0040E0D0u;
    std::uint32_t renderAddress=0x0040E120u;
    int verticesPerParticle=4;
    int bytesPerParticle=0x80;
    int indicesPerParticle=6;
    int staticIndexQuadCapacity=170;
    std::uint32_t lockFlags=0x2001u;
    std::uint32_t specular=0u;
    // Exact setup performed by 0x417700 before rendering the shared debris pool.
    float debrisU0=0.939453125f;     // 0x3F708000
    float debrisV0=0.126953125f;     // 0x3E020000
    float debrisUvExtent=0.05859375f;// 0x3D700000
    std::uint32_t debrisDiffuse=0x00CFAF4Fu;
    float screenScale=0.0005312500288709998f; // 0x43B490
};
inline constexpr LegacyParticleBillboardTrace kLegacyParticleBillboard{};

// Six auxiliary effect state records are written by 0x4156C0 and consumed by
// 0x415880.  The record is now directly proven as x/y/z + one-shot trigger.
struct LegacyAuxiliaryEffectRecordContract {
    float x;
    float y;
    float z;
    std::uint32_t trigger;
};
static_assert(sizeof(LegacyAuxiliaryEffectRecordContract)==0x10);

struct LegacyAuxiliaryEffectTrace {
    std::uint32_t initAddress=0x004156A0u;
    std::uint32_t spawnAddress=0x004156C0u;
    std::uint32_t updateRenderAddress=0x00415880u;
    std::uint32_t baseAddress=0x0257DB00u;
    int slotCount=6;
    int stride=0x10;
    float inactiveY=0.5f;       // set to every slot by 0x4156A0
    float spawnYOffset=0.01f;   // 0x43B330 added by 0x415804
    float activeYLimit=0.11f;   // 0x43B3A0
    float oneShotYThreshold=0.03f; // 0x43B304, slots 0..4
    float slot0To4RisePerMs=5.0e-5f; // 0x43B37C
    float slot5RisePerMs=2.5e-5f;    // 0x43B444
};
inline constexpr LegacyAuxiliaryEffectTrace kLegacyAuxiliaryEffects{};
// r339 DIRECT EXE 0x4159C2..0x4159E4 / 0x415A63..0x415A85:
// every active plaque is X-rotated to face the current camera in the Y/Z
// plane before its texture-7 submit. 0x43B448 is -pi/2.
inline float legacyAuxiliaryPitch(float cameraY,float cameraZ,float y,float z){
    return -1.5707963705062866f-std::atan((cameraY-y)/(cameraZ-z));
}


// DIRECT EXE r128: the five voiced auxiliary slots have exact semantic IDs.
// 0x415880 dispatches these spatial one-shots after y>0.03, while slot 5 is
// the separate timeout presentation and has no one-shot speech branch.
struct LegacyAuxiliaryIdentityTrace {
    static constexpr std::array<std::uint32_t,5> logicalSfx{{0x21u,0x24u,0x20u,0x25u,0x26u}};
    static constexpr std::array<std::string_view,5> fourcc{{"bonu","time","life","slow","acce"}};
    static constexpr std::array<float,5> scalar{{1.2f,1.1f,1.4f,1.0f,1.0f}};
    static constexpr std::array<std::string_view,6> semantic{{
        "score-bonus","time-bonus","extra-life","enemy-slow","xonix-accelerate","timeout"
    }};
    static constexpr std::uintptr_t pickupDispatcher=0x004156C0u;
    static constexpr std::uintptr_t pickupCaller=0x00417BFAu;
    static constexpr std::uintptr_t modelBuilder=0x00422240u;
};
inline constexpr LegacyAuxiliaryIdentityTrace kLegacyAuxiliaryIdentity{};
// r49 DIRECT EXE: direct dispatcher id=5 is not pickup P5.  In the main
// gameplay loop 0x4196F6 subtracts dt from the timer; when the timer becomes
// negative, 0x419795 pushes id=5 and 0x419797 calls 0x4156C0.  That path
// reaches 0x415804 and writes auxiliary slot 5 without changing a gameplay
// stat, then 0x41979F clamps the timer to zero and sets timeout-state flags.
// Pickup index 5 remains the separate random-effect path through dispatcher id=6.
struct LegacyTimeoutAuxiliaryTrace {
    std::uint32_t timerSubtract=0x004196F6u;
    std::uint32_t negativeTimerCompare=0x00419760u;
    std::uint32_t directId5Push=0x00419795u;
    std::uint32_t directId5Call=0x00419797u;
    std::uint32_t dispatcher=0x004156C0u;
    std::uint32_t slotWrite=0x00415804u;
    std::uint32_t timerClampZero=0x0041979Fu;
    std::uint32_t timerGlobal=0x0257DA14u;
    int auxiliarySlot=5;
    // r156: 0x415880 selects logical texture handle 7 before submitting the
    // timeout mesh. Its UVs map to the TOU2 atlas-7 strip at y=144; the
    // original 256x48 payload reads "ВРЕМЯ ВЫШЛО!".
    int textureHandle=7;
    std::string_view resource="TOU2";
    int atlas=7;
    int atlasX=0, atlasY=144, sourceWidth=256, sourceHeight=48;
    float meshU0=0.501953125f, meshU1=0.998046875f;
    float meshV0=0.564453125f, meshV1=0.748046875f;
};
inline constexpr LegacyTimeoutAuxiliaryTrace kLegacyTimeoutAuxiliary{};




// Direct r37 D3D7 caller-state trace. These wrappers are outside the billboard
// submitter itself: 0x40E120 only projects/emits/submits quads. State ownership
// belongs to the surrounding render path.
struct LegacyD3D7RenderStateTrace {
    std::uint32_t alphaBlendWrapper=0x00405A60u; // SetRenderState(0x1B,value)
    std::uint32_t zFuncWrapper=0x00405A20u;      // SetRenderState(0x17,value)
    std::uint32_t textureHandleWrapper=0x004059F0u;
    int alphaBlendEnableState=0x1B;
    int zFuncState=0x17;
    int zFuncLessEqual=4;
    int zFuncAlways=8;
};
inline constexpr LegacyD3D7RenderStateTrace kLegacyD3D7RenderState{};

// r47 direct EXE texture-slot table. 0x405690 creates nine legacy D3D texture
// surfaces from the static width/height/flags table at 0x43F040; 0x4059F0
// selects a slot through the D3D texture-handle table at 0x4505B4.  This proves
// that logical handle 0 is a 64x64 surface. r139 later closed gameplay/theme
// ownership through the stored {slot1,slot0,slot2,usage} upload records; this
// table remains the low-level surface-size/handle contract only.
struct LegacyTextureSlotTableTrace {
    std::uint32_t createRoutine=0x00405690u;
    std::uint32_t staticTableAddress=0x0043F040u;
    std::uint32_t handleTableAddress=0x004505B4u;
    std::uint32_t selectRoutine=0x004059F0u;
    int slotCount=9;
    std::array<int,9> width{{64,64,64,256,256,256,128,256,256}};
    std::array<int,9> height{{64,64,64,256,256,256,128,256,256}};
    std::array<int,9> flags{{1,1,1,0,0,0,0,0,0}};
};
inline constexpr LegacyTextureSlotTableTrace kLegacyTextureSlots{};

// r38 direct caller-order trace around 0x4146E1..0x414797.
// This is intentionally per-callsite: it must not be generalized to every
// 0x40E120 caller until all callsites are traced.
struct LegacyBillboardCallerOrderTrace {
    std::uint32_t setupAddress=0x004146E1u;
    std::uint32_t billboardSetupCall=0x00414709u;
    std::uint32_t billboardSubmitCall=0x0041477Au;
    std::uint32_t alphaEnableCall=0x00414784u;
    std::uint32_t texture6Call=0x0041478Bu;
    std::uint32_t followingLayerCall=0x00414790u;
    std::uint32_t alphaDisableCall=0x00414797u;
    int followingTextureHandle=6;
};
inline constexpr LegacyBillboardCallerOrderTrace kLegacyBillboardCallerOrder{};

// r39 exhaustive direct xref inventory for every 0x40E120 submit call in the
// original executable. baseAddress==0 denotes a register/dynamic base. Counts
// are exact where constant; dynamicCount marks the EBX remainder path.
struct LegacyBillboardSubmitCallsite {
    std::uint32_t setupCall;
    std::uint32_t submitCall;
    std::uint32_t baseAddress;
    int count;
    bool dynamicBase;
    bool dynamicCount;
};

inline constexpr std::array<LegacyBillboardSubmitCallsite,9> kLegacyBillboardSubmitCallsites{{
    {0x00414709u,0x0041477Au,0x0257E8D8u,0x80,false,false},
    {0x004152CAu,0x0041532Du,0x0257E8D8u,0x80,false,false},
    {0x004174DFu,0x0041753Du,0u,          0x78,true, false},
    {0x004174DFu,0x0041759Eu,0u,          0,   true, true },
    {0x00417738u,0x004177B0u,0x0254CFE8u,0x60,false,false},
    {0x0041817Du,0x004181DAu,0u,          0x80,true, false},
    {0x00418315u,0x00418324u,0x0254A2E8u,0xA0,false,false},
    {0x00418359u,0x00418368u,0x0254B1E8u,0xA0,false,false},
    {0x0041839Du,0x004183ACu,0x0254C0E8u,0xA0,false,false},
}};


// DIRECT EXE r55. Shared particle/debris renderers inherit texture 3 with
// alpha blending disabled in the normal gameplay/inter-level/death callers.
// This remains a caller-owned state contract; 0x40E120 itself does not set it.
struct LegacySharedBillboardInheritedStateTrace {
    std::uint32_t gameplayTexture3=0x0041A5F6u;
    std::uint32_t gameplayAlphaOff=0x0041A5ECu;
    std::uint32_t gameplaySharedDebrisCall=0x0041A602u;
    std::uint32_t gameplayEraserCall=0x0041A830u;
    std::uint32_t interLevelTexture3=0x0041B14Du;
    std::uint32_t interLevelAlphaOff=0x0041B143u;
    std::uint32_t interLevelSharedDebrisCall=0x0041B160u;
    std::uint32_t interLevelEraserCall=0x0041B175u;
    std::uint32_t deathTexture3=0x0041CD50u;
    std::uint32_t deathAlphaOff=0x0041CD46u;
    std::uint32_t deathSharedDebrisCall=0x0041CDAEu;
    std::uint32_t deathEraserCall=0x0041CDC0u;
    int textureHandle=3;
    bool alphaBlend=false;
};
inline constexpr LegacySharedBillboardInheritedStateTrace kLegacySharedBillboardInheritedState{};


// DIRECT EXE r56. These were the remaining independent 0x40E120 groups.
// They inherit the same texture-3 / alpha-off state in their proven normal
// callers; none of these routines owns that render state itself.
struct LegacyRemainingBillboardInheritedStateTrace {
    std::uint32_t eraserSubmit=0x0041532Du;
    std::uint32_t eraserRoutine=0x00414F30u;
    std::uint32_t sharedChunkSubmit=0x0041753Du;
    std::uint32_t sharedRemainderSubmit=0x0041759Eu;
    std::uint32_t sharedRoutine=0x00417470u;
    std::uint32_t pickupSmashSubmit=0x004181DAu;
    std::uint32_t pickupSmashRoutine=0x00418110u;
    std::uint32_t gameplaySharedCall=0x0041A5FCu;
    std::uint32_t gameplayPickupSmashRenderCall=0x0041A608u;
    std::uint32_t gameplayEraserCall=0x0041A830u;
    std::uint32_t interLevelPickupSmashRenderCall=0x0041B169u;
    std::uint32_t interLevelEraserCall=0x0041B175u;
    std::uint32_t deathSharedCall=0x0041CDA8u;
    std::uint32_t deathPickupSmashRenderCall=0x0041CDB4u;
    std::uint32_t deathEraserCall=0x0041CDC0u;
    int inheritedTextureHandle=3;
    bool inheritedAlphaBlend=false;
};
inline constexpr LegacyRemainingBillboardInheritedStateTrace kLegacyRemainingBillboardInheritedState{};

// r49 CORRECTION to r39: direct xrefs show 0x418200/0x418290 belong to the
// 0x41C030 death presentation, not the 0x41B2A0 final sequence. This is a
// three-colour death-scene particle effect. The three 160-record groups are contiguous windows of one
// 480-record x/y/z/vx/vy/vz array. 0x418290 renders each window separately.
struct LegacyDeathTriColorBillboardArrayTrace {
    std::uint32_t initAddress=0x00418200u;
    std::uint32_t renderAddress=0x00418290u;
    std::uint32_t deathRoutine=0x0041C030u;
    std::uint32_t initCallsite=0x0041C40Eu;
    std::uint32_t renderCallsite=0x0041CD61u;
    std::uint32_t inheritedAlphaDisableCall=0x0041CD46u;
    std::uint32_t inheritedTexture3Call=0x0041CD50u;
    int inheritedTextureHandle=3;
    bool inheritedAlphaBlend=false;
    std::uint32_t baseAddress=0x0254A2E8u;
    int recordStride=0x18;
    int totalRecords=480;
    int groupCount=3;
    int recordsPerGroup=160;
    std::array<std::uint32_t,3> groupBase{{0x0254A2E8u,0x0254B1E8u,0x0254C0E8u}};
    std::array<std::uint32_t,3> diffuse{{0x005FDF5Fu,0x00FF7F7Fu,0x00DFDFDFu}};
    std::uint32_t sourceXAddress=0x0257DA48u;
    std::uint32_t sourceYAddress=0x0257DA4Cu;
    std::uint32_t sourceZAddress=0x0257DA50u;
    float horizontalVelocityScale=4.0e-7f;
    float verticalVelocityScale=5.0e-7f;
    float gravityPerMs=1.5e-7f;
    float billboardU0=0.939453125f;
    float billboardV0=0.126953125f;
    float billboardUvSpan=0.05859375f;
    float billboardSizeScale=0.000531250028871f;
    int horizontalRandMask=0xFF;
    int horizontalRandBias=0x80;
    int verticalRandMask=0x7F;
};
inline constexpr LegacyDeathTriColorBillboardArrayTrace kLegacyDeathTriColorBillboards{};
// Compatibility alias for snapshots before r90. The old "ambient" name was
// disproved by the sole xrefs into 0x41C030 and must not be used semantically.
inline constexpr auto kLegacyAmbientBillboards=kLegacyDeathTriColorBillboards;
