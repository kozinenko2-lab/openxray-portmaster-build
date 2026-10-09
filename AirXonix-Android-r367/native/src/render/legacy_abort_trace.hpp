#pragma once
#include <cstdint>
namespace LegacyAbort {
struct Trace {
    static constexpr int cinematicSlot=4;
    static constexpr std::uint32_t preparedPointer=0x0257F5DCu;
    static constexpr std::uint32_t textureSelect=0x0041E7A6u;
    static constexpr int textureSlot=4;
    static constexpr std::uint32_t submit=0x0041E7C6u;
    static constexpr float submitX=0.f;
    static constexpr float submitZ=0.f;
    static constexpr float panelYScale=1.100000023841858f;
    // DIRECT EXE 0x41E5CE..0x41E626.
    static constexpr float panelRatePerMs=0.0007999999797903001f; // 0x43B428
    static constexpr float holdY=-0.03f;                         // 0x43B520
    static constexpr float inputThresholdY=-0.03500000014901161f;// 0x43B58C
    static constexpr float offscreenY=-0.25f;                    // 0x43B57C
};
}
