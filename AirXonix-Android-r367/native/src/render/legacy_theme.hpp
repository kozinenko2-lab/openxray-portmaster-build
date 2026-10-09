#pragma once
#include <algorithm>
#include <cmath>
#include <array>
#include <cstddef>
#include <cstdint>

struct LegacyEnvironmentTheme {
    // r139 correction: the table is stored in selector/global order, not logical-slot order.
    const char* slot1; // record +0 -> 0x025B7870 -> logical texture slot 1
    const char* slot0; // record +4 -> 0x025B5B48 -> logical texture slot 0
    const char* slot2; // record +8 -> 0x025B5B4C -> logical texture slot 2
};

// r139 DIRECT EXE correction of the old r48 labels. Exact 12-entry table at
// 0x004418B0, selected by 0x00422FC0. Each 16-byte record is
// {slot1FourCC, slot0FourCC, slot2FourCC, usageCounter}. 0x00423350 proves the
// mapping: 0x025B5B48 is emitted as the first 0x4058F0 record with slot index 0;
// 0x025B7870 is emitted as the next record with slot index 1; 0x025B5B4C goes
// to slot 2. FourCC strings below keep table order and external spelling.
inline constexpr std::array<LegacyEnvironmentTheme,12> kLegacyEnvironmentThemes{{
    {"0034","TST1","0057"},
    {"VOL3","TST2","0212"},
    {"SKY7","TST3","0007"},
    {"VOL9","TST4","0214"},
    {"SKY3","TST8","0040"},
    {"VOL2","0205","0117"},
    {"SK02","0205","0031"},
    {"VOL0","0212","0049"},
    {"SK01","0052","0057"},
    {"VOL4","0106","0217"},
    {"SKY7","0031","0049"},
    {"SK01","TST4","0117"}
}};

// Theme #7 is the exact triplet visible in the supplied reference screenshot.
inline constexpr std::size_t kLegacyReferenceTheme = 7;


struct LegacyEnvironmentThemeTrace {
    std::uint32_t tableAddress=0x004418B0u;
    std::uint32_t selectRoutine=0x00422FC0u;
    std::uint32_t runtimeSlot0Global=0x025B5B48u;
    std::uint32_t runtimeSlot1Global=0x025B7870u;
    std::uint32_t runtimeSlot2Global=0x025B5B4Cu;
    std::uint32_t gameplayConstructor=0x00423350u;
    std::uint32_t uploadRoutine=0x004058F0u;
    int recordStride=16;
    int themeCount=12;
};
inline constexpr LegacyEnvironmentThemeTrace kLegacyEnvironmentThemeTrace{};


// r165 DIRECT EXE: literal 0x00422FC0 selector. After finding the minimum
// use counter, the original does `rand() & 7`, then +1, and scans cyclically
// in the order 1,2,...,11,0 until that many minimum-use records are seen.
// The 12-record resource table below is directly transcribed from the EXE.
struct LegacyEnvironmentThemeSelectorTrace {
    std::uint32_t routine=0x00422FC0u;
    std::uint32_t table=0x004418B0u;
    std::uint32_t firstUsage=0x004418BCu;
    int themeCount=12;
    int recordStride=16;
    static std::size_t select(std::array<std::uint32_t,12>& usage,int randomValue){
        const std::uint32_t minUse=*std::min_element(usage.begin(),usage.end());
        const int wanted=(static_cast<unsigned>(randomValue)&7u)+1;
        int seen=0; std::size_t idx=0;
        for(;;){
            idx=(idx+1u)%12u;
            if(usage[idx]==minUse && ++seen>=wanted){ ++usage[idx]; return idx; }
        }
    }
};
inline constexpr LegacyEnvironmentThemeSelectorTrace kLegacyEnvironmentThemeSelectorTrace{};


struct LegacyMenuEnvironmentTheme {
    const char* slot0; // 64x64 moving/background texture (VOL*)
    const char* slot1; // 64x64 sky/secondary texture (SKY*/SK0*)
};

