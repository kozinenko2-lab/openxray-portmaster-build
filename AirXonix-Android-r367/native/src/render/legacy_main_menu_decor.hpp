#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include "render/legacy_transform.hpp"

// Direct transcription anchors for AirXonix.wrp.exe main-menu 3-D decoration.
// No semantic names are assigned to the seven 0x4225A0 models beyond slot0..6.
struct LegacyMainMenuDecorationTrace {
    std::uint32_t titleRoutine=0x00411EF0u;
    std::uint32_t sceneRoutine=0x00412150u;
    std::uint32_t modelBuilder=0x004225A0u;
    std::uint32_t logoMaster=0x02585A60u, logoPrepared=0x0257F5C4u;
    std::uint32_t laxyMaster=0x02585AD8u, laxyPrepared=0x025B5B24u;
    std::uint32_t decorMasterBase=0x02585A74u, decorPreparedBase=0x02583718u;
    int logoTextureSlot=8, laxyTextureSlot=2, decorTextureSlot=3, reflectionTextureSlot=6;
    int conditionalTexture7Slot=7;

    // r209 DIRECT EXE 0x405AB0 + whole-.text SetRenderState census and
    // caller 0x413338..0x4133B8: the title is entered with ZFUNC=LESSEQUAL.
    // The game never writes D3DRENDERSTATE_ZWRITEENABLE (state 14), so the
    // 0x411EF0 LOGO/LAXY layer keeps the default depth-write state enabled.
    // Do not reintroduce the old GLES-only glDepthMask(GL_FALSE) workaround.
    static constexpr bool titleNormalDepth=true;
    static constexpr bool titleDepthWriteEnabled=true;
    // r349 DIRECT EXE render-state census: 0x411EF0 never enables D3D
    // ALPHATESTENABLE and the RGB565 LOGO resource has no source alpha. Black
    // texels are opaque parts of the rotating coin, not holes through it. The
    // old GLES alpha-test discarded 13,212 black LOGO texels and made the
    // object appear to deform/flip as its far face became visible.
    static constexpr bool titleAlphaTestEnabled=false;
    static constexpr std::uint32_t titleCameraSelectorGlobal=0x02545898u;
    static constexpr std::uint32_t titleCameraSelectorInit=0u; // 0x4147BC

    // r224 DIRECT EXE 0x413338..0x413351 -> 0x411D60. The caller selects
    // D3DCMP_ALWAYS through 0x405A20(0), 0x411D60 submits both prepared
    // menu-background objects and then 0x405E40, and only after return is
    // D3DCMP_LESSEQUAL restored. ZWRITE remains at its normal enabled state.
    static constexpr bool backgroundDepthAlways=true;
    static constexpr bool backgroundDepthWriteEnabled=true;
    static constexpr std::uint32_t backgroundFrameSubmit=0x00405E40u;

    // r341 DIRECT EXE 0x413338..0x413653: after 0x411D60 returns, the caller
    // restores D3DCMP_LESSEQUAL once at 0x41334A and never disables Z before
    // the five M1 item submissions at 0x4135F3/0x413618 -> 0x40C350. There is
    // also no ZWRITEENABLE owner in .text. The GLES path must therefore keep
    // both depth testing and depth writes enabled for the animated menu items.
    static constexpr bool menuItemsDepthTestEnabled=true;
    static constexpr bool menuItemsDepthWriteEnabled=true;

    static constexpr bool slot5ReferencedByScene=false;
    static constexpr bool slot6ReferencedByScene=true;
    static constexpr int slot6RotateX=-0x200;
    static constexpr float slot6WaveXScale=0.003000000026077032f;
    static constexpr float slot6Y=-0.12300000339746475f;
    static constexpr float slot6Z=-0.28999999165534973f;
    // r183 caller tail 0x4132B7..0x413444. After 0x412150 restores
    // logical texture 3, the caller draws two scaled airborne-enemy subtype-0
    // selectors around the currently selected M1 entry.
    static constexpr float selectorX=0.008999999612569809f;
    static constexpr float selectorScale=0.20000000298023224f;
    static constexpr float selectorFirstZ=-0.001500000013038516f;
    static constexpr float selectorZStep=-0.003000000026077032f;
    static constexpr int selectorLegacyUnitsPerMs=2;
    static float selectorZForIndex(int index){ return selectorFirstZ + selectorZStep*float(index); }

