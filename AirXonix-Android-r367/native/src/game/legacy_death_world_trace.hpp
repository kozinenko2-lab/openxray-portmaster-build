#pragma once
#include <cstdint>

// Direct AirXonix.wrp.exe transcription, r100.
// These constants describe the original death-world frame and transition reset.
struct LegacyCrawlerTransitionResetTrace {
    std::uint32_t routine = 0x00416890u;
    std::uint32_t countAddress = 0x0254A2D8u;
    std::uint32_t recordBase = 0x0254D8E8u;
    int recordStride = 0x1C;
    int gridXOffset = 0x00;
    int gridYOffset = 0x04;
    int respawnDelayOffset = 0x10;
    float resetGridX = 32.0f;
    float resetGridY = 65.0f;
    float firstDelay = 0.10000000149f;
    float delayStep = 0.20000000298f;
};
inline constexpr LegacyCrawlerTransitionResetTrace kLegacyCrawlerTransitionReset{};

struct LegacyDeathWorldFrameTrace {
    std::uint32_t begin = 0x0041C7D9u;
    std::uint32_t endPresentCall = 0x0041CE22u;

    std::uint32_t cameraXAddress = 0x0257DA58u;
    std::uint32_t cameraYAddress = 0x0257DA5Cu;
    std::uint32_t cameraZAddress = 0x0257DA60u;
    float cameraCenter = 0.40000000596f;
    float cameraTrackScale = 0.5f;
    float cameraXBias = 0.44999998808f;
    float cameraZBias = 0.34999999404f;

    std::uint32_t globalEffectsUpdate = 0x004185F0u;
    std::uint32_t airborneUpdate = 0x00416110u;
    std::uint32_t crawlerUpdate = 0x00416950u;
    float crawlerSuppressedDelayAdvancePerMs = 0.0001500000071f;
    std::uint32_t pickupUpdate = 0x004179B0u;
    std::uint32_t cyclicAnimationUpdate = 0x0041FD50u;

    float modelPhasePerMs = 0.0399999991f;
    float modelPhaseClamp = 1.5707963705f;
    std::uint32_t modelPhaseApply = 0x00420BB0u;
    // r101 direct constructor/renderer xref closes the old anonymous-model
    // label: 0x420BB0 rotates/re-lights the first central Xonix body mesh.
    std::uint32_t xonixBodyModelPointer = 0x02583738u;
    std::uint32_t xonixBodyRenderBlob = 0x025849D8u;
    std::uint32_t xonixSecondaryRenderBlob = 0x02585A98u;
    std::uint32_t xonixRotorRenderBlob = 0x025B5B28u;
    float rotorBasePerMs = 0.007000000216f;
    float rotorCuttingAddPerMs = 0.006000000052f;
    float twoPi = 6.2831854820f;

    std::uint32_t cameraBuild = 0x0040C250u;
    std::uint32_t audioListenerPosition = 0x0040A5C0u;
    std::uint32_t frameBegin = 0x00406030u;
    std::uint32_t xonixDraw = 0x004209E0u;
    std::uint32_t auxiliaryEffects = 0x00415880u;
    std::uint32_t reflectionFlush = 0x0040E960u;
    std::uint32_t deathTriColorUpdate = 0x00418290u;
    std::uint32_t crawlerDeathParticles = 0x00417470u;
    std::uint32_t fieldDebris = 0x00417700u;
    std::uint32_t pickupSmash = 0x00418110u;
    std::uint32_t homing = 0x00415490u;
    std::uint32_t eraser = 0x00414F30u;
    std::uint32_t auxiliaryOverlay = 0x0041BEE0u;
    std::uint32_t airborneGroundShadow = 0x00422DE0u;
    std::uint32_t framePresent = 0x004060C0u;

    std::uint32_t capableRendererFlag = 0x0045000Cu;
    int reflectionTexture = 6;
    int particleTexture = 3;
    int shadowTexture = 0;
};
inline constexpr LegacyDeathWorldFrameTrace kLegacyDeathWorldFrame{};