// r58 DIRECT EXE: exact 8-entry menu/presentation table at 0x00441830,
// selected by 0x00422F40. Each record is {slot1FourCC, slot0FourCC, 0,
// usageCounter}. 0x00423C30 and 0x00423EE0 then upload the selected pair
// into logical texture slots 0 and 1. This is the background source for the
// M1/M2 presentation family; 1111 is a separate slot-6 resource and must not
// be tiled as a menu background.
inline constexpr std::array<LegacyMenuEnvironmentTheme,8> kLegacyMenuEnvironmentThemes{{
    {"VOL4","SKY3"},
    {"VOL2","SK01"},
    {"VOL0","SK01"},
    {"VOL2","SK02"},
    {"VOL9","SK02"},
    {"0052","SKY7"},
    {"0049","SKY7"},
    {"0007","SKY3"}
}};

struct LegacyMenuEnvironmentThemeTrace {
    std::uint32_t tableAddress=0x00441830u;
    std::uint32_t selectRoutine=0x00422F40u;
    std::uint32_t runtimeSlot0Global=0x025B786Cu;
    std::uint32_t runtimeSlot1Global=0x025B5B54u;
    std::uint32_t m1Constructor=0x00423C30u;
    std::uint32_t m2Constructor=0x00423EE0u;
    std::uint32_t animatedBackgroundRoutine=0x00411D60u;
    std::uint32_t recordsTexture1Select=0x0040FA74u;
    int recordStride=16;
    int themeCount=8;
};
inline constexpr LegacyMenuEnvironmentThemeTrace kLegacyMenuEnvironmentThemeTrace{};


// r59 DIRECT EXE: exact animated two-layer menu background state machine in
// 0x00411D60.  The two prepared meshes are selected as logical textures 0/1
// immediately before 0x0040C350.  Keep this independent of renderer policy so
// the legacy update law can be regression-tested without SDL/GLES.
struct LegacyMenuBackgroundAnimationTrace {
    std::uint32_t routine=0x00411D60u;
    std::uint32_t primaryPhaseGlobal=0x02545724u;
    float primaryStepPerMs=0.0003000000142492354f; // 0x0043B2E4
    float phaseWrap=1.0f;
    float primaryDerivedOffset=8.0f;               // 0x0043B354
    std::uint32_t secondaryPhaseGlobal=0x025458BCu;
    float secondarySubtractScale=0.00009999999747378752f; // 0x0043B350
    float secondaryDerivedOffset=4.0f;             // 0x0043B284
    std::uint32_t texture0Select=0x00411EA6u;
    std::uint32_t texture0Submit=0x00411EBFu;
    std::uint32_t texture1Select=0x00411EC4u;
    std::uint32_t texture1Submit=0x00411EDFu;
    float submitX=-1.0f;
    float submitY=-0.05f;
    float submitZ=-0.05f;

    static float wrap01(float v) {
        while(v>=1.0f)v-=1.0f;
        while(v<0.0f)v+=1.0f;
        return v;
    }
    static float advancePrimary(float phase,int dtMs) {
        return wrap01(phase+float(dtMs)*0.0003000000142492354f);
    }
    static float advanceSecondary(float phase,float primaryAfterUpdate) {
        return wrap01(phase-(primaryAfterUpdate+8.0f)*0.00009999999747378752f);
    }
};
inline constexpr LegacyMenuBackgroundAnimationTrace kLegacyMenuBackgroundAnimationTrace{};

