#pragma once
#include <cstdint>
namespace LegacyGameOver {
struct Trace {
    static constexpr std::uint32_t cinematicBuilder=0x004223E0u;
    static constexpr int cinematicSlot=3;
    static constexpr std::uint32_t modelPointer=0x0257F5ACu;
    static constexpr std::uint32_t preparedPointer=0x0257F5D8u;
    static constexpr std::uint32_t textureSelect=0x0041CA48u;
    static constexpr int textureSlot=4;
    static constexpr std::uint32_t submit=0x0041CA92u;
    static constexpr float submitX=0.0f;
    static constexpr float submitZ=0.003000000026077032f;
    static constexpr float targetX=0.5015625357627869f;
    static constexpr float driftScale=0.000699999975040555f;

    // DIRECT EXE r204: 0x41CA63..0x41CA6D calls 0x40C250 with a dedicated
    // presentation camera before GOVE is transformed/submitted. The six
    // arguments are exactly (0,0,0, 0,-510,0). GOVE therefore must not use
    // the live death/gameplay camera that remains active for the world behind it.
    static constexpr float cameraX=0.0f;
    static constexpr float cameraY=0.0f;
    static constexpr float cameraZ=0.0f;
    static constexpr int cameraAngle1=0;
    static constexpr int cameraAngle2=-510;
    static constexpr int cameraAngle3=0;
};
}
