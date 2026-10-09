#pragma once
#include <array>
#include <cstdint>

// r223 direct EXE reconstruction of 0x405C30 + 0x405E40.
// 0x405C30 prepares eight 32-byte D3DTLVERTEX records. 0x405E40 uploads all
// eight and calls IDirect3DDevice7::DrawIndexedPrimitiveVB with TRIANGLELIST,
// 24 16-bit indices. It is the black two-pixel screen/backbuffer frame used by
// both the gameplay field caller and the main-menu background caller.
namespace LegacyScreenFrame {
inline constexpr std::uint32_t Builder = 0x00405C30u;
inline constexpr std::uint32_t Submit = 0x00405E40u;
inline constexpr float InsetPixels = 2.0f;
inline constexpr float ScreenDepth = 0.020999999716877937f; // bits 0x3CAC0831
inline constexpr float Rhw = 47.619049072265625f;           // bits 0x423E79E8

struct TlVertex {
    float sx{},sy{},sz{},rhw{};
    std::uint32_t diffuse{},specular{};
    float u{},v{};
};

inline std::array<TlVertex,8> vertices(int width,int height){
    const float w=static_cast<float>(width),h=static_cast<float>(height);
    const float wi=w-InsetPixels,hi=h-InsetPixels;
    const auto make=[](float x,float y){
        return TlVertex{x,y,ScreenDepth,Rhw,0u,0u,0.f,0.f};
    };
    return {{
        make(0.f,0.f), make(InsetPixels,InsetPixels),
        make(w,0.f),   make(wi,InsetPixels),
        make(w,h),     make(wi,hi),
        make(0.f,h),   make(InsetPixels,hi)
    }};
}

inline constexpr std::array<std::uint16_t,24> Indices{{
    1,0,3, 3,0,2,
    5,3,2, 5,2,4,
    6,5,4, 6,7,5,
    6,0,7, 7,0,1
}};
}