// r59 DIRECT EXE: records/high-score screen background path.  0x0040FA74
// selects logical texture 1; its local phase advances by the same 0.0003
// scalar and is negated for both origin arguments passed to 0x0040E3D0.
// The quad spans 3.0 x 3.0 in the helper's coordinate domain.  The colour is
// packed from the current records presentation intensity; its exact semantic
// label remains intentionally neutral.
struct LegacyRecordsBackgroundTrace {
    std::uint32_t recordsRoutine=0x0040F160u;
    std::uint32_t texture1Select=0x0040FA74u;
    std::uint32_t phaseUpdateStart=0x0040FA79u;
    std::uint32_t quadCall=0x0040FAE7u;
    std::uint32_t quadRoutine=0x0040E3D0u;
    std::uint32_t frameDeltaGlobal=0x025450C8u;
    float phaseStep=0.0003000000142492354f;
    float extentX=3.0f;
    float extentY=3.0f;
    float uvInset0=0.9900000095367432f;
    float uvInset1=1.0101009607315063f;
    static float advancePhase(float phase,int tickDelta) {
        return LegacyMenuBackgroundAnimationTrace::wrap01(
            phase+float(tickDelta)*0.0003000000142492354f);
    }
};
inline constexpr LegacyRecordsBackgroundTrace kLegacyRecordsBackgroundTrace{};


// r60 DIRECT EXE: literal manually-prepared menu background geometry built by
// 0x004147B0 and mutated by 0x00411D60. These records are the exact 24-byte
// {x,y,z,u,v,light} payload consumed by 0x0040C350; topology is one quad whose
// indices are stored by the old transformed-workspace byte offsets 0,44,88,132.
struct LegacyMenuPreparedVertexTrace {
    float x,y,z,u,v,light;
};

struct LegacyMenuPreparedLayerTrace {
    std::uint32_t initRoutine=0x004147B0u;
    std::uint32_t baseAddress=0u;
    std::array<LegacyMenuPreparedVertexTrace,4> vertices{};
    std::array<std::uint32_t,4> topologyByteOffsets{{0u,44u,88u,132u}};
};

inline LegacyMenuPreparedLayerTrace makeLegacyMenuLayer0(float phase) {
    LegacyMenuPreparedLayerTrace r{};
    r.baseAddress=0x02545710u;
    r.vertices={{
        {0.f,0.f,0.09f,0.f,phase,1.f},
        {0.f,0.f,1.f,0.f,phase+8.f,0.f},
        {2.f,0.f,1.f,16.f,phase+8.f,0.f},
        {2.f,0.f,0.09f,16.f,phase,1.f}
    }};
    return r;
}

inline LegacyMenuPreparedLayerTrace makeLegacyMenuLayer1(float phase) {
    LegacyMenuPreparedLayerTrace r{};
    r.baseAddress=0x025458A8u;
    r.vertices={{
        {0.f,0.f,1.f,0.f,phase,0.f},
        {0.f,0.5f,0.09f,0.f,phase+4.f,1.f},
        {2.f,0.5f,0.09f,4.f,phase+4.f,1.f},
        {2.f,0.f,1.f,4.f,phase,0.f}
    }};
    return r;
}

// r60 DIRECT EXE: steady-state main-menu item presentation in 0x00412F90.
// The builder 0x004229B0 creates model slots 0..14; the five M1 entries use
// slots 0,1,2,3,5 (M105 is absent, hence M106 maps to slot 5).
struct LegacyMainMenuItemPresentationTrace {
    std::uint32_t mainMenuRoutine=0x00412F90u;
    std::uint32_t modelBuilder=0x004229B0u;
    std::uint32_t modelArray=0x02585A9Cu;
    std::uint32_t preparedArray=0x025B5AE8u;
    std::array<int,5> modelSlots{{0,1,2,3,5}};
    std::array<float,5> zPositions{{-0.0015f,-0.0045f,-0.0075f,-0.0105f,-0.0135f}};
    float selectedBrightness=1.0f;
    float unselectedBrightness=0.6f;
    float brightnessStepPerMs=0.002f;
    float selectedScale=1.15f;
    float unselectedScale=1.0f;
    float scaleStepPerMs=0.001f;
    std::uint32_t loopStart=0x00413459u;
    std::uint32_t normalItemSubmit=0x0041363Du;
    std::uint32_t exitItemSubmit=0x00413606u;