    // r177 direct 0x4122C1..0x412719 submission ledger. These counts/offsets
    // intentionally describe draw ownership rather than semantic model names.
    static constexpr int slot0BaseDrawCount=5;
    static constexpr int slot0ReflectionCount=5;
    static constexpr int slot0FirstZRotation=-0x100;
    static constexpr int slot0SecondZRotation=+0x100;
    static constexpr float slot0RadiusA=-0.20000000298023224f;
    static constexpr float slot0RadiusB=+0.20000000298023224f;
    static constexpr float slot0RadiusC=+0.014999999664723873f;
    static constexpr int slot1PitchOffset=-0x200;
    static constexpr float slot1Radius=-0.10000000149011612f;
    // r180 direct register-liveness trace: 0x412A17/0x412A20 reuse ESI/EDI
    // last assigned by slot1 at 0x41254F/0x41255A. The airborne subtype-2
    // decoration therefore shares slot1's -0.1 yaw orbit; it is not -0.2.
    static constexpr float airborneSubtype2Radius=slot1Radius;

    // r181 direct 0x412A2A..0x412BE1 crawler/menu pair. The first crawler is
    // not an origin-centred 0.07 circle and has no -0.2 Z bias. Its world
    // position is composed from the shared yaw/pitch basis plus the slot3
    // 0.11 yaw orbit that remains live in the original stack temporaries.
    static constexpr float crawlerPrimaryOrbitRadius=0.10999999940395355f;
    static constexpr float crawlerPrimaryLiftRadius=0.07000000029802322f;
    static constexpr float crawlerPrimaryReflection=0.30000001192092896f;
    static constexpr float crawlerSecondaryReflection=0.5f;

    struct CrawlerPrimaryPosition { float x=0.f,y=0.f,z=0.f; };
    static CrawlerPrimaryPosition crawlerPrimaryPosition(float pitchWave,float yawWave){
        const float cp=std::cos(pitchWave), sp=std::sin(pitchWave);
        const float cy=std::cos(yawWave), sy=std::sin(yawWave);
        return {crawlerPrimaryOrbitRadius*cy - crawlerPrimaryLiftRadius*sp*sy,
                crawlerPrimaryLiftRadius*cp,
                crawlerPrimaryOrbitRadius*sy + crawlerPrimaryLiftRadius*sp*cy};
    }
    static constexpr int slot2BaseDrawCount=2;
    static constexpr int slot2ReflectionCount=2;
    static constexpr float slot2RadiusA=-0.01600000075995922f;
    static constexpr float slot2RadiusB=+0.04600000008940697f;
    static constexpr float slot3Radius=+0.10999999940395355f;

    // r178 direct 0x41271E..0x412981 slot4 orbit. The EXE does not use an
    // origin-centered circle. It constructs the two local points around
    // (0.11, 0.05, 0) with radius 0.04, phase-shifted by 1024 legacy units,
    // then applies the shared pitch/yaw wave to the translation.
    static constexpr float slot4CenterX=0.10999999940395355f;
    static constexpr float slot4CenterY=0.05000000074505806f;
    static constexpr float slot4Radius=0.03999999910593033f;
    static constexpr int slot4BaseRotateX=0x200;
    static constexpr int slot4SecondPhaseOffset=0x400;
    static constexpr float slot4Reflection=0.800000011920929f;

    // r179 semantic/material classification from direct atlas reconstruction.
    // Slots 0..4 all sample atlas3 U=32.5..39.5, V=24.5..31.5: this is the
    // lower-left 8x8 band of the XONI tile placed at atlas3 (32,0). They are
    // XONI-material menu decoration primitives, not unidentified enemies.
    static constexpr int xoniDecorAtlasX0=32;
    static constexpr int xoniDecorAtlasY0=24;
    static constexpr int xoniDecorAtlasX1=40;
    static constexpr int xoniDecorAtlasY1=32;
    // Slot6 is submitted only after logical texture 7 is selected. Its plane
    // UVs cover the first atlas7 tile, IN2$, whose original payload is "$1000".
    static constexpr const char* slot6Resource="IN2$";

