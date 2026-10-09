#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

// DIRECT EXE r250: 0x4053B0 builds one shared seven-direction template;
// 0x4055C0 stores the current world-space camera/reference point and 0x4055E0
// turns that template into a small camera-oriented additive fan at (x,y,z).
// The prepared object has eight slots, but slot 0 is not referenced by its
// topology. 0x4055E0 fills slots 1..7; slot 7 is the bright centre.
namespace LegacyDirectionalGlint {

struct Vec3 { float x=0.f,y=0.f,z=0.f; };
struct Vertex {
    float x=0.f,y=0.f,z=0.f;
    float u=0.96875f,v=0.15625f;
    float light=0.f;
};

struct Trace {
    static constexpr std::uint32_t templateBuilder=0x004053B0u;
    static constexpr std::uint32_t referenceSetter=0x004055C0u;
    static constexpr std::uint32_t submitHelper=0x004055E0u;
    static constexpr std::uint32_t xonixSubmit=0x00420AE0u;
    static constexpr float templateAzimuth=3.9269909858703613f; // 5*pi/4
    static constexpr float templateElevation=0.7853981852531433f; // pi/4
    static constexpr float azimuthWide=0.5235987901687622f; // pi/6
    static constexpr float azimuthNarrow=0.1745329350233078f; // pi/18
    static constexpr float elevationSpread=0.3490658700466156f; // pi/9
    static constexpr float u=0.96875f;
    static constexpr float v=0.15625f;
    static constexpr float xonixSize=0.0019000000320374966f;
    static constexpr float airborneSize=0.005499999970197678f;
    static constexpr float airborneY=0.004999999888241291f;
    static constexpr float crawlerSize=0.0035000001080334187f;
    static constexpr float crawlerYOffset=0.013000000268220901f;
};

// Prepared topology written at 0x4054CC..0x405578. The original offsets are
// 44-byte transformed-workspace offsets: 0x134/0x2c/... => indices 7/1/...
// Since this native helper exposes only the seven populated source vertices,
// prepared indices 1..7 map to native indices 0..6.
inline constexpr std::array<std::array<std::uint8_t,3>,6> Fan{{
    {{6,0,5}}, {{6,1,0}}, {{6,2,1}},
    {{6,3,2}}, {{6,4,3}}, {{6,5,4}}
}};

inline Vec3 spherical(float azimuth,float elevation){
    const float ce=std::cos(elevation);
    return {std::cos(azimuth)*ce,std::sin(elevation),std::sin(azimuth)*ce};
}

inline std::array<Vec3,7> sourceDirections(){
    constexpr float a=Trace::templateAzimuth;
    constexpr float b=Trace::templateElevation;
    constexpr float wide=Trace::azimuthWide;
    constexpr float narrow=Trace::azimuthNarrow;
    constexpr float spread=Trace::elevationSpread;
    return {{
        spherical(a+wide,b),
        spherical(a+wide-narrow,b+spread),
        spherical(a-wide+narrow,b+spread),
        spherical(a-wide,b),
        spherical(a-narrow,b-spread),
        spherical(a+narrow,b-spread),
        spherical(a,b)
    }};
}

inline Vec3 normalized(Vec3 p){
    const float d=std::sqrt(p.x*p.x+p.y*p.y+p.z*p.z);
    if(d==0.f)return {};
    const float inv=1.f/d;
    return {p.x*inv,p.y*inv,p.z*inv};
}

// Literal high-level equivalent of 0x4055E0. The helper forms the normalized
// camera-to-target vector, adds it to each unit template direction, normalizes
// again, scales by the caller's size and finally submits the prepared object
// translated to the target world position.
inline std::array<Vertex,7> build(float referenceX,float referenceY,float referenceZ,
                                  float targetX,float targetY,float targetZ,float size){
    const Vec3 view=normalized({referenceX-targetX,referenceY-targetY,referenceZ-targetZ});
    const auto source=sourceDirections();
    std::array<Vertex,7> out{};
    for(std::size_t i=0;i<source.size();++i){
        const Vec3 h=normalized({source[i].x+view.x,source[i].y+view.y,source[i].z+view.z});
        out[i].x=targetX+h.x*size;
        out[i].y=targetY+h.y*size;
        out[i].z=targetZ+h.z*size;
        out[i].u=Trace::u; out[i].v=Trace::v;
        out[i].light=(i==6u)?1.f:0.f;
    }
    return out;
}

} // namespace LegacyDirectionalGlint