    static float approach(float value,float target,float maxDelta) {
        if(value<target)return value+std::min(maxDelta,target-value);
        if(value>target)return value-std::min(maxDelta,value-target);
        return value;
    }
};
inline constexpr LegacyMainMenuItemPresentationTrace kLegacyMainMenuItemPresentationTrace{};


// r306 DIRECT EXE: registered Mode Select renderer 0x00411870. The selector
// inherits the steady M1 item camera left by 0x00412F90 and owns its own light,
// animation phases and three 3-D model families. The capability path is the
// relevant one for the GLES2 port (r293).
struct LegacyModeSelectPresentationTrace {
    std::uint32_t function=0x00411870u;
    std::uint32_t sceneBegin=0x00411AEBu;
    std::uint32_t slot0Submit=0x00411BB1u;
    std::uint32_t airborneSubmit=0x00411C21u;
    std::uint32_t crawlerSubmitA=0x00411CA3u;
    std::uint32_t crawlerSubmitB=0x00411CBAu;
    std::uint32_t reflectionA=0x00411CD6u;
    std::uint32_t reflectionB=0x00411CFEu;
    std::uint32_t sceneEnd=0x00411D1Fu;
    std::uint32_t slot0Prepared=0x025B5AE8u;
    std::uint32_t airborneMaster=0x0257F574u;
    std::uint32_t airbornePrepared=0x02583748u;
    std::uint32_t crawlerMaster=0x02585A5Cu;
    std::uint32_t crawlerPrepared=0x0257F5B8u;
    int maxFade=0x7c0;
    int lightShift=3;
    int angleStepPerMs=2;
    float backgroundStepPerMs=0.0005000000237487257f;
    float slot0Z=0.005000000353902578f;
    float airborneX=-0.013000000268220901f;
    float airborneBaseZ=-0.003000000026077032f;
    float airborneRowStep=0.0017500000540167093f;
    float airborneScale=0.12999999523162842f;
    int crawlerPitch=0x200;
    float crawlerXA=-0.009999999776482582f;
    float crawlerXB=0.009499999694526196f;
    float crawlerZ=0.005000000353902578f;
    float crawlerScaleBase=0.15000000596046448f;
    float crawlerScaleSinArg=0.004999999888241291f;
    float crawlerScaleAmplitude=0.05000000074505806f;
    float crawlerReflection=0.75f;
    // 0x411870 never calls 0x40C250; it inherits the last M1 item camera.
    float cameraX=0.f;
    float cameraY=0.017999999225139618f;
    float cameraZ=-0.003000000026077032f;
    int cameraAngle1=0,cameraAngle2=-512,cameraAngle3=0;

    static int lightByte(int fadeCounter){return std::clamp(fadeCounter,0,0x7c0)>>3;}
    static float airborneZ(int selected){return -0.003000000026077032f-float(std::clamp(selected,0,4))*0.0017500000540167093f;}
    static float crawlerScale(int anglePhase){return 0.15000000596046448f+std::sin(float(anglePhase)*0.004999999888241291f)*0.05000000074505806f;}
    // r318 DIRECT EXE 0x411BBD..0x411C21: subtype-0 airborne model uses
    // X rotation by the same dt*2 phase, scale .13, and selected-row Z.
    static int airborneAngle(int anglePhase){return anglePhase & 0x7ff;}
    // 0x411C2D..0x411CBA: crawler base matrix is X(+0x200), then
    // Z(anglePhase), then the sinusoidal uniform scale. r345 correction: 0x43B27C is 0.15, not 1.0.
    static int crawlerZAngle(int anglePhase){return anglePhase & 0x7ff;}
    static constexpr int crawlerXAngle(){return 0x200;}
    static constexpr float crawlerReflectionIntensity(){return 0.75f;}
    // r321 DIRECT EXE 0x411AC1..0x411AE3 + 0x40BFB0: row 14 of the
    // 40x15 text grid is recoloured every frame with red only. The source
    // value is trunc((sin(angle)+1)*31 + 128), then shifted << 16.
    static int footerRed(int anglePhase){
        constexpr float twoPi=6.28318530717958647692f;
        const int a=anglePhase&0x7ff;
        const float s=std::sin(twoPi*float(a)/2048.f);
        return std::clamp(static_cast<int>((s+1.f)*31.f+128.f),0,255);
    }
    static std::uint32_t footerPackedRgb(int anglePhase){return std::uint32_t(footerRed(anglePhase))<<16;}
    // r322 DIRECT EXE 0x40BB20 + 0x40BD90/0x40BF40: 32 palette levels.
    // Each RGB channel is floor(channel*level/31). Mode Select supplies
    // level=(fadeCounter>>6), clamped to 0..31.
    static int textPaletteLevel(int fadeCounter){return std::clamp(fadeCounter>>6,0,31);}
    static std::uint32_t fadeTextRgb(std::uint32_t rgb,int fadeCounter){
        const int level=textPaletteLevel(fadeCounter);
        auto sc=[&](std::uint32_t c){return (c*std::uint32_t(level))/31u;};
        const std::uint32_t r=sc((rgb>>16)&0xffu),g=sc((rgb>>8)&0xffu),b=sc(rgb&0xffu);
        return (r<<16)|(g<<8)|b;
    }
};
inline constexpr LegacyModeSelectPresentationTrace kLegacyModeSelectPresentationTrace{};