    struct OrbitXZ { float x=0.f,z=0.f; };
    static OrbitXZ yawOrbit(float yawWave,float radius){
        return {std::sin(yawWave)*radius,std::cos(yawWave)*radius};
    }

    struct Slot4Position { float x=0.f,y=0.f,z=0.f; };
    static Slot4Position slot4Position(int angle2048,float pitchWave,float yawWave,bool second){
        const float a=float((angle2048 + (second?slot4SecondPhaseOffset:0)) & 0x7ff) * (twoPi/2048.f);
        const float lx=slot4CenterX + slot4Radius*std::sin(a);
        const float ly=slot4CenterY;
        const float lz=slot4Radius*std::cos(a);
        const float cp=std::cos(pitchWave), sp=std::sin(pitchWave);
        const float cy=std::cos(yawWave), sy=std::sin(yawWave);
        const float y=ly*cp-lz*sp;
        const float z1=ly*sp+lz*cp;
        return {lx*cy-z1*sy, y, z1*cy+lx*sy};
    }

    static constexpr float twoPi=6.2831853071795864769f;
    static constexpr float angleToLegacy=325.949310302734375f;
    static float wrap(float v){ while(v>=twoPi)v-=twoPi; while(v<0.f)v+=twoPi; return v; }
    static int legacyAngle(float radians){ return static_cast<int>(radians*angleToLegacy); }

    // 0x411D60 + 0x411EF0 startup/title state.  These are process-lifetime
    // globals in the original EXE: 0x440D14 starts at -0.5, 0x440D10 at
    // 0.06, while 0x0254593C/40/44 are zero-initialised.  Nothing in the
    // main-menu dispatcher restores them when M1 is entered again.
    struct TitleState { float drop=0.06f, reveal=0.f, phase=0.f; int waitMs=0; bool chimeArmed=true; };
    // r198 startup constants recovered directly from AirXonix.wrp.exe.
    static constexpr int startupBlackPreRollMs=0x320; // 0x424D0E Sleep(800)
    static constexpr int titleHoldMs=0x5dc;            // 0x411F3B cmp eax,1500
    static constexpr float titleDropInitial=0.06f;
    static constexpr float titleDropPerMs=0.00005f;
    static constexpr float titleRevealPerMs=0.001f;
    static constexpr float titlePhasePerMs=0.001f;
    static constexpr bool resetTitleStateOnMainMenuReentry=false;
    static constexpr float entryInitial=-0.5f;
    static constexpr float entryRatePerMs=0.00039999998989515007f;
    static float advanceEntryPhase(float phase,const TitleState& titleBeforeFrame,int dt){
        // 0x411D60 runs BEFORE 0x411EF0 each frame.  The menu-entry camera is
        // frozen at -0.5 while 0x0254593C (title reveal) is exactly zero.
        if(titleBeforeFrame.reveal!=0.f)
            phase=std::min(0.f,phase+float(std::max(0,dt))*entryRatePerMs);
        return phase;
    }
    static bool introOnly(float entryPhase){ return entryPhase<=entryInitial; }
    static void advanceTitle(TitleState& s,int dt){
        const float fdt=float(std::max(0,dt));
        s.drop=std::max(0.f,s.drop-fdt*titleDropPerMs);
        if(s.drop==0.f){ s.waitMs+=dt; if(s.waitMs>titleHoldMs){s.chimeArmed=false;s.reveal=std::min(1.f,s.reveal+fdt*titleRevealPerMs);} }
        s.phase=wrap(s.phase+fdt*titlePhasePerMs);
    }
    // r194 DIRECT EXE 0x411FA4..0x412096: [F+4]=reveal*-.0083 is pushed as
    // the X argument of 0x40C350 and [F+0]=reveal*-.0058 as Y. r166 swapped
    // them, which parked the finished LOGO coin below the bottom edge instead
    // of the lower-left corner opposite LAXY.
    static float logoTranslateX(const TitleState&s){return s.reveal*-0.008299999870359898f;}
    static float logoTranslateY(const TitleState&s){return s.reveal*-0.005799999926239252f;}
    static float logoTranslateZ(const TitleState&s){return s.drop+0.01f;}
    static float logoScale(const TitleState&s){return 1.f-s.reveal*0.7f;}
    static float logoYaw(const TitleState&s){return (1.f-s.reveal)*(s.phase+1.5707963267948966f+1.5f*std::cos(s.phase));}
    static float laxyYaw(const TitleState&s){return (1.f-s.reveal)*(0.1f*std::sin(2.f*s.phase));}
    static float laxyX(){return 0.008700000122189522f;}
    static float laxyY(){return -0.006200000178068876f;}
    static float laxyZ(const TitleState&s){return 0.01f+(1.f-s.reveal)*0.05f;}

