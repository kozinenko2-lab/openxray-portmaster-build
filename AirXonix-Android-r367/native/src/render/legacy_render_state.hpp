#pragma once

// Literal D3D7 render-state contract recovered in r73.
// This deliberately describes the legacy state machine without depending on
// GLES headers so it can be regression-tested in the headless build.
namespace LegacyRenderState {

enum class DepthFunction {
    LessEqual,
    Always
};

struct State {
    bool depthTestEnabled = true;
    bool depthWriteEnabled = true;   // no dynamic D3D ZWRITEENABLE writes found
    DepthFunction depthFunction = DepthFunction::LessEqual;
    bool alphaBlendEnabled = false;
    bool srcBlendOne = true;
    bool dstBlendOne = true;
};

constexpr State Initial{};

constexpr State setAlpha(State s,bool enabled){s.alphaBlendEnabled=enabled;return s;}
constexpr State setNormalDepth(State s,bool normal){s.depthFunction=normal?DepthFunction::LessEqual:DepthFunction::Always;return s;}

// 0x40FF29..0x40FF3E representative reflection caller:
// alpha ON -> texture 6 / 1111 -> 0x40E960 -> alpha OFF.
constexpr State ReflectionPass = setAlpha(Initial,true);

static_assert(Initial.depthTestEnabled);
static_assert(Initial.depthWriteEnabled);
static_assert(!Initial.alphaBlendEnabled);
static_assert(Initial.srcBlendOne && Initial.dstBlendOne);
static_assert(ReflectionPass.alphaBlendEnabled);

} // namespace LegacyRenderState