// r60 DIRECT EXE: the records-screen background colour argument passed to
// 0x0040E3D0 is an RGB grayscale replicated from (transitionCounter >> 4).
// At the normal transition clamp 0x7C0 this produces 124 -> 0x007C7C7C.
struct LegacyRecordsBackgroundColourTrace {
    std::uint32_t shiftSite=0x0040FA71u;
    std::uint32_t packStart=0x0040FAC4u;
    std::uint32_t quadCall=0x0040FAE7u;
    int transitionClamp=0x7C0;
    static std::uint32_t packedRgb(int transitionCounter) {
        const std::uint32_t i=static_cast<std::uint32_t>(transitionCounter>>4) & 0xffu;
        return i*0x00010101u;
    }
};
inline constexpr LegacyRecordsBackgroundColourTrace kLegacyRecordsBackgroundColourTrace{};


// r61 DIRECT EXE: steady-state M1 menu camera and exact 640x480 screen-space
// placement. 0x00411D60 ramps 0x00440D14 from -0.5 toward 0 at +dt*0.0004;
// 0x004133BD..0x004133E9 then calls 0x0040C250 with
//   camera=(0, 0.018 - 0.1*phase, -0.003), angles=(0,-512,0).
// 0x004147BC initializes 0x02545898 to zero, so 0x00440D1C remains zero in
// the normal M1 path. At steady phase=0, the 0x00403860 menu-item mesh has
// local visible bounds x=+-0.006, z=+-0.001. Combined with 0x0040C350 and
// the exact legacy 640x480 projection, this yields the pixel rectangles below.
struct LegacyMainMenuCameraTrace {
    std::uint32_t phaseGlobal=0x00440D14u;
    std::uint32_t phaseRateGlobal=0x00440D18u;
    std::uint32_t angle3Global=0x00440D1Cu;
    std::uint32_t angle3FactorGlobal=0x02545898u;
    std::uint32_t angle3FactorInit=0x004147BCu;
    std::uint32_t cameraSetupStart=0x004133BDu;
    std::uint32_t cameraCall=0x004133E9u;
    float initialPhase=-0.5f;
    float phaseStepPerMs=0.00039999998989515007f;
    float steadyCameraX=0.0f;
    float steadyCameraY=0.017999999225139618f;
    float cameraYPhaseScale=0.10000000149011612f;
    float steadyCameraZ=-0.003000000026077032f;
    int steadyAngle1=0;
    int steadyAngle2=-512;
    int steadyAngle3=0;
    float itemHalfWidth=0.006000000052154064f;
    float itemHalfHeight=0.0010000000474974513f;