    // 0x412150 state. A/B/C are the exact three wrapped radian accumulators.
    struct SceneState { float a=0.f,b=0.f,c=0.f; int angle2048=0; int timer=0; };
    struct Derived { float amp=0.f; float yawWave=0.f; float pitchWave=0.f; };
    static Derived advanceScene(SceneState& s,int dt){
        const float fdt=float(std::max(0,dt));
        s.a=wrap(s.a+fdt*0.001f); s.b=wrap(s.b+fdt*0.0015f); s.c=wrap(s.c+fdt*0.0002f);
        s.angle2048=(s.angle2048+dt)&0x7ff; s.timer+=dt;
        const float amp=(std::cos(s.c)+1.f)*0.25f;
        return {amp,2.f*amp*std::cos(s.a),amp*std::sin(s.b)};
    }
    static float sceneCameraX(){return 0.f;}
    static float sceneCameraY(){return -0.12999999523162842f;}
    static float sceneCameraZ(float menuEntryPhase){return menuEntryPhase-0.30000001192092896f;}
    static float orbitX(float theta){return std::cos(theta)*0.14000000059604645f;}
    static float orbitY(){return -0.15000000596046448f;}
    static float orbitZ(float theta){return std::sin(theta)*0.07000000029802322f-0.15000000596046448f;}

    // r182: exact 0x412C04..0x412EDE four-pickup block.
    static constexpr std::array<int,4> menuPickupTypes{{5,1,3,2}};
    static constexpr std::array<float,4> menuPickupPhaseOffsets{{0.f,1.5707963267948966f,4.71238898038469f,3.141592653589793f}};
    static constexpr std::array<float,4> menuPickupReflections{{0.95f,0.99f,0.35f,0.15f}};
    static constexpr float menuPickupDirectional=0.5f;
    static constexpr float menuPickupAmbient=0.2f;
    static constexpr float menuPostPickupDirectional=0.7f;
    static constexpr float menuPostPickupAmbient=0.3f;
    static constexpr int pickup5TiltX=500;

    // DIRECT EXE 0x412C04..0x412E4A: QUESTION/TIME/SLOW are submitted
    // under one shared matrix.  The matrix is built once as Z(spin) then
    // X(500) and is deliberately NOT reset for slots 1 and 2.  Only the LIFE
    // heart later receives its own identity + Y(spin) matrix.  Earlier native
    // revisions invented private TIME/SLOW yaw/pitch transforms, making the
    // two discs visibly precess instead of rotating as one menu group.
    static LegacyTransform::Matrix34 menuPickupOrientation(std::size_t slot,int ownSpin){
        LegacyTransform::Matrix34 m=LegacyTransform::identity();
        if(slot<3){
            LegacyTransform::rotateZ(m,ownSpin);
            LegacyTransform::rotateX(m,pickup5TiltX);
        }else if(slot==3){
            LegacyTransform::rotateY(m,ownSpin);
        }
        return m;
    }
};
inline constexpr LegacyMainMenuDecorationTrace kLegacyMainMenuDecorationTrace{};