    struct PixelRect { float left,top,right,bottom; };
    static PixelRect steadyRect640(int itemIndex,float scale) {
        constexpr float focal=320.f;
        constexpr float halfW=320.f;
        constexpr float halfH=240.f;
        constexpr float depth=0.017999999225139618f;
        constexpr float cameraZ=-0.003000000026077032f;
        constexpr float hx=0.006000000052154064f;
        constexpr float hz=0.0010000000474974513f;
        constexpr float z[5]={-0.0015f,-0.0045f,-0.0075f,-0.0105f,-0.0135f};
        if(itemIndex<0)itemIndex=0;
        if(itemIndex>4)itemIndex=4;
        const float cx=halfW;
        const float cy=halfH+(cameraZ-z[itemIndex])*focal/depth;
        const float px=hx*scale*focal/depth;
        const float py=hz*scale*focal/depth;
        return {cx-px,cy-py,cx+px,cy+py};
    }
};
inline constexpr LegacyMainMenuCameraTrace kLegacyMainMenuCameraTrace{};

// r61 DIRECT EXE: records/high-score text overlay. M1 constructor 0x00423C30
// uploads fnt4 to logical texture 5. 0x0040BF60 writes characters into a
// 40x15 cell buffer at 0x0051BF10 (row stride 0xA0 = 40 dwords), and
// 0x0040BF40 renders the full grid through 0x0040BD90. In 640x480 this is an
// exact 16x32 pixel cell grid. 0x0040FAEF selects texture 5, enables alpha at
// 0x0040FAF8, renders at 0x0040FB05, then disables alpha at 0x0040FB0B.
struct LegacyRecordsTextGridTrace {
    std::uint32_t recordsRoutine=0x0040F160u;
    std::uint32_t fontFourCC=0x34746E66u; // "fnt4"
    int fontTextureSlot=5;
    std::uint32_t gridBase=0x0051BF10u;
    std::uint32_t gridClear=0x0040BE90u;
    std::uint32_t putText=0x0040BF60u;
    std::uint32_t drawGrid=0x0040BF40u;
    std::uint32_t drawCore=0x0040BD90u;
    int columns=40;
    int rows=15;
    int cellWidth640=16;
    int cellHeight480=32;
    std::uint32_t texture5Select=0x0040FAEFu;
    std::uint32_t alphaEnable=0x0040FAF8u;
    std::uint32_t renderCall=0x0040FB05u;
    std::uint32_t alphaDisable=0x0040FB0Bu;
    std::uint32_t headingAddress=0x00440DC0u; // CP1251: "Р Е К О Р Д Ы"
    int headingLength=13;
    int headingColumn=14;
    int headingRow=1;
    std::uint32_t headingPackedRgb=0x00FF0000u;
    std::uint32_t separatorAddress=0x0043F14Cu; // "--------------------------"
    int separatorLength=26;
    int separatorColumn=7;
    int topSeparatorRow=2;
    int bottomSeparatorRow=13;
    std::uint32_t modeLabelRoutine=0x0040F100u;
    std::uint32_t modeNameBase=0x025B5BCCu; // 5 x 12-byte SOUNDINF names
    int modeSourceStride=12;
    int modeLabelLength=11;
    // 0x40F100 fills eleven spaces, finds the first space terminator in the
    // source mode name, then copies the visible bytes centred into the field.
    static int centeredModeStart(int visibleLength){ return (11-visibleLength)/2; }
    int modeLabelColumn=15;
    int firstScoreRow=3;
    int scoreRows=10;
    int nameColumn=7;
    int nameLength=16;
    int leaderColumn=23;
    int leaderLength=10;
    std::uint32_t leaderAddress=0x00440DD0u; // ".........."
    int scoreRightEdgeColumn=33;

    static int pixelX640(int column){ return column*16; }
    static int pixelY480(int row){ return row*32; }
};
inline constexpr LegacyRecordsTextGridTrace kLegacyRecordsTextGridTrace{};


// r61 DIRECT EXE: exact fnt4 glyph UV lookup generated by 0x0040BBA0 after
// 0x0040BB20. The table at 0x0051C870 contains 16x10 records of four floats
// {u0,v0,u1,v1}. 0x0040BF60 first stores (characterByte-0x20) in the
// grid high byte. 0x0040BD90 then uses that internal code directly when <128
// or subtracts 64 when >=128. Thus ASCII 0x20..0x7F maps to glyphs 0..95
// and CP1251 uppercase Cyrillic 0xC0..0xDF maps to glyphs 96..127.
struct LegacyFnt4GlyphTrace {
    std::uint32_t initIndices=0x0040BB20u;
    std::uint32_t initUv=0x0040BBA0u;
    std::uint32_t uvTable=0x0051C870u;
    std::uint32_t gridWriter=0x0040BF60u;
    std::uint32_t renderer=0x0040BD90u;
    int columns=16;
    int rows=10;
    float uStart=0.005859375f;
    float vStart=0.009765625f;
    float uExtent=0.060546875f;
    float vExtent=0.09765625f;
    float uStep=0.06191406399011612f;
    float vStep=0.09847655892372131f;
    static int glyphIndex(unsigned char ch){
        const unsigned int code=(static_cast<unsigned int>(ch)-0x20u)&0xffu;
        return code<128u?static_cast<int>(code):static_cast<int>(code)-64;
    }
    static std::array<float,4> uvForGlyph(int index){
        if(index<0)index=0;
        if(index>159)index=159;
        const int col=index%16,row=index/16;
        const float u0=0.005859375f+float(col)*0.06191406399011612f;
        const float v0=0.009765625f+float(row)*0.09847655892372131f;
        return {u0,v0,u0+0.060546875f,v0+0.09765625f};
    }
};
inline constexpr LegacyFnt4GlyphTrace kLegacyFnt4GlyphTrace{};


// r62 DIRECT EXE: texture-3 3D decoration pass on the records screen. This
// follows the fnt4 grid; it is not a 2D sprite layer. Two mirrored instances of
// gameplay pickup model 0 are prepared first, then six crawler/ground-enemy
// base instances are submitted. r63/r66 closed the six 0x40E710 calls: they
// queue the environment-mapped master of that same crawler model and are
// flushed later by 0x40E960 through logical texture 6 (1111).
struct LegacyRecordsDecorationTrace {
    std::uint32_t texture3Select=0x0040FB10u;
    std::uint32_t pickupModel0=0x025849C0u;
    std::uint32_t pickupPrepared0=0x025835E4u;
    std::uint32_t pickupSubmitA=0x0040FB7Eu;
    std::uint32_t pickupSubmitB=0x0040FC07u;
    std::uint32_t restoreLessEqual=0x0040FC33u;
    float pickupX=0.035f;
    float pickupY=0.048f;
    float pickupZ=0.08f;
    int pickupMirrorCount=2;
    std::uint32_t crawlerModel=0x0257F5B8u;
    std::uint32_t crawlerSubmitFirst=0x0040FD22u;
    std::uint32_t crawlerSubmitLast=0x0040FDADu;
    int crawlerCount=6;
    std::uint32_t companionModel=0x02585A5Cu;
    std::uint32_t companionSubmitFirst=0x0040FDD1u;
    std::uint32_t companionSubmitLast=0x0040FE9Fu;
    int companionCount=6;
    std::uint32_t postCompanionModelA=0x0257F574u;
    std::uint32_t postCompanionPreparedA=0x02583748u;
    std::uint32_t postCompanionModelB=0x0257F57Cu;
    std::uint32_t postCompanionPreparedB=0x02583750u;
    std::uint32_t finalAlphaEnable=0x0040FF29u;
    std::uint32_t finalTexture6Select=0x0040FF30u;
    std::uint32_t finalLayerSubmit=0x0040FF37u;
};
inline constexpr LegacyRecordsDecorationTrace kLegacyRecordsDecorationTrace{};
