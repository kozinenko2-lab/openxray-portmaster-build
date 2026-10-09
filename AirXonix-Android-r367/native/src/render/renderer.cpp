#include <cstring>
#include "renderer.hpp"
#include "game/game.hpp"
#include "legacy_atlas_runtime.hpp"
#include "legacy_camera.hpp"
#include "legacy_gameover_trace.hpp"
#include "legacy_models.hpp"
#include "legacy_hud.hpp"
#include "legacy_settings_widgets.hpp"
#include "legacy_reflection.hpp"
#include "legacy_screen_pipeline.hpp"
#include "legacy_screen_frame.hpp"
#include "legacy_transform.hpp"
#include "legacy_main_menu_decor.hpp"
#include "legacy_information_render_state.hpp"
#include "legacy_information_backdrop.hpp"
#include "legacy_theme.hpp"
#include "legacy_quality_trace.hpp"
#include "legacy_directional_glint.hpp"
#include "legacy_field_render_state.hpp"
#include "legacy_field_frontend.hpp"
#include "legacy_field_tessellators.hpp"
#include "legacy_billboard.hpp"
#include "legacy_cnt3_digits.hpp"
#include "game/legacy_particles.hpp"
#include "game/legacy_information_transition.hpp"
#include "game/legacy_records_transition.hpp"
#include "game/legacy_interlevel_trace.hpp"
#include "game/legacy_finale_cinematic_trace.hpp"
#include "game/legacy_settings_visual_trace.hpp"
#include "game/legacy_effects.hpp"
#include "game/legacy_controls_trace.hpp"
#include "game/legacy_pause_trace.hpp"
#include "game/legacy_restore_trace.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

#ifndef GL_ALWAYS
#define GL_ALWAYS 0x0207
#endif

namespace {
GLuint shader(GLenum type,const char* src){GLuint s=glCreateShader(type);glShaderSource(s,1,&src,nullptr);glCompileShader(s);GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){char log[512]{};glGetShaderInfoLog(s,sizeof(log),nullptr,log);std::fprintf(stderr,"shader: %s\n",log);glDeleteShader(s);return 0;}return s;}

LegacyMasterVertex transformedVertex(const LegacyMasterVertex& v,const LegacyTransform::Matrix34& m,float tx,float ty,float tz){
    LegacyMasterVertex o=v;
    const auto p=LegacyTransform::transformPoint(m,v.x,v.y,v.z,tx,ty,tz);
    const auto n=LegacyTransform::transformNormal(m,v.nx,v.ny,v.nz);
    o.x=p.x; o.y=p.y; o.z=p.z;
    o.nx=n.x; o.ny=n.y; o.nz=n.z;
    return o;
}

struct LegacyDirectionalLighting { float dirX,dirY,dirZ,ambient; };

float legacyLightWith(const LegacyMasterVertex& v,const LegacyDirectionalLighting& l){
    const float d=v.nx*(-l.dirX)+v.ny*(-l.dirY)+v.nz*(-l.dirZ);
    return l.ambient+std::max(0.f,d);
}

float legacyLight(const LegacyMasterVertex& v){
    // r58 DIRECT EXE: startup/gameplay 0x40E380(+pi/4,-pi/4,.8,.2).
    constexpr auto l=LegacyModels::InitialLighting;
    return legacyLightWith(v,{l.dirX,l.dirY,l.dirZ,l.ambient});
}

// r182 DIRECT EXE 0x412BE6..0x412BFA: the four menu pickups use a
// weaker directional light than normal gameplay: (+pi/4,-pi/4,.5,.2).
constexpr LegacyDirectionalLighting kMenuPickupLighting{
    0.25f,-0.3535533845424652f,0.25f,0.2f};
// 0x412EE3..0x412EF7 restores a brighter state before texture6/7:
// (+pi/4,-pi/4,.7,.3).  The subsequent IN2$/$1000 base draw inherits it.
constexpr LegacyDirectionalLighting kMenuPostPickupLighting{
    0.3499999940395355f,-0.4949747323989868f,0.3499999940395355f,0.30000001192092896f};

// r194 DIRECT EXE 0x412273..0x412287: 0x412150 opens with
// 0x40E380(+pi/4,-pi/4,.7,.3). Slots 0..4, airborne subtype 2 and both crawlers
// are lit by this state until 0x412BE6 switches to kMenuPickupLighting.
constexpr LegacyDirectionalLighting kMenuSceneLighting=kMenuPostPickupLighting;
// r194 DIRECT EXE 0x41337E..0x413392: the title layer 0x411EF0 is lit with
// 0x40E380(+pi/4,-pi/4,.6,.25).
constexpr LegacyDirectionalLighting kMenuTitleLighting{
    0.30000001192092896f,-0.4242640733718872f,0.30000001192092896f,0.25f};
// r194 DIRECT EXE 0x4133EE..0x413402: selector markers are lit with
// 0x40E380(-pi/4,-pi/4,.7,.3); first angle is negative, so Z flips.
constexpr LegacyDirectionalLighting kMenuSelectorLighting{
    0.3499999940395355f,-0.4949747323989868f,-0.3499999940395355f,0.30000001192092896f};

// r194 DIRECT EXE render-state census (every IDirect3DDevice7::SetRenderState
// and SetTextureStageState call site in AirXonix.wrp.exe): 0x405AB0 sets
// SHADEMODE, PERSPECTIVE, DITHER, SPECULAR=0, CULL=NONE, LIGHTING=0, ZFUNC,
// ZENABLE, stage-0 LINEAR mag/min/mip and COLOROP=MODULATE; 0x405A20/0x405A60
// toggle only ZFUNC/ALPHABLENDENABLE with SRC/DEST=ONE/ONE. There is no
// COLORKEYENABLE (41), no ALPHATESTENABLE (15) and no IDirectDrawSurface7::
// SetColorKey call. Black texels of atlas3 are therefore drawn opaque black on
// Xonix/enemy/pickup/debris/menu-decor/XON1 geometry. The r175 black-pixel
// discard made the Xonix body and other models see-through.
constexpr bool kLegacyColorKeyDiscard=false;

bool appendLegacyMesh(const LegacyMesh& mesh,const LegacyTransform::Matrix34& model,
                      float tx,float ty,float tz,
                      const LegacyScreenPipeline::CameraState& camera,
                      const LegacyCamera::ProjectionState& projection,
                      LegacyScreenPipeline::OutputBatch& out,
                      float r=1.f,float g=1.f,float b=1.f,float a=1.f,
                      float lightOverride=-1.f,
                      const LegacyDirectionalLighting* lightingOverride=nullptr){
    bool any=false;
    std::vector<LegacyMasterVertex> tv; tv.reserve(mesh.vertices.size());
    for(const auto& v:mesh.vertices) tv.push_back(transformedVertex(v,model,tx,ty,tz));
    for(const auto& face:mesh.faces){
        if(face.index.size()!=3 && face.index.size()!=4)continue;
        std::array<LegacyScreenPipeline::InputVertex,4> p{};
        bool valid=true;
        for(std::size_t i=0;i<face.index.size();++i){
            const auto idx=face.index[i]; if(idx>=tv.size()){valid=false;break;}
            const auto& v=tv[idx];
            const float light=lightOverride>=0.f?lightOverride:(lightingOverride?legacyLightWith(v,*lightingOverride):legacyLight(v));
            p[i]={v.x,v.y,v.z,v.u,v.v,light,r,g,b,a};
        }
        if(valid) any|=LegacyScreenPipeline::appendPolygon(p.data(),face.index.size(),camera,projection,out);
    }
    return any;
}

// r348: literal geometry effect of AirXonix.wrp.exe:0x41ECC0. This routine
// builds the two crawler floor projections; it must never be used as the
// crawler's visible body draw.
bool appendCrawlerProjection(const LegacyMesh& mesh,float tx,float ty,float tz,float postY,float light,
                             const LegacyScreenPipeline::CameraState& camera,
                             const LegacyCamera::ProjectionState& projection,
                             LegacyScreenPipeline::OutputBatch& out){
    constexpr auto g=LegacyModels::GroundEnemyPresentation;
    bool any=false;
    for(const auto& face:mesh.faces){
        if(face.index.size()!=3 && face.index.size()!=4) continue;
        std::array<LegacyScreenPipeline::InputVertex,4> p{};
        bool valid=true;
        for(std::size_t i=0;i<face.index.size();++i){
            const auto idx=face.index[i];
            if(idx>=mesh.vertices.size()){valid=false;break;}
            const auto& v=mesh.vertices[idx];
            const float worldY=v.y+ty;
            const float x=v.x+g.projectionSkewX*worldY+tx;
            const float z=v.z+g.projectionSkewZ*worldY+tz;
            p[i]={x,g.projectionPlaneY+postY,z,
                  x*g.projectionUvScale,z*g.projectionUvScale,
                  light,1.f,1.f,1.f,1.f};
        }
        if(valid) any|=LegacyScreenPipeline::appendPolygon(p.data(),face.index.size(),camera,projection,out);
    }
    return any;
}

}

Renderer::~Renderer(){
    if(atlas3_)glDeleteTextures(1,&atlas3_);
    if(atlas4_)glDeleteTextures(1,&atlas4_);
    if(atlas4Game_)glDeleteTextures(1,&atlas4Game_);
    if(atlas7_)glDeleteTextures(1,&atlas7_);
    if(menuAtlasM1_)glDeleteTextures(1,&menuAtlasM1_);
    if(menuAtlasM2_)glDeleteTextures(1,&menuAtlasM2_);
    if(font5_)glDeleteTextures(1,&font5_);
    if(menuLogo_)glDeleteTextures(1,&menuLogo_);
    if(menuLaxy_)glDeleteTextures(1,&menuLaxy_);
    if(reflectionTexture_)glDeleteTextures(1,&reflectionTexture_);
    for(auto& theme:environmentTextures_)for(GLuint& t:theme)if(t)glDeleteTextures(1,&t);
    for(auto& pair:menuEnvironmentTextures_)for(GLuint& t:pair)if(t)glDeleteTextures(1,&t);
    if(ibo_) glDeleteBuffers(1,&ibo_);
    if(vbo_) glDeleteBuffers(1,&vbo_);
    if(program_) glDeleteProgram(program_);
}

bool Renderer::initProgram(){
    static const char* vs=
        "attribute vec4 aPos;attribute vec2 aUv;attribute float aLight;attribute vec4 aColor;"
        "varying vec2 vUv;varying float vLight;varying vec4 vColor;"
        "uniform float uUvFlip;uniform vec2 uUvOffset;"
        "void main(){vec2 uv=aUv+uUvOffset;if(uUvFlip>0.5)uv.y=1.0-uv.y;vUv=uv;vLight=aLight;vColor=aColor;gl_Position=aPos;}";
    static const char* fs=
        "precision mediump float;varying vec2 vUv;varying float vLight;varying vec4 vColor;"
        "uniform sampler2D uTexture;uniform float uUseTexture;uniform float uBrightness;uniform vec3 uTint;uniform float uAlphaTest;"
        "void main(){vec4 tex=texture2D(uTexture,vUv);"
        "if(uUseTexture>0.5 && uAlphaTest>0.5 && tex.a<0.5) discard;"
        "vec4 base=mix(vec4(1.0),tex,uUseTexture);"
        "gl_FragColor=vec4(base.rgb*vColor.rgb*vLight*uBrightness*uTint,base.a*vColor.a);}";
    GLuint v=shader(GL_VERTEX_SHADER,vs),f=shader(GL_FRAGMENT_SHADER,fs);if(!v||!f)return false;
    program_=glCreateProgram();glAttachShader(program_,v);glAttachShader(program_,f);
    glBindAttribLocation(program_,0,"aPos");glBindAttribLocation(program_,1,"aUv");glBindAttribLocation(program_,2,"aLight");glBindAttribLocation(program_,3,"aColor");
    glLinkProgram(program_);glDeleteShader(v);glDeleteShader(f);GLint ok=0;glGetProgramiv(program_,GL_LINK_STATUS,&ok);if(!ok)return false;
    aPos_=0;aUv_=1;aLight_=2;aColor_=3;uBrightness_=glGetUniformLocation(program_,"uBrightness");uTint_=glGetUniformLocation(program_,"uTint");uUseTexture_=glGetUniformLocation(program_,"uUseTexture");uTexture_=glGetUniformLocation(program_,"uTexture");uUvFlip_=glGetUniformLocation(program_,"uUvFlip");uUvOffset_=glGetUniformLocation(program_,"uUvOffset");uAlphaTest_=glGetUniformLocation(program_,"uAlphaTest");
    glGenBuffers(1,&vbo_);glGenBuffers(1,&ibo_);return true;
}

GLuint Renderer::uploadTexture(const LegacyAtlasImage& image,bool repeat){
    if(image.width<=0||image.height<=0||image.rgba.empty())return 0;
    GLuint texture=0;glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,repeat?GL_REPEAT:GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,repeat?GL_REPEAT:GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,image.width,image.height,0,GL_RGBA,GL_UNSIGNED_BYTE,image.rgba.data());
    if(glGetError()!=GL_NO_ERROR){glDeleteTextures(1,&texture);return 0;}return texture;
}

bool Renderer::initGameplayAtlas(const std::string& texturesPath){
    LegacyAtlasImage image;std::string error;
    if(!LegacyAtlasRuntime::buildGameplayAtlas3(texturesPath,image,&error,&originalResources_)){std::fprintf(stderr,"AirXonix atlas3: %s\n",error.c_str());return false;}
    atlas3_=uploadTexture(image,false);
    // r175: keep the recovered black colour-key in the only gameplay atlas.
    // r77 already established shader discard for keyed legacy textures. The
    // r16x opaque duplicate forced black texels back to alpha=255 and caused
    // black/brown model patches plus apparently missing transparent regions.
    // r194: 0x405B3D..0x405B73 sets stage-0 MAG/MIN/MIP filter = LINEAR (2).
    // uploadTexture already selects GL_LINEAR; the r175 NEAREST override is
    // removed together with the non-existent colour-key discard.
    return atlas3_!=0;
}

bool Renderer::initUiAtlas(const std::string& texturesPath){
    LegacyAtlasImage image;std::string error;
    if(!LegacyAtlasRuntime::buildUiAtlas4(texturesPath,image,&error,&originalResources_)){std::fprintf(stderr,"AirXonix atlas4 stage: %s\n",error.c_str());return false;}
    atlas4_=uploadTexture(image,false);
    image={};error.clear();
    if(!LegacyAtlasRuntime::buildUiAtlas4GameComplete(texturesPath,image,&error,&originalResources_)){std::fprintf(stderr,"AirXonix atlas4 game: %s\n",error.c_str());return false;}
    atlas4Game_=uploadTexture(image,false);
    return atlas4_!=0 && atlas4Game_!=0;
}

bool Renderer::initAuxAtlas(const std::string& texturesPath){
    LegacyAtlasImage image;std::string error;
    if(!LegacyAtlasRuntime::buildAuxAtlas7(texturesPath,image,&error,&originalResources_)){std::fprintf(stderr,"AirXonix atlas7: %s\n",error.c_str());return false;}
    atlas7_=uploadTexture(image,false);return atlas7_!=0;
}

void Renderer::initMenuAtlases(const std::string& texturesPath){
    // 0x423C30 and 0x423EE0 rebuild the same legacy atlas #4 surface for two
    // distinct menu/screen resource families.  GLES keeps them as two cached
    // textures so the eventual 0x412F90 callsite mapping can switch families
    // without rebuilding/uploading an atlas at runtime.  They are NOT drawn
    // r153: M1 and M2 ownership is now resolved; both cached atlases are used
    // by their matching presentation states without runtime atlas rebuilds.
    LegacyAtlasImage image;std::string error;
    if(LegacyAtlasRuntime::buildMenuAtlas4M1(texturesPath,image,&error,&originalResources_))
        menuAtlasM1_=uploadTexture(image,false);
    else if(!error.empty()) std::fprintf(stderr,"AirXonix M1 menu atlas: %s\n",error.c_str());
    image={};error.clear();
    if(LegacyAtlasRuntime::buildMenuAtlas3M1(texturesPath,image,&error,&originalResources_))
        menuAtlas3M1_=uploadTexture(image,false);
    image={};error.clear();
    if(LegacyAtlasRuntime::buildMenuAtlas3M2(texturesPath,image,&error,&originalResources_))
        menuAtlas3M2_=uploadTexture(image,false);
    image={};error.clear();
    if(LegacyAtlasRuntime::buildMenuAtlas4M2(texturesPath,image,&error,&originalResources_))
        menuAtlasM2_=uploadTexture(image,false);
    else if(!error.empty()) std::fprintf(stderr,"AirXonix M2 menu atlas: %s\n",error.c_str());
}

bool Renderer::initReflectionTexture(const std::string& texturesPath){
    LegacyAtlasImage image;std::string error;
    if(!LegacyAtlasRuntime::loadTexture(texturesPath,"1111",&originalResources_,image,&error)){
        std::fprintf(stderr,"AirXonix reflection texture 1111: %s\n",error.c_str());
        return false;
    }
    reflectionTexture_=uploadTexture(image,false);
    return reflectionTexture_!=0;
}

bool Renderer::initEnvironmentTextures(const std::string& texturesPath){
    // r48/r139 DIRECT EXE: preload every exact 0x4418B0 theme record.
    // Runtime chooses among them through 0x422FC0; keeping all resident avoids
    // filesystem work during the original inter-level transition.
    bool ok=true;
    for(std::size_t ti=0;ti<kLegacyEnvironmentThemes.size();++ti){
        const auto& theme=kLegacyEnvironmentThemes[ti];
        const std::array<const char*,3> names{{theme.slot0,theme.slot1,theme.slot2}};
        for(std::size_t slot=0;slot<3;++slot){ LegacyAtlasImage image; std::string error;
            if(!LegacyAtlasRuntime::loadTexture(texturesPath,names[slot],&originalResources_,image,&error)){
                std::fprintf(stderr,"AirXonix env theme %zu slot %zu texture %s: %s\n",ti,slot,names[slot],error.c_str()); ok=false; continue;
            }
            environmentTextures_[ti][slot]=uploadTexture(image,true);
            if(!environmentTextures_[ti][slot])ok=false;
        }
    }
    return ok;
}


bool Renderer::initMenuEnvironmentTextures(const std::string& texturesPath){
    // r62 DIRECT EXE: preload all eight exact pairs; Game::enterMainMenu uses
    // the original 0x422F40 least-used/random selector and exposes themeIndex.
    bool ok=true;
    for(std::size_t themeIndex=0;themeIndex<kLegacyMenuEnvironmentThemes.size();++themeIndex){
        const auto& theme=kLegacyMenuEnvironmentThemes[themeIndex];
        const std::array<const char*,2> names{{theme.slot0,theme.slot1}};
        for(std::size_t slot=0;slot<2;++slot){
            LegacyAtlasImage image;std::string error;
            if(!LegacyAtlasRuntime::loadTexture(texturesPath,names[slot],&originalResources_,image,&error)){
                std::fprintf(stderr,"AirXonix menu env %s: %s\n",names[slot],error.c_str());ok=false;continue;
            }
            menuEnvironmentTextures_[themeIndex][slot]=uploadTexture(image,true);
            if(!menuEnvironmentTextures_[themeIndex][slot])ok=false;
        }
    }
    return ok;
}

void Renderer::initLegacyModels(){
    // r249: 0x43F0AC is written exactly once on successful original renderer
    // initialization, from EBX=1. Keep the literal shipped-EXE quality path.
    constexpr bool highQuality=LegacyQualityTrace::highQuality;
    xonixModel_=LegacyModelFactory::buildXonix(highQuality);
    menuLogoModel_=LegacyModelFactory::buildMainMenuLogoParts(highQuality);
    for(int i=0;i<7;++i)menuDecorationModels_[std::size_t(i)]=LegacyModelFactory::buildMainMenuDecorationSlot(i,highQuality);
    groundEnemyModel_=LegacyModelFactory::buildGroundEnemy(highQuality);
    for(int i=0;i<4;++i)airEnemyModels_[std::size_t(i)]=LegacyModelFactory::buildAirEnemySubtype(i,highQuality);
    airEnemyShadowModel_=LegacyModelFactory::buildAirEnemyShadow();
    for(int i=0;i<6;++i)pickupModels_[std::size_t(i)]=LegacyModelFactory::buildPickupType(i,highQuality);
    for(int i=0;i<6;++i)auxiliaryModels_[std::size_t(i)]=LegacyModelFactory::buildAuxiliarySlot(i);
    trailNormalModel_=LegacyModelFactory::buildTrailSegment(false,highQuality);
    trailDamagedModel_=LegacyModelFactory::buildTrailSegment(true,highQuality);
    settingsTrackModel_=LegacySettingsWidgets::buildTrack();
    settingsKnobModel_=LegacySettingsWidgets::buildKnob();
    eraserModel_=LegacyModelFactory::buildEraser(highQuality);
    homingModel_=LegacyModelFactory::buildHoming(highQuality);
    informationFlyingMineModel_=LegacyModelFactory::buildInformationFlyingMine();
    interLevelCinematicSlot0_=LegacyModelFactory::buildCinematicSlot(0);
    levelIntroCinematicSlot1_=LegacyModelFactory::buildCinematicSlot(1);
    pauseCinematicSlot2_=LegacyModelFactory::buildCinematicSlot(2);
    gameOverCinematicSlot3_=LegacyModelFactory::buildCinematicSlot(3);
    abortCinematicSlot4_=LegacyModelFactory::buildCinematicSlot(4);
    interLevelCinematicSlot5_=LegacyModelFactory::buildCinematicSlot(5);
}

bool Renderer::init(SDL_Window* window,const std::string& texturesPath,const std::string& originalExePath){
    if(!originalExePath.empty()){std::string packedError;if(originalResources_.open(originalExePath,&packedError))std::fprintf(stdout,"AirXonix packed resources: %s (%zu BMPPACK textures)\n",originalExePath.c_str(),originalResources_.textureCount());else std::fprintf(stderr,"AirXonix packed resources disabled: %s\n",packedError.c_str());}
    glEnable(GL_DITHER);glDisable(GL_CULL_FACE);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDisable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE);resize(window);
    if(!initProgram()) return false;
    initLegacyModels();
    if(!initGameplayAtlas(texturesPath))std::fprintf(stderr,"Renderer: continuing without gameplay atlas texture\n");
    if(!initUiAtlas(texturesPath))std::fprintf(stderr,"Renderer: continuing without UI atlas texture\n");
    if(!initAuxAtlas(texturesPath))std::fprintf(stderr,"Renderer: continuing without auxiliary atlas #7 texture\n");
    initMenuAtlases(texturesPath);
    { LegacyAtlasImage logo; std::string err; if(LegacyAtlasRuntime::loadTexture(texturesPath,"LOGO",&originalResources_,logo,&err)) menuLogo_=uploadTexture(logo,false); else if(!err.empty()) std::fprintf(stderr,"AirXonix LOGO: %s\n",err.c_str()); }
    { LegacyAtlasImage laxy; std::string err; if(LegacyAtlasRuntime::loadTexture(texturesPath,"LAXY",&originalResources_,laxy,&err)) menuLaxy_=uploadTexture(laxy,false); else if(!err.empty()) std::fprintf(stderr,"AirXonix LAXY: %s\n",err.c_str()); }
    { LegacyAtlasImage f; std::string e; if(LegacyAtlasRuntime::loadTexture(texturesPath,"fnt4",&originalResources_,f,&e)) font5_=uploadTexture(f,false); }
    if(!initEnvironmentTextures(texturesPath))std::fprintf(stderr,"Renderer: continuing with incomplete environment textures\n");
    if(!initReflectionTexture(texturesPath))std::fprintf(stderr,"Renderer: continuing without reflection texture 1111\n");
    if(!initMenuEnvironmentTextures(texturesPath))std::fprintf(stderr,"Renderer: continuing with incomplete menu environment textures\n");
    rebuildBackgroundBatch();
    return glGetError()==GL_NO_ERROR;
}

void Renderer::resize(SDL_Window* window){int w=640,h=480;SDL_GL_GetDrawableSize(window,&w,&h);constexpr float target=4.f/3.f;viewportX_=viewportY_=0;viewportW_=w;viewportH_=h;if(h>0&&float(w)/h>target){viewportW_=int(h*target);viewportX_=(w-viewportW_)/2;}else if(w>0){viewportH_=int(w/target);viewportY_=(h-viewportH_)/2;}glViewport(viewportX_,viewportY_,viewportW_,viewportH_);rebuildLegacyScreenFrameBatch();}
void Renderer::beginFrame(){glClearColor(.015f,.02f,.04f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);}

void Renderer::rebuildBoard(const Game& game){
    vertices_.clear();floorWorldVertices_.clear();wallWorldVertices_.clear();frontLogoWorldVertices_.clear();
    vertices_.reserve(8192);floorWorldVertices_.reserve(32);wallWorldVertices_.reserve(64*64*18);
    const Field& f=game.field();
    auto colorTri=[&](float ax,float ay,float az,float bx,float by,float bz,float cx,float cy,float cz,float r,float g,float b,float a){
        vertices_.push_back({ax,ay,az,r,g,b,a});vertices_.push_back({bx,by,bz,r,g,b,a});vertices_.push_back({cx,cy,cz,r,g,b,a});
    };
    auto texTri=[&](std::vector<WorldTexVertex>& dst,
                    float ax,float ay,float az,float au,float av,
                    float bx,float by,float bz,float bu,float bv,
                    float cx,float cy,float cz,float cu,float cv,float light=1.f){
        dst.push_back({ax,ay,az,au,av,light,light,light,1.f});
        dst.push_back({bx,by,bz,bu,bv,light,light,light,1.f});
        dst.push_back({cx,cy,cz,cu,cv,light,light,light,1.f});
    };
    auto texTop=[&](std::vector<WorldTexVertex>& dst,float x0,float z0,float x1,float z1,float y,float u0,float v0,float u1,float v1,float light=1.f){
        texTri(dst,x0,y,z0,u0,v0,x1,y,z1,u1,v1,x1,y,z0,u1,v0,light);
        texTri(dst,x0,y,z0,u0,v0,x0,y,z1,u0,v1,x1,y,z1,u1,v1,light);
    };
    auto wallX=[&](float x,float z0,float z1,float y0,float y1,bool positive,float u0,float u1,float light){
        // 0x41F660/0x41F7F0 copy V=0/.25 from 0x258498C/0x2584990.
        const float v0=LegacyFieldTessellators::HighWallVMin,v1=LegacyFieldTessellators::HighWallVMax;
        if(positive){texTri(wallWorldVertices_,x,y0,z0,u0,v0,x,y1,z1,u1,v1,x,y1,z0,u0,v1,light);texTri(wallWorldVertices_,x,y0,z0,u0,v0,x,y0,z1,u1,v0,x,y1,z1,u1,v1,light);}
        else {texTri(wallWorldVertices_,x,y0,z0,u0,v0,x,y1,z0,u0,v1,x,y1,z1,u1,v1,light);texTri(wallWorldVertices_,x,y0,z0,u0,v0,x,y1,z1,u1,v1,x,y0,z1,u1,v0,light);}
    };
    auto wallZ=[&](float z,float x0,float x1,float y0,float y1,bool positive,float u0,float u1,float light){
        // 0x41F460 copies V=0/.25 from 0x258498C/0x2584990.
        const float v0=LegacyFieldTessellators::HighWallVMin,v1=LegacyFieldTessellators::HighWallVMax;
        if(positive){texTri(wallWorldVertices_,x0,y0,z,u0,v0,x1,y1,z,u1,v1,x0,y1,z,u0,v1,light);texTri(wallWorldVertices_,x0,y0,z,u0,v0,x1,y0,z,u1,v0,x1,y1,z,u1,v1,light);}
        else {texTri(wallWorldVertices_,x0,y0,z,u0,v0,x0,y1,z,u0,v1,x1,y1,z,u1,v1,light);texTri(wallWorldVertices_,x0,y0,z,u0,v0,x1,y1,z,u1,v1,x1,y0,z,u1,v0,light);}
    };
    constexpr float step=.003125f,base=.4f;
    constexpr float safeTop=LegacyModels::SafeFieldTopY;
    // r105: literal scan domains from 0x41F090/0x41F270. The original CPU
    // tessellators emit strip vertices only when SAFE/non-SAFE state changes;
    // equivalent merged quads preserve the same visible surface while avoiding
    // the old full-board floor approximation. 0x41F270 covers only the interior
    // 62x62 cells (field+65), never the SAFE border.
    const auto& cells=f.cells();
    // r214: 0x41F270 is camera-dependent and is emitted in
    // rebuildLegacyScreenBatch(), after the presentation camera is known.
    // Keep floorWorldVertices_ for the later static low-edge family only.
    // r215: 0x41F090 SAFE-top runs are emitted as four-vertex polygons in
    // rebuildLegacyScreenBatch(), preserving legacy clipping before fan split.

    // r107: literal low transition polygons from 0x41F980/0x41FB30. These are
    // a texture-slot-0 layer at Y=.0005 with constant light .35.  The original
    // keeps a static .4 + i*.003125 grid and i*.0625 UV grid for this family.
    auto appendLowQuad=[&](const std::array<LegacyFieldTessellators::LowVertex,4>& q){
        auto push=[&](int i){const auto& v=q[static_cast<std::size_t>(i)];floorWorldVertices_.push_back({v.x,v.y,v.z,v.u,v.v,v.light,v.light,v.light,1.f});};
        push(0);push(1);push(2); push(0);push(2);push(3);
    };
    for(const auto& e:LegacyFieldTessellators::lowHorizontalTransitions(cells))
        appendLowQuad(LegacyFieldTessellators::lowHorizontalQuad(e.x,e.y));
    for(const auto& r:LegacyFieldTessellators::lowVerticalRuns(cells))
        appendLowQuad(LegacyFieldTessellators::lowVerticalQuad(r));

    // r216: 0x41EE90 capture-marker runs are emitted as intact four-vertex
    // polygons in rebuildLegacyScreenBatch(), just like 0x41F090 SAFE tops.

    // r221: prepared rim 0x2583948 is no longer pre-split here. 0x40C350
    // transforms the shared 76-vertex object, clips/culls each of its 38
    // four-index polygons, then emits a triangle fan. rebuildLegacyScreenBatch
    // now preserves that polygon boundary exactly.
    // r173 DIRECT EXE: 0x420527 selects texture slot 3 and 0x42053D passes
    // the literal four-vertex XON1 fascia at 0x4417D0 to 0x40D1E0.  The
    // object deliberately sits in front of the 0.4..0.6 playfield, so drawing
    // it on z=.4 (r170-r172) allowed the field/rim geometry to cover it.
    // Vertex records decode as:
    //   (.3875,-.004,.3840, 0,.99000), (.3875,.008,.3875, 0,.87625),
    //   (.6125,.008,.3875, 1,.87625), (.6125,-.004,.3840, 1,.99000).
    {
        // r175: keep this family out of the generic triangle-list buffer.
        // 0x40D1E0 receives ONE 4-vertex polygon in BL->TL->TR->BR order;
        // splitting it as r173/r174 did reversed the legacy CPU-facing sign
        // and 0x40CE60 rejected both triangles before GLES saw them.
    }

    // r218: high SAFE side-wall families are emitted as intact four-vertex
    // polygons in rebuildLegacyScreenBatch(), preserving 0x40D1E0 grouping.
}

void Renderer::rebuildBackgroundBatch(){
    backgroundVertices_={
        {-1.f,-1.f,0.f,1.f,0.f,3.f,1.f,1.f,1.f,1.f,1.f},
        { 1.f,-1.f,0.f,1.f,4.f,3.f,1.f,1.f,1.f,1.f,1.f},
        { 1.f, 1.f,0.f,1.f,4.f,0.f,1.f,1.f,1.f,1.f,1.f},
        {-1.f, 1.f,0.f,1.f,0.f,0.f,1.f,1.f,1.f,1.f,1.f}
    };
    backgroundIndices_={0,1,2,0,2,3};
}

void Renderer::rebuildLegacyScreenFrameBatch(){
    legacyFrameVertices_.clear();
    legacyFrameIndices_.assign(LegacyScreenFrame::Indices.begin(),LegacyScreenFrame::Indices.end());
    const int w=std::max(1,viewportW_),h=std::max(1,viewportH_);
    const auto src=LegacyScreenFrame::vertices(w,h);
    legacyFrameVertices_.reserve(src.size());
    for(const auto& v:src){
        const float x=v.sx/(0.5f*float(w))-1.f;
        const float y=1.f-v.sy/(0.5f*float(h));
        const float z=2.f*v.sz-1.f; // D3D [0,1] sz -> GL [-1,+1] NDC depth
        legacyFrameVertices_.push_back({x,y,z,1.f,0.f,0.f,1.f,0.f,0.f,0.f,0.f});
    }
}

void Renderer::rebuildLegacyScreenBatch(const Game& game){
    using namespace LegacyScreenPipeline;
    screenVertices_.clear();screenIndices_.clear();safeScreenVertices_.clear();safeScreenIndices_.clear();floorScreenVertices_.clear();floorScreenIndices_.clear();lowEdgeScreenVertices_.clear();lowEdgeScreenIndices_.clear();wallScreenVertices_.clear();wallScreenIndices_.clear();
    const auto gc=((game.phase()==GamePhase::Dying)||(game.phase()==GamePhase::GameOver)) ? LegacyCamera::deathState(game.displayPlayerWorldX(),game.displayPlayerHeight(),game.displayPlayerWorldZ(),game.deathScene().triColorBurstStarted,game.displayCameraYawOffset()) : (game.phase()==GamePhase::InterLevel ? LegacyCamera::interLevelState(game.displayPlayerWorldX(),game.displayPlayerHeight(),game.displayPlayerWorldZ(),game.legacyCameraZoom(),game.displayCameraYawOffset()) : (game.phase()==GamePhase::FinalSequence ? LegacyCamera::finaleState(game.displayPlayerWorldX(),game.finalePresentationY(),game.displayPlayerWorldZ(),game.legacyCameraZoom(),game.displayCameraYawOffset()) : LegacyCamera::gameplayState(game.displayPlayerWorldX(),game.displayPlayerWorldZ(),game.legacyCameraZoom(),game.displayCameraYawOffset())));
    CameraState camera{gc.x,gc.y,gc.z,gc.angle1,gc.angle2,gc.angle3};const auto projection=LegacyCamera::projectionState(640,480);
    auto convert=[&](const OutputBatch& batch,std::vector<ScreenVertex>& outV,std::vector<std::uint16_t>& outI){outV.reserve(batch.vertices.size());for(const auto& v:batch.vertices)outV.push_back({v.clipX,v.clipY,v.clipZ,v.clipW,v.u,v.v,v.light,v.r,v.g,v.b,v.a});outI=batch.indices;};

    const auto fg=LegacyFieldFrontend::build(gc.x,gc.y,gc.z);

    // r103: exact prepared background object 0x2583600. Legacy does not draw a
    // fullscreen quad here: it draws four world-space quads at Y=-0.04 around
    // a camera-dependent inner opening. Vertex UVs are x*8/z*8 and light is
    // 1-z*(5/6). The pass owns texture slot 1 and ZFUNC=ALWAYS.
    {
        const auto ring=LegacyFieldFrontend::backgroundRing(fg,game.legacyBackgroundVPhase());
        OutputBatch bg; bg.vertices.reserve(16); bg.indices.reserve(24);
        for(const auto& q:LegacyFieldFrontend::BackgroundQuads){
            std::array<InputVertex,4> p{};
            for(int i=0;i<4;++i){const auto& v=ring[q[std::size_t(i)]];p[std::size_t(i)]={v.x,v.y,v.z,v.u,v.v,v.light,1.f,1.f,1.f,1.f};}
            appendPolygon(p.data(),4,camera,projection,bg);
        }
        backgroundVertices_.clear();backgroundIndices_.clear();convert(bg,backgroundVertices_,backgroundIndices_);
    }
    OutputBatch colorBatch;colorBatch.vertices.reserve(vertices_.size());colorBatch.indices.reserve(vertices_.size());
    for(std::size_t i=0;i+2<vertices_.size();i+=3){std::array<InputVertex,3> tri{};for(int j=0;j<3;++j){const auto& v=vertices_[i+std::size_t(j)];tri[std::size_t(j)]={v.x,v.y,v.z,0.f,0.f,1.f,v.r,v.g,v.b,v.a};}appendTriangle(tri,camera,projection,colorBatch);}convert(colorBatch,screenVertices_,screenIndices_);
    auto appendTexturedTriangles=[&](const std::vector<WorldTexVertex>& src,OutputBatch& batch){
        batch.vertices.reserve(batch.vertices.size()+src.size());
        batch.indices.reserve(batch.indices.size()+src.size());
        for(std::size_t i=0;i+2<src.size();i+=3){
            std::array<InputVertex,3> tri{};
            for(int j=0;j<3;++j){
                const auto& v=src[i+std::size_t(j)];
                tri[std::size_t(j)]={v.x,v.y,v.z,v.u,v.v,1.f,v.r,v.g,v.b,v.a};
            }
            appendTriangle(tri,camera,projection,batch);
        }
    };

    const auto& cells=game.field().cells();

    // r220: keep the literal 0x42043F..0x420542 submit families separate.
    // The prepared rim and SAFE top are texture-2 / ZFUNC=ALWAYS and must be
    // submitted before crawler pass #1 and before the non-SAFE floor.
    OutputBatch safeBatch;
    // r221 DIRECT EXE 0x2583948 -> 0x40C350: 76 source vertices and 38
    // independent four-index polygons. Preserve each polygon through the
    // legacy CPU clip/back-face stage before fan triangulation.
    {
        const auto rv=LegacyFieldFrontend::rimVertices();
        const auto rq=LegacyFieldFrontend::rimQuads();
        for(const auto& q:rq){
            std::array<InputVertex,4> p{};
            for(std::size_t i=0;i<p.size();++i){
                const auto& v=rv[q[i]];
                p[i]={v.x,v.y,v.z,v.u,v.v,v.light,1.f,1.f,1.f,1.f};
            }
            appendPolygon(p.data(),p.size(),camera,projection,safeBatch);
        }
    }
    for(const auto& r:LegacyFieldTessellators::safeRuns(cells)){
        const auto q=LegacyFieldTessellators::safeRunQuad(r);
        std::array<InputVertex,4> p{};
        for(std::size_t i=0;i<p.size();++i){
            const auto& v=q[i];
            p[i]={v.x,v.y,v.z,v.u,v.v,v.light,1.f,1.f,1.f,1.f};
        }
        appendPolygon(p.data(),p.size(),camera,projection,safeBatch);
    }
    convert(safeBatch,safeScreenVertices_,safeScreenIndices_);

    // r214 DIRECT EXE 0x41F270: each contiguous non-SAFE run is one four-vertex
    // polygon. X/Z come from camera-dependent 65-boundary arrays built by
    // 0x4201E0, U/V from their scaled companions, Y=0 and light=0x25B5B2C.
    OutputBatch floorBatch;
    for(const auto& r:LegacyFieldTessellators::nonSafeInteriorRuns(cells)){
        const auto x0=static_cast<std::size_t>(r.x0),x1=static_cast<std::size_t>(r.x1);
        const auto z0=static_cast<std::size_t>(r.y),z1=static_cast<std::size_t>(r.y+1);
        std::array<InputVertex,4> p{{
            {fg.x[x0],LegacyFieldTessellators::FloorY,fg.z[z0],fg.scaledX[x0],fg.scaledZ[z0],LegacyFieldFrontend::NonSafeLight,1.f,1.f,1.f,1.f},
            {fg.x[x0],LegacyFieldTessellators::FloorY,fg.z[z1],fg.scaledX[x0],fg.scaledZ[z1],LegacyFieldFrontend::NonSafeLight,1.f,1.f,1.f,1.f},
            {fg.x[x1],LegacyFieldTessellators::FloorY,fg.z[z1],fg.scaledX[x1],fg.scaledZ[z1],LegacyFieldFrontend::NonSafeLight,1.f,1.f,1.f,1.f},
            {fg.x[x1],LegacyFieldTessellators::FloorY,fg.z[z0],fg.scaledX[x1],fg.scaledZ[z0],LegacyFieldFrontend::NonSafeLight,1.f,1.f,1.f,1.f}
        }};
        appendPolygon(p.data(),p.size(),camera,projection,floorBatch);
    }
    convert(floorBatch,floorScreenVertices_,floorScreenIndices_);

    // Low transition edges are a later texture-0 / LEQUAL family.
    OutputBatch lowEdgeBatch;
    appendTexturedTriangles(floorWorldVertices_,lowEdgeBatch);
    convert(lowEdgeBatch,lowEdgeScreenVertices_,lowEdgeScreenIndices_);

    // r218/r220: high walls precede capture-marker tops in the original and
    // both inherit texture 2 with ZFUNC=LEQUAL.
    OutputBatch wallBatch;
    auto appendWallQuad=[&](const std::array<LegacyFieldTessellators::TopVertex,4>& q){
        std::array<InputVertex,4> p{};
        for(std::size_t i=0;i<p.size();++i){
            const auto& v=q[i];
            p[i]={v.x,v.y,v.z,v.u,v.v,v.light,1.f,1.f,1.f,1.f};
        }
        appendPolygon(p.data(),p.size(),camera,projection,wallBatch);
    };
    for(const auto& r:LegacyFieldTessellators::highVerticalRuns(cells))
        appendWallQuad(LegacyFieldTessellators::highVerticalRunQuad(r));
    for(const auto& e:LegacyFieldTessellators::highHorizontalTransitions(cells)){
        if(e.dir==LegacyFieldTessellators::EdgeDir::SafeToNonSafe)
            appendWallQuad(LegacyFieldTessellators::highHorizontalAQuad(e));
        else
            appendWallQuad(LegacyFieldTessellators::highHorizontalBQuad(e));
    }
    for(const auto& r:LegacyFieldTessellators::captureMarkerRuns(cells,game.field().markerTimers())){
        const auto q=LegacyFieldTessellators::staticTopRunQuad(r.y,r.x0,r.x1,r.height,LegacyFieldTessellators::SafeLight);
        appendWallQuad(q);
    }
    convert(wallBatch,wallScreenVertices_,wallScreenIndices_);

    // r193 DIRECT EXE correction: 0x42053D passes ONE four-vertex polygon
    // (0x4417D0) to 0x40D1E0 in BL->TL->TR->BR order. Do not pre-split this
    // fascia into two independently culled triangles: 0x40D1E0 clips and
    // front-face tests the polygon once, then emits its own triangle fan.
    {
        constexpr float xl=.38750001788139343f, xr=.6125000119209290f;
        constexpr float yb=-.004000000189989805f, yt=.00800000037997961f;
        constexpr float zb=.3840000033378601f, zt=.38750001788139343f;
        constexpr float vt=.8762500286102295f, vb=.9900000095367432f;
        constexpr float kXon1Light=.699999988079071f;
        std::array<InputVertex,4> fascia{{
            // r194 DIRECT EXE 0x420003..0x420016: init writes light .7
            // (0x3F333333) into all four records at 0x4417E4 + i*24.
            {xl,yb,zb,0.f,vb,kXon1Light,1.f,1.f,1.f,1.f},
            {xl,yt,zt,0.f,vt,kXon1Light,1.f,1.f,1.f,1.f},
            {xr,yt,zt,1.f,vt,kXon1Light,1.f,1.f,1.f,1.f},
            {xr,yb,zb,1.f,vb,kXon1Light,1.f,1.f,1.f,1.f}
        }};
        OutputBatch fasciaBatch;
        fasciaBatch.vertices.reserve(4);
        fasciaBatch.indices.reserve(6);
        appendPolygon(fascia.data(),fascia.size(),camera,projection,fasciaBatch);
        frontLogoScreenVertices_.clear();
        frontLogoScreenIndices_.clear();
        convert(fasciaBatch,frontLogoScreenVertices_,frontLogoScreenIndices_);
    }
}

void Renderer::syncLevelLoad(std::uint64_t serial){
    if(serial==observedLevelLoadSerial_)return;
    observedLevelLoadSerial_=serial;
    // DIRECT EXE 0x41916F..0x419196: per-level initializer writes 5000 to
    // all six pickup-smash timers. Particle bytes are left alone; with an age
    // >=2500 the native 0x418110 path neither updates nor submits that slot.
    for(auto& slot:pickupSmashSlots_)slot.ageMs=5000;
}

void Renderer::spawnPickupSmash(const PickupSmashEvent& event){
    // DIRECT EXE 0x418040: each pickup owns a fixed 128-record block. A new
    // smash of the same slot resets its timer and overwrites all 128 records;
    // it does NOT append another burst alongside the old one.
    const int slotIndex=std::clamp(event.type,0,5);
    auto& slot=pickupSmashSlots_[std::size_t(slotIndex)];
    slot.ageMs=0;
    for(std::size_t i=0;i<slot.particles.size();++i){
        const auto& q=event.particles[i];
        slot.particles[i]={event.worldX,event.height,event.worldZ,q.vx,q.vy,q.vz};
    }
}

void Renderer::rebuildLegacyModelBatch(const Game& game){
    using namespace LegacyScreenPipeline;
    modelVertices_.clear();modelIndices_.clear();xonixVertices_.clear();xonixIndices_.clear();directionalGlintVertices_.clear();directionalGlintIndices_.clear();shadowVertices_.clear();shadowIndices_.clear();reflectionVertices_.clear();reflectionIndices_.clear();particleVertices_.clear();particleIndices_.clear();auxiliaryVertices_.clear();auxiliaryIndices_.clear();deathOverlayVertices_.clear();deathOverlayIndices_.clear();crawlerPass1Vertices_.clear();crawlerPass1Indices_.clear();crawlerPass2Vertices_.clear();crawlerPass2Indices_.clear();interLevelCinematicVertices_.clear();interLevelCinematicIndices_.clear();levelIntroPlaqueVertices_.clear();levelIntroPlaqueIndices_.clear();levelIntroDigitsVertices_.clear();levelIntroDigitsIndices_.clear();
    OutputBatch batch;batch.vertices.reserve(8192);batch.indices.reserve(16384);
    OutputBatch shadowBatch;shadowBatch.vertices.reserve(1024);shadowBatch.indices.reserve(2048);
    OutputBatch reflectionBatch;reflectionBatch.vertices.reserve(4096);reflectionBatch.indices.reserve(8192);
    OutputBatch crawlerPass1Batch,crawlerPass2Batch,interLevelCinematicBatch,levelIntroPlaqueBatch,levelIntroDigitsBatch,xonixBatch,directionalGlintBatch; crawlerPass1Batch.vertices.reserve(1024);crawlerPass1Batch.indices.reserve(2048);crawlerPass2Batch.vertices.reserve(1024);crawlerPass2Batch.indices.reserve(2048);interLevelCinematicBatch.vertices.reserve(128);interLevelCinematicBatch.indices.reserve(256);levelIntroPlaqueBatch.vertices.reserve(64);levelIntroPlaqueBatch.indices.reserve(96);levelIntroDigitsBatch.vertices.reserve(16);levelIntroDigitsBatch.indices.reserve(24);xonixBatch.vertices.reserve(1024);xonixBatch.indices.reserve(2048);directionalGlintBatch.vertices.reserve(64);directionalGlintBatch.indices.reserve(96);
    const auto gc=(game.levelIntroActive() || game.levelEntryActive()) ? LegacyCamera::GameplayCameraState{game.startupCameraX(),game.startupCameraY(),game.startupCameraZ(),game.startupCameraAngle1(),game.startupCameraAngle2(),game.startupCameraAngle3()} : (((game.phase()==GamePhase::Dying)||(game.phase()==GamePhase::GameOver)) ? LegacyCamera::deathState(game.displayPlayerWorldX(),game.displayPlayerHeight(),game.displayPlayerWorldZ(),game.deathScene().triColorBurstStarted,game.displayCameraYawOffset()) : (game.phase()==GamePhase::InterLevel ? LegacyCamera::interLevelState(game.displayPlayerWorldX(),game.displayPlayerHeight(),game.displayPlayerWorldZ(),game.legacyCameraZoom(),game.displayCameraYawOffset()) : (game.phase()==GamePhase::FinalSequence ? LegacyCamera::finaleState(game.displayPlayerWorldX(),game.finalePresentationY(),game.displayPlayerWorldZ(),game.legacyCameraZoom(),game.displayCameraYawOffset()) : LegacyCamera::gameplayState(game.displayPlayerWorldX(),game.displayPlayerWorldZ(),game.legacyCameraZoom(),game.displayCameraYawOffset()))));CameraState camera{gc.x,gc.y,gc.z,gc.angle1,gc.angle2,gc.angle3};const auto projection=LegacyCamera::projectionState(640,480);
    LegacyReflection::CameraState reflectionCamera{gc.x,gc.y,gc.z,0.f,0.f};
    OutputBatch particleBatch; particleBatch.vertices.reserve(6u*LegacyParticles::PickupCount*4u); particleBatch.indices.reserve(6u*LegacyParticles::PickupCount*6u);
    OutputBatch auxiliaryBatch; auxiliaryBatch.vertices.reserve(128); auxiliaryBatch.indices.reserve(192);
    OutputBatch deathOverlayBatch; deathOverlayBatch.vertices.reserve(4); deathOverlayBatch.indices.reserve(6);
    // DIRECT EXE 0x418110: the six fixed pools have independent integer
    // timers. Critically, age<2500 is tested BEFORE adding this frame's dt;
    // e.g. age=2490,dt=20 still gets one final update+submit at age=2510.
    const int particleDt=game.paused()?0:std::clamp(gameFrameDt_,0,100);
    for(std::size_t slotIndex=0;slotIndex<pickupSmashSlots_.size();++slotIndex){
        auto& slot=pickupSmashSlots_[slotIndex];
        if(slot.ageMs>=LegacyParticles::PickupLifetimeMs)continue;
        slot.ageMs+=particleDt;
        for(auto& p:slot.particles){
            const float oldVy=p.vy;
            p.x+=p.vx*float(particleDt);
            p.y+=oldVy*float(particleDt);
            p.z+=p.vz*float(particleDt);
            p.vy=oldVy-float(particleDt)*LegacyParticles::PickupGravityPerMs;
        }
        const auto c=LegacyParticles::PickupPackedColor[slotIndex];
        const auto setup=LegacyBillboard::pickupSmash(640,c);
        for(const auto& p:slot.particles)
            LegacyBillboard::append(p.x,p.y,p.z,camera,projection,setup,particleBatch);
    }
    // 0x417700 -> 0x40E120(base,96): shared field erosion debris is a second
    // user of exactly the same TL-vertex billboard builder and the same atlas
    // rectangle/size formula, with fixed diffuse 0x00CFAF4F.
    const auto fieldSetup=LegacyBillboard::fieldDebris(640);
    if(game.displayFieldDebris()){
        // r365 DIRECT EXE 0x4177A9..0x4177B0 always submits base,96 to
        // 0x40E120. There is no active-record filter on the render path;
        // negative/zero records are rejected naturally by legacy projection.
        for(const auto& q:game.entities().fieldDebris())
            LegacyBillboard::append(q.x,q.y,q.z,camera,projection,fieldSetup,particleBatch);
    }
    // 0x4151C7..0x41532D: eraser impact debris is a third independent pool:
    // 128 records in a 16-at-a-time ring buffer. It uses the same atlas cell
    // and diffuse as field debris but the smaller legacy size multiplier.
    const auto eraserSetup=LegacyBillboard::eraserDebris(640);
    for(const auto& q:game.specialObjects().eraserDebris()){
        // 0x415323 calls 0x40E120(base,128) unconditionally; zeroed records
        // rely on the native projection/depth reject rather than an active bit.
        LegacyBillboard::append(q.x,q.y,q.z,camera,projection,eraserSetup,particleBatch);
    }
    // 0x417390/0x417470: lethal crawler contact emits a separate 512-record
    // burst. 0x417470 renders it through the same 0x40E120 TL-billboard path
    // with size = screenWidth*0.000625 and diffuse 0x00FF8F8F.
    if(game.deathBurstActive()){
        const auto deathSetup=LegacyBillboard::deathBurst(640);
        for(const auto& q:game.deathBurstParticles())
            LegacyBillboard::append(q.x,q.y,q.z,camera,projection,deathSetup,particleBatch);
    }
    // 0x418290: one contiguous 480-record death-only array split into three
    // 160-record colour windows. The exact initializer/update/render contract is
    // reconstructed; activation remains tied to the separately traced death Y phase.
    if(game.deathTriColorInitialized()){
        const auto& tri=game.deathTriColorParticles();
        for(int group=0;group<LegacyParticles::DeathTriColorGroupCount;++group){
            const auto triSetup=LegacyBillboard::deathTriColor(
                640,LegacyParticles::DeathTriColorPackedColor[std::size_t(group)]);
            const int first=group*LegacyParticles::DeathTriColorGroupSize;
            const int last=first+LegacyParticles::DeathTriColorGroupSize;
            for(int i=first;i<last;++i){
                const auto& q=tri[std::size_t(i)];
                LegacyBillboard::append(q.x,q.y,q.z,camera,projection,triSetup,particleBatch);
            }
        }
    }
    auto appendReflection=[&](const LegacyMesh& mesh,const LegacyTransform::Matrix34& model,float tx,float ty,float tz,float requestedIntensity){
        const auto rv=LegacyReflection::buildVertices(mesh,model,tx,ty,tz,reflectionCamera);
        const float brightness=LegacyReflection::queuedBrightness(requestedIntensity);
        for(const auto& face:mesh.faces){
            if(face.index.size()!=3 && face.index.size()!=4)continue;
            std::array<InputVertex,4> p{};bool valid=true;
            for(std::size_t i=0;i<face.index.size();++i){const auto idx=face.index[i];if(idx>=rv.size()){valid=false;break;}const auto& v=rv[idx];p[i]={v.x,v.y,v.z,v.u,v.v,brightness,1.f,1.f,1.f,1.f};}
            if(valid)appendPolygon(p.data(),face.index.size(),camera,projection,reflectionBatch);
        }
    };
    // r339 DIRECT EXE 0x415880..0x415ABC: all six auxiliary event plaques
    // are live world geometry on logical texture 7.  The simulation has already
    // advanced Y for this frame; the render helper then pitches each plaque
    // toward the current camera in the Y/Z plane and submits it at its XYZ.
    for(std::size_t i=0;i<game.auxiliaryEffects().size();++i){
        const auto& a=game.auxiliaryEffects()[i];
        if(!a.active())continue;
        LegacyTransform::Matrix34 m=LegacyTransform::identity();
        LegacyTransform::rotateXRad(m,legacyAuxiliaryPitch(camera.y,camera.z,a.y,a.z));
        appendLegacyMesh(auxiliaryModels_[i],m,a.x,a.y,a.z,camera,projection,auxiliaryBatch);
    }

    // r340 DIRECT EXE 0x41BDB0/0x41BEE0: expanding additive impact quad.
    // The legacy render blob has four XYZ/UV/light records and one quad.
    if(game.deathOverlay().active){
        const auto& o=game.deathOverlay();
        const float h=o.halfExtent;
        LegacyMesh q;
        q.vertices={
            {-h,0.f, h,0.f,1.f,0.f,193.f/256.f,161.f/256.f},
            { h,0.f, h,0.f,1.f,0.f,255.f/256.f,161.f/256.f},
            { h,0.f,-h,0.f,1.f,0.f,255.f/256.f,223.f/256.f},
            {-h,0.f,-h,0.f,1.f,0.f,193.f/256.f,223.f/256.f}
        };
        q.faces.push_back({{0u,1u,2u,3u}});
        const auto id=LegacyTransform::identity();
        appendLegacyMesh(q,id,o.x,o.y,o.z,camera,projection,deathOverlayBatch,
                         o.r/255.f,o.g/255.f,o.b/255.f,1.f,o.intensity);
    }

    constexpr float step=.003125f,base=.4f;

    // r116 direct EXE: 0x41ADCF selects logical texture 4, then
    // 0x41AE13..0x41AEAC renders cinematic slots 5 and 0 as a mirrored pair.
    // 0x40E5C0 is radian X rotation and 0x40E6A0 is radian Z rotation.
    if(game.paused()){
        // r135 direct EXE: 0x41E080..0x41E0AD transforms model slot 2 / PAUS
        // and submits prepared slot 2 at X=0, Y=panelY, Z=0.009.
        LegacyTransform::Matrix34 mp=LegacyTransform::identity();
        const int angle=static_cast<int>(game.pauseScene().rotorPhase*(2048.f/(2.f*3.14159265358979323846f)));
        LegacyTransform::rotateZ(mp,angle);
        // DIRECT EXE 0x41E071..0x41E07B: PAUS does not use the gameplay
        // camera. 0x40C250 receives (0,0,0, 0,-500,0) immediately before the
        // slot-2 transform/submit. Projecting it through the field camera made
        // the modal panel miss the viewport on hardware.
        const LegacyScreenPipeline::CameraState pauseCamera{0.f,0.f,0.f,0,-500,0};
        appendLegacyMesh(pauseCinematicSlot2_,mp,0.f,game.pauseScene().panelY,
                         0.008999999612569809f,pauseCamera,projection,interLevelCinematicBatch);
    }
    if(game.phase()==GamePhase::Abort){
        // r133 direct EXE: 0x41E7A6 selects logical texture 4; prepared slot 4
        // / ABOR is submitted at 0x41E7C6 with X=0, Z=0 and Y=panelY*1.1.
        LegacyTransform::Matrix34 ma=LegacyTransform::identity();
        // DIRECT EXE 0x41E7AD..0x41E7B7: ABOR owns the same presentation
        // camera family as PAUS: 0x40C250(0,0,0, 0,-500,0). Using the
        // gameplay camera in r133-r174 made the prompt logically active but
        // projected outside the visible presentation plane.
        const LegacyScreenPipeline::CameraState abortCamera{0.f,0.f,0.f,0,-500,0};
        appendLegacyMesh(abortCinematicSlot4_,ma,0.f,game.abortConfirm().panelY*1.100000023841858f,
                         0.f,abortCamera,projection,interLevelCinematicBatch);
    }
    if(game.phase()==GamePhase::GameOver){
        // r132 direct EXE: 0x41CA48 selects logical texture 4 and submits
        // cinematic slot 3 / GOVE through 0x40C350 at X=0, Z=.003. The Y
        // translation is the frozen post-impact X drift computed at burst time:
        // (0.5015625358-impactX)*0.000699999975. In native state that exact
        // value is deathScene.velocityX and remains unchanged during the tail.
        LegacyTransform::Matrix34 mg=LegacyTransform::identity();
        // r204 DIRECT EXE 0x41CA63..0x41CA6D: GOVE owns a dedicated
        // presentation camera (0,0,0; 0,-510,0). The death camera is restored
        // immediately after this submit for the live world behind the plaque.
        const CameraState gameOverCamera{
            LegacyGameOver::Trace::cameraX,LegacyGameOver::Trace::cameraY,LegacyGameOver::Trace::cameraZ,
            LegacyGameOver::Trace::cameraAngle1,LegacyGameOver::Trace::cameraAngle2,LegacyGameOver::Trace::cameraAngle3};
        appendLegacyMesh(gameOverCinematicSlot3_,mg,0.f,game.gameOverScene().visual.velocityX,
                         LegacyGameOver::Trace::submitZ,gameOverCamera,projection,interLevelCinematicBatch);
    }
    if(game.phase()==GamePhase::FinalSequence){
        // r142 direct EXE: 0x41B94C..0x41BA3B renders the same cinematic
        // slot5/slot0 model pair as inter-level, but with finale-specific
        // amplitudes and Z placement. The original Y argument is an elapsed-ms
        // integer reinterpreted as float, a denormal visually equivalent to 0.
        const auto& fs=game.finalScene();
        const float rx=fs.cinematicRotX(),rz=fs.cinematicRotZ();
        LegacyTransform::Matrix34 m5=LegacyTransform::identity();
        LegacyReflection::rotateX(m5,rx);
        LegacyReflection::rotateZ(m5,rz);
        appendLegacyMesh(interLevelCinematicSlot5_,m5,0.f,
                         LegacyFinaleCinematic::Trace::nativeEquivalentY,
                         LegacyFinaleCinematic::Trace::slot5Z,camera,projection,interLevelCinematicBatch);
        LegacyTransform::Matrix34 m0=LegacyTransform::identity();
        LegacyReflection::rotateX(m0,-rx);
        LegacyReflection::rotateZ(m0,-rz);
        appendLegacyMesh(interLevelCinematicSlot0_,m0,0.f,
                         LegacyFinaleCinematic::Trace::nativeEquivalentY,
                         LegacyFinaleCinematic::Trace::slot0Z,camera,projection,interLevelCinematicBatch);
    }
    if(game.phase()==GamePhase::InterLevel){
        const auto& il=game.interLevelScene();
        const float rx=il.cinematicRotX(),rz=il.cinematicRotZ();
        LegacyTransform::Matrix34 m5=LegacyTransform::identity();
        LegacyReflection::rotateX(m5,rx);
        LegacyReflection::rotateZ(m5,rz);
        // DIRECT EXE 0x41ADCF..0x41AEAC: the GAME/COMP plaque pair owns
        // a dedicated camera (0,0,0; 0,-512,0), not the gameplay camera.
        const CameraState plaqueCamera{0.f,0.f,0.f,0,-512,0};
        appendLegacyMesh(interLevelCinematicSlot5_,m5,0.f,il.cinematicYOffset,
                         LegacyInterLevel::Trace::cinematicSlot5Z,plaqueCamera,projection,interLevelCinematicBatch);
        LegacyTransform::Matrix34 m0=LegacyTransform::identity();
        LegacyReflection::rotateX(m0,-rx);
        LegacyReflection::rotateZ(m0,-rz);
        appendLegacyMesh(interLevelCinematicSlot0_,m0,0.f,il.cinematicYOffset,
                         LegacyInterLevel::Trace::cinematicSlot0Z,plaqueCamera,projection,interLevelCinematicBatch);
    }

    // r287 DIRECT EXE 0x41DAB0..0x41DB66: startup level plaque uses a
    // dedicated (0,0,0;0,-512,0) camera. LEV2 is prepared slot 1 under
    // logical texture 7; CNT3 is the dynamic decimal mesh under texture 4.
    if(game.levelIntroActive()){
        const auto& li=game.levelIntroScene();
        const int levelNumber=static_cast<int>(game.levelIndex()+1);
        const CameraState introCamera{0.f,0.f,0.f,0,-512,0};
        LegacyTransform::Matrix34 mi=LegacyTransform::identity();
        // r344: 0x41D70C/0x41D76E applies an in-place Z rotation to the LEV2
        // master before lighting. CNT3 digits remain unrotated.
        LegacyTransform::rotateZRad(mi,li.plaqueRotationRad());
        appendLegacyMesh(levelIntroCinematicSlot1_,mi,li.plaqueX(levelNumber),li.plaqueY,0.f,
                         introCamera,projection,levelIntroPlaqueBatch);
        const int digitCount=levelNumber<10?1:2;
        const auto dm=LegacyCnt3Digits::build(levelNumber,digitCount);
        LegacyMesh digitMesh; digitMesh.vertices.reserve(dm.vertices.size()); digitMesh.faces.reserve(dm.quads.size());
        for(const auto& v:dm.vertices){
            LegacyMasterVertex mv{}; mv.x=v.x;mv.y=v.y;mv.z=v.z;mv.ny=1.f;mv.u=v.u;mv.v=v.v;
            digitMesh.vertices.push_back(mv);
        }
        for(const auto& q:dm.quads) digitMesh.faces.push_back({{q[0],q[1],q[2],q[3]}});
        const LegacyTransform::Matrix34 digitMatrix=LegacyTransform::identity();
        appendLegacyMesh(digitMesh,digitMatrix,li.digitX(levelNumber),li.digitY(),LevelIntroState::digitZ,
                         introCamera,projection,levelIntroDigitsBatch,1.f,1.f,1.f,1.f,LegacyCnt3Digits::Light);
    }

    // Xonix compound model.
    LegacyTransform::Matrix34 body=LegacyTransform::identity();
    appendLegacyMesh(xonixModel_.body,body,game.displayPlayerWorldX(),game.displayPlayerHeight(),game.displayPlayerWorldZ(),camera,projection,xonixBatch);
    LegacyTransform::Matrix34 prop=LegacyTransform::identity();
    const int propAngle=static_cast<int>(game.displayPlayerPropellerPhase()*(2048.f/(2.f*3.14159265358979323846f)));
    LegacyTransform::rotateY(prop,propAngle);
    appendLegacyMesh(xonixModel_.propeller,prop,game.displayPlayerWorldX(),game.displayPlayerHeight(),game.displayPlayerWorldZ(),camera,projection,xonixBatch);
    const float rr=game.displayPlayerRotorRadius(),phase=game.displayPlayerRotorPhase();
    for(int i=0;i<4;++i){
        const float a=phase+float(i)*1.5707963267948966f;
        const float tipX=game.displayPlayerWorldX()+std::cos(a)*rr;
        const float tipY=game.displayPlayerHeight()+LegacyModels::Xonix.rotorYOffset;
        const float tipZ=game.displayPlayerWorldZ()+std::sin(a)*rr;
        LegacyTransform::Matrix34 n=LegacyTransform::identity();
        appendLegacyMesh(xonixModel_.rotorNode,n,tipX,tipY,tipZ,camera,projection,xonixBatch);
    }

    // 0x4213A0 + 0x41A73F..0x41A80D: exact active trail segment models.
    // Native TrailCell layout is {x,progress,y,hazard}; the x86 renderer uses
    // world base .40156, .003125 cell step and lowers each segment by
    // progress*.00008 while progress saturates at 100.
    for(const auto& t:game.player().trail()){
        LegacyTransform::Matrix34 m=LegacyTransform::identity();
        const float x=.4015600085f+float(t.x)*.003125f;
        const float z=.4015600085f+float(t.y)*.003125f;
        // DIRECT EXE 0x41A73F..0x41A80D: submit origin is literal .008 for
        // normal and .010 for damaged trail, then progress*.00008 is removed.
        // Do not compensate for the local mesh profile here; that changes the
        // original world transform and was the source of the r161-r169 drift.
        const float baseY=t.hazard?0.010000000707805157f:0.00800000037997961f;
        const float y=baseY-float(t.progress)*0.00007999999821186066f;
        // Hazard propagation is visibly red in the original; tint the damaged
        // trail family instead of relying on the atlas tile alone.
        if(t.hazard) appendLegacyMesh(trailDamagedModel_,m,x,y,z,camera,projection,batch,1.f,0.12f,0.08f,1.f);
        else appendLegacyMesh(trailNormalModel_,m,x,y,z,camera,projection,batch);
    }

    for(const auto& e:game.entities().air()){
        const float ex=base+e.x*step,ez=base+e.y*step;
        // r46+r139 DIRECT EXE: 0x422DE0 uses logical texture handle 0 with
        // world-aligned UV and reduced diffuse intensity. The gameplay theme
        // table is stored {slot1,slot0,slot2}; reference theme #7 therefore
        // routes 0212 to handle 0. SHAD belongs to atlas/texture slot 3.
        LegacyMesh shadowMesh=airEnemyShadowModel_;
        for(auto& v:shadowMesh.vertices){
            v.u=(v.x+ex-LegacyModels::AirEnemyShadow.uvWorldBias)*LegacyModels::AirEnemyShadow.uvWorldScale;
            v.v=(v.z+ez-LegacyModels::AirEnemyShadow.uvWorldBias)*LegacyModels::AirEnemyShadow.uvWorldScale;
        }
        LegacyTransform::Matrix34 shadow=LegacyTransform::identity();
        appendLegacyMesh(shadowMesh,shadow,ex,0.f,ez,camera,projection,shadowBatch,
                         1.f,1.f,1.f,1.f,LegacyModels::AirEnemyShadow.initialLightScalar);
        // CONFIRMED r41 from 0x41A43E..0x41A49C: airborne-enemy model
        // orientation is a conjugated tilt, not the r40 Y-only heading.
        // 0x401380 = Y rotation, 0x401400 = Z rotation.
        LegacyTransform::Matrix34 m=LegacyTransform::identity();
        LegacyTransform::rotateY(m,-e.heading2048);
        LegacyTransform::rotateZ(m,e.angularAccumulator);
        LegacyTransform::rotateY(m,e.heading2048);
        appendLegacyMesh(airEnemyModels_[std::size_t(std::clamp(e.subtype,0,3))],m,ex,LegacyModels::AirEnemySubmitY,ez,camera,projection,batch);
    }
    for(const auto& e:game.entities().ground()){
        // r348 DIRECT EXE 0x418840/0x418A90 -> 0x41ECC0: these are two
        // world-aligned FLOOR PROJECTIONS, not two translucent crawler bodies.
        // Preserve the original 28-byte record and vertical phase. During
        // descent use the current grid position for visible X/Z to avoid the
        // stale cached world-coordinate snap on landing. Projection Y remains
        // fixed by 0x41ECC0.
        const auto& gp=LegacyModels::GroundEnemyPresentation;
        const float crawlerX=e.presentationWorldX(),crawlerZ=e.presentationWorldZ();
        appendCrawlerProjection(groundEnemyModel_,crawlerX,e.respawnDelay+gp.firstTransformYOffset,crawlerZ,
                                gp.firstPostTransformYOffset,gp.firstLightScalar,camera,projection,crawlerPass1Batch);
        appendCrawlerProjection(groundEnemyModel_,crawlerX,e.respawnDelay+gp.secondTransformYOffset,crawlerZ,
                                gp.secondPostTransformYOffset,gp.secondLightScalar,camera,projection,crawlerPass2Batch);

        // r348 DIRECT EXE 0x41A3E7..0x41A428: the actual visible crawler is a
        // separate ordinary 0x40C350 submit at recordY+.013. Logical texture 3
        // is inherited here, which supplies the same pink XONI material seen in
        // the menu crawler. No alpha blend and no per-instance rotation.
        LegacyTransform::Matrix34 body=LegacyTransform::identity();
        appendLegacyMesh(groundEnemyModel_,body,crawlerX,e.respawnDelay+gp.bodyYOffset,crawlerZ,
                         camera,projection,batch);
    }
    // r251 DIRECT EXE 0x41A4E6..0x41A5C0: one caller-owned additive
    // 0x4055E0 pass follows the opaque airborne/crawler/Xonix model work.
    // Its literal order is airborne -> crawler -> four Xonix rotor tips.
    // 0x4055C0 receives the current camera XYZ once before these calls.
    auto appendDirectionalGlint=[&](float x,float y,float z,float size){
        const auto fan=LegacyDirectionalGlint::build(camera.x,camera.y,camera.z,x,y,z,size);
        for(const auto& f:LegacyDirectionalGlint::Fan){
            std::array<InputVertex,3> tri{};
            for(std::size_t k=0;k<3;++k){
                const auto& v=fan[f[k]];
                tri[k]={v.x,v.y,v.z,v.u,v.v,v.light,1.f,1.f,1.f,1.f};
            }
            appendTriangle(tri,camera,projection,directionalGlintBatch);
        }
    };
    for(const auto& e:game.entities().air()){
        // 0x41A519..0x41A52A walks the visible-coordinate tail of each
        // 0x50-byte record: cached world X/Z and literal Y=0.005.
        appendDirectionalGlint(e.worldX,LegacyDirectionalGlint::Trace::airborneY,e.worldZ,
                               LegacyDirectionalGlint::Trace::airborneSize);
    }
    for(const auto& e:game.entities().ground()){
        // 0x254D8F8 points at crawler record +0x10: respawn/vertical phase,
        // then cached world X/Z at +0x14/+0x18. 0x41A54F adds 0.013 to +0x10.
        appendDirectionalGlint(e.presentationWorldX(),e.respawnDelay+LegacyDirectionalGlint::Trace::crawlerYOffset,e.presentationWorldZ(),
                               LegacyDirectionalGlint::Trace::crawlerSize);
    }
    for(int i=0;i<4;++i){
        const float a=phase+float(i)*1.5707963267948966f;
        appendDirectionalGlint(game.displayPlayerWorldX()+std::cos(a)*rr,
                               game.displayPlayerHeight()+LegacyModels::Xonix.rotorYOffset,
                               game.displayPlayerWorldZ()+std::sin(a)*rr,
                               LegacyDirectionalGlint::Trace::xonixSize);
    }

    const auto& special=game.specialObjects();
    if(special.homing().active){
        const auto& h=special.homing();
        // 0x4155BE..0x4155F6 and r42 decode of 0x401160: the old helper
        // rotates X/Z vertex pairs around Y using fsincos(radians). Applying
        // the same phase as an object-space Y matrix is semantically exact.
        LegacyTransform::Matrix34 spin=LegacyTransform::identity();
        const int angle=static_cast<int>(h.spinPhase*(2048.f/(2.f*3.14159265358979323846f)));
        LegacyTransform::rotateY(spin,angle);
        appendLegacyMesh(homingModel_.body,spin,h.worldX,h.height,h.worldZ,camera,projection,batch);
        // r68 DIRECT EXE: 0x41446B..0x41448D queues the homing main body
        // through 0x40E710 with requested intensity 0.18. The later dome
        // does not share this proven queue call, so only the compound body
        // receives the environment-map overlay here.
        appendReflection(homingModel_.body,spin,h.worldX,h.height,h.worldZ,LegacyReflection::HomingRequestedIntensity);

        // 0x415656..0x415688: the second finalized mesh bobs independently.
        LegacyTransform::Matrix34 dome=LegacyTransform::identity();
        const float domeY=h.height+0.0017f+std::cos(h.soundPhase)*0.001f;
        appendLegacyMesh(homingModel_.dome,dome,h.worldX,domeY,h.worldZ,camera,projection,batch);
    }

    if(special.eraser().active){
        const auto& e=special.eraser();
        // r42 exact trace 0x41535C..0x415428: the eraser radial component
        // has independent rotation and radial phase. The legacy angle advances
        // by 2*dt, while the radians phase drives both scale and vertical bob.
        LegacyTransform::Matrix34 spin=LegacyTransform::identity();
        LegacyTransform::rotateY(spin,e.rotationAngle2048);
        LegacyTransform::setScale(spin,LegacyModels::EraserPresentation.scaleBase +
                                  LegacyModels::EraserPresentation.scaleAmplitude*std::cos(e.spinPhase));
        const float starY=LegacyModels::EraserPresentation.bobBaseY +
                          LegacyModels::EraserPresentation.bobAmplitudeY*std::sin(e.spinPhase);
        appendLegacyMesh(eraserModel_.star,spin,e.worldX,starY,e.worldZ,camera,projection,batch);
        LegacyTransform::Matrix34 core=LegacyTransform::identity();
        appendLegacyMesh(eraserModel_.core,core,e.worldX,LegacyModels::EraserPresentation.bobBaseY,e.worldZ,camera,projection,batch);
    }

    const auto& ps=game.pickups().items();
    for(std::size_t i=0;i<ps.size();++i){
        const auto& p=ps[i];
        if(p.timerMs>0)continue;
        LegacyTransform::Matrix34 m=LegacyTransform::identity();
        const int spin=game.pickups().presentationAngle2048();
        if(i==2u){
            // 0x417F4B..0x417F9F: heart has its own transform. It does NOT get
            // the common +90-degree X pitch; it rotates at half the shared
            // angle and pulses between scale 0.2 and 1.2.
            LegacyTransform::rotateY(m,spin/2);
            const int pulseIndex=((spin*3)>>2)&0x3ff;
            const float pulseAngle=float(pulseIndex)*(2.f*3.14159265358979323846f/1024.f);
            LegacyTransform::setScale(m,LegacyModels::PickupPresentation.heartScaleBase +
                                       LegacyModels::PickupPresentation.heartScaleAmplitude*std::sin(pulseAngle));
        }else{
            // 0x417D62..0x417DD4: one common matrix for P0/P1/P3/P4/P5.
            LegacyTransform::rotateX(m,LegacyModels::PickupPresentation.fixedPitch2048);
            LegacyTransform::rotateY(m,spin);
        }
        appendLegacyMesh(pickupModels_[i],m,p.worldX,p.height+LegacyModels::PickupSubmitYOffset,p.worldZ,camera,projection,batch,1.f,1.f,1.f,1.f,i==4u?1.f:-1.f);
        // 0x417E12..0x417FE6: all six pickup masters queue a deferred
        // 0x40E710 reflection pass. Requested intensities are exact call-site
        // literals; 0x40E710 itself applies the recovered *0.6 factor.
        appendReflection(pickupModels_[i],m,p.worldX,p.height+LegacyModels::PickupSubmitYOffset,p.worldZ,LegacyReflection::PickupRequestedIntensity[i]);
    }

    reflectionVertices_.reserve(reflectionBatch.vertices.size());
    for(const auto& v:reflectionBatch.vertices)reflectionVertices_.push_back({v.clipX,v.clipY,v.clipZ,v.clipW,v.u,v.v,v.light,v.r,v.g,v.b,v.a});
    reflectionIndices_=std::move(reflectionBatch.indices);
    shadowVertices_.reserve(shadowBatch.vertices.size());
    for(const auto& v:shadowBatch.vertices)shadowVertices_.push_back({v.clipX,v.clipY,v.clipZ,v.clipW,v.u,v.v,v.light,v.r,v.g,v.b,v.a});
    shadowIndices_=std::move(shadowBatch.indices);
    crawlerPass1Vertices_.reserve(crawlerPass1Batch.vertices.size());
    for(const auto& v:crawlerPass1Batch.vertices)crawlerPass1Vertices_.push_back({v.clipX,v.clipY,v.clipZ,v.clipW,v.u,v.v,v.light,v.r,v.g,v.b,v.a});
    crawlerPass1Indices_=std::move(crawlerPass1Batch.indices);
    crawlerPass2Vertices_.reserve(crawlerPass2Batch.vertices.size());
    for(const auto& v:crawlerPass2Batch.vertices)crawlerPass2Vertices_.push_back({v.clipX,v.clipY,v.clipZ,v.clipW,v.u,v.v,v.light,v.r,v.g,v.b,v.a});
    crawlerPass2Indices_=std::move(crawlerPass2Batch.indices);
    interLevelCinematicVertices_.reserve(interLevelCinematicBatch.vertices.size());
    for(const auto& v:interLevelCinematicBatch.vertices)interLevelCinematicVertices_.push_back({v.clipX,v.clipY,v.clipZ,v.clipW,v.u,v.v,v.light,v.r,v.g,v.b,v.a});
    interLevelCinematicIndices_=std::move(interLevelCinematicBatch.indices);
    levelIntroPlaqueVertices_.reserve(levelIntroPlaqueBatch.vertices.size());
    for(const auto& v:levelIntroPlaqueBatch.vertices)levelIntroPlaqueVertices_.push_back({v.clipX,v.clipY,v.clipZ,v.clipW,v.u,v.v,v.light,v.r,v.g,v.b,v.a});
    levelIntroPlaqueIndices_=std::move(levelIntroPlaqueBatch.indices);
    levelIntroDigitsVertices_.reserve(levelIntroDigitsBatch.vertices.size());
    for(const auto& v:levelIntroDigitsBatch.vertices)levelIntroDigitsVertices_.push_back({v.clipX,v.clipY,v.clipZ,v.clipW,v.u,v.v,v.light,v.r,v.g,v.b,v.a});
    levelIntroDigitsIndices_=std::move(levelIntroDigitsBatch.indices);
    xonixVertices_.reserve(xonixBatch.vertices.size());for(const auto& v:xonixBatch.vertices)xonixVertices_.push_back({v.clipX,v.clipY,v.clipZ,v.clipW,v.u,v.v,v.light,v.r,v.g,v.b,v.a});xonixIndices_=std::move(xonixBatch.indices);
    directionalGlintVertices_.reserve(directionalGlintBatch.vertices.size());for(const auto& v:directionalGlintBatch.vertices)directionalGlintVertices_.push_back({v.clipX,v.clipY,v.clipZ,v.clipW,v.u,v.v,v.light,v.r,v.g,v.b,v.a});directionalGlintIndices_=std::move(directionalGlintBatch.indices);
    modelVertices_.reserve(batch.vertices.size());for(const auto& v:batch.vertices)modelVertices_.push_back({v.clipX,v.clipY,v.clipZ,v.clipW,v.u,v.v,v.light,v.r,v.g,v.b,v.a});modelIndices_=std::move(batch.indices);
    particleVertices_.reserve(particleBatch.vertices.size());for(const auto& v:particleBatch.vertices)particleVertices_.push_back({v.clipX,v.clipY,v.clipZ,v.clipW,v.u,v.v,v.light,v.r,v.g,v.b,v.a});particleIndices_=std::move(particleBatch.indices);
    auxiliaryVertices_.reserve(auxiliaryBatch.vertices.size());for(const auto& v:auxiliaryBatch.vertices)auxiliaryVertices_.push_back({v.clipX,v.clipY,v.clipZ,v.clipW,v.u,v.v,v.light,v.r,v.g,v.b,v.a});auxiliaryIndices_=std::move(auxiliaryBatch.indices);
    deathOverlayVertices_.reserve(deathOverlayBatch.vertices.size());for(const auto& v:deathOverlayBatch.vertices)deathOverlayVertices_.push_back({v.clipX,v.clipY,v.clipZ,v.clipW,v.u,v.v,v.light,v.r,v.g,v.b,v.a});deathOverlayIndices_=std::move(deathOverlayBatch.indices);
}

void Renderer::rebuildMenuBackgroundBatch(){
    menuBg0Vertices_.clear();menuBg0Indices_.clear();menuBg1Vertices_.clear();menuBg1Indices_.clear();
    // r350: 0x411D60/0x412150 consume the same global frame delta
    // (0x025450C8) as the owning screen loop. Do not sample SDL_GetTicks() a
    // second time here: that desynchronises the large XONIX scene under load.
    const int dt=std::clamp(gameFrameDt_,0,100);
    menuFrameDt_=dt;
    menuPrimaryPhase_=kLegacyMenuBackgroundAnimationTrace.advancePrimary(menuPrimaryPhase_,dt);
    menuSecondaryPhase_=kLegacyMenuBackgroundAnimationTrace.advanceSecondary(menuSecondaryPhase_,menuPrimaryPhase_);

    const LegacyScreenPipeline::CameraState camera{0.f,0.f,0.f,0,0,0};
    const auto projection=LegacyCamera::projectionState(640,480);
    auto build=[&](const LegacyMenuPreparedLayerTrace& layer,std::vector<ScreenVertex>& outV,std::vector<std::uint16_t>& outI){
        std::array<LegacyScreenPipeline::InputVertex,4> q{};
        for(std::size_t i=0;i<4;++i){const auto& v=layer.vertices[i];
            q[i]={v.x+kLegacyMenuBackgroundAnimationTrace.submitX,
                  v.y+kLegacyMenuBackgroundAnimationTrace.submitY,
                  v.z+kLegacyMenuBackgroundAnimationTrace.submitZ,
                  v.u,v.v,v.light,1.f,1.f,1.f,1.f};
        }
        LegacyScreenPipeline::OutputBatch batch;
        if(!LegacyScreenPipeline::appendPolygon(q.data(),q.size(),camera,projection,batch))return;
        outV.reserve(batch.vertices.size());
        for(const auto& v:batch.vertices)outV.push_back({v.clipX,v.clipY,v.clipZ,v.clipW,v.u,v.v,v.light,v.r,v.g,v.b,v.a});
        outI=batch.indices;
    };
    build(makeLegacyMenuLayer0(menuPrimaryPhase_),menuBg0Vertices_,menuBg0Indices_);
    build(makeLegacyMenuLayer1(menuSecondaryPhase_),menuBg1Vertices_,menuBg1Indices_);
}


void Renderer::rebuildMainMenuDecorationBatch(const Game& game,bool mainMenuCaller){
    using namespace LegacyScreenPipeline;
    menuLogoVertices_.clear();menuLogoIndices_.clear();menuLaxyVertices_.clear();menuLaxyIndices_.clear();menuItemVertices_.clear();menuItemIndices_.clear();
    menuDecorationVertices_.clear();menuDecorationIndices_.clear();menuDecorationTexture7Vertices_.clear();menuDecorationTexture7Indices_.clear();menuDecorationReflectionVertices_.clear();menuDecorationReflectionIndices_.clear();menuSelectorVertices_.clear();menuSelectorIndices_.clear();

    // r166 DIRECT EXE: 0x411EF0 and 0x412150 have independent persistent
    // animation state. Do not derive these phases from the 0x411D60 scrolling
    // background phases.
    LegacyMainMenuDecorationTrace::TitleState ts{menuDecorState_.titleDrop,menuDecorState_.titleReveal,menuDecorState_.titlePhase,menuDecorState_.titleWait,menuDecorState_.titleChime};
    // r198 DIRECT EXE ordering: 0x411D60 runs before 0x411EF0.  Therefore the
    // M1 entry camera stays at its static -0.5 initializer for the whole
    // Alawar-only phase; it starts moving only on the frame AFTER reveal
    // becomes non-zero.  r166..r197 advanced it from frame zero, exposing the
    // menu behind the rocking publisher logo.
    menuDecorState_.entryPhase=LegacyMainMenuDecorationTrace::advanceEntryPhase(
        menuDecorState_.entryPhase,ts,menuFrameDt_);
    // 0x411EF0 belongs only to the M1 caller. Settings also executes 0x411D60
    // and 0x412150, but never advances the title state.
    if(mainMenuCaller){
        LegacyMainMenuDecorationTrace::advanceTitle(ts,menuFrameDt_);
        menuDecorState_.titleDrop=ts.drop;menuDecorState_.titleReveal=ts.reveal;menuDecorState_.titlePhase=ts.phase;menuDecorState_.titleWait=ts.waitMs;menuDecorState_.titleChime=ts.chimeArmed;
    }
    LegacyMainMenuDecorationTrace::SceneState ss{menuDecorState_.sceneA,menuDecorState_.sceneB,menuDecorState_.sceneC,menuDecorState_.sceneAngle,menuDecorState_.sceneTimer};
    const auto d=LegacyMainMenuDecorationTrace::advanceScene(ss,menuFrameDt_);
    menuDecorState_.sceneA=ss.a;menuDecorState_.sceneB=ss.b;menuDecorState_.sceneC=ss.c;menuDecorState_.sceneAngle=ss.angle2048;menuDecorState_.sceneTimer=ss.timer;

    const auto projection=LegacyCamera::projectionState(640,480);
    auto copyBatch=[&](const OutputBatch& in,std::vector<ScreenVertex>& ov,std::vector<std::uint16_t>& oi){
        ov.reserve(in.vertices.size());for(const auto& v:in.vertices)ov.push_back({v.clipX,v.clipY,v.clipZ,v.clipW,v.u,v.v,v.light,v.r,v.g,v.b,v.a});oi=in.indices;
    };

    // r175 closes the r60/r61 temporary 2-D approximation. 0x412F90 submits
    // M101/M102/M103/M104/M106 as five world-space 0x403860 planes through
    // the animated M1 camera. At phase=0 this projects to the exact rectangles
    // already locked by r61; during -0.5 -> 0 it naturally performs the
    // original camera fly-in rather than leaving the entries frozen onscreen.
    if(mainMenuCaller){
        const CameraState itemCamera{kLegacyMainMenuCameraTrace.steadyCameraX,
            kLegacyMainMenuCameraTrace.steadyCameraY-kLegacyMainMenuCameraTrace.cameraYPhaseScale*menuDecorState_.entryPhase,
            kLegacyMainMenuCameraTrace.steadyCameraZ,
            kLegacyMainMenuCameraTrace.steadyAngle1,kLegacyMainMenuCameraTrace.steadyAngle2,kLegacyMainMenuCameraTrace.steadyAngle3};
        OutputBatch items;
        constexpr float hz=kLegacyMainMenuCameraTrace.itemHalfHeight;
        constexpr float hx=kLegacyMainMenuCameraTrace.itemHalfWidth;
        constexpr float atlasH=256.f;
        constexpr int sy[5]={0,48,96,144,192};
        for(int i=0;i<5;++i){
            const float scale=game.mainMenu().scale[std::size_t(i)];
            const float light=game.mainMenu().brightness[std::size_t(i)];
            const float z=kLegacyMainMenuItemPresentationTrace.zPositions[std::size_t(i)];
            const float xh=hx*scale,zh=hz*scale;
            const float v0=float(sy[i])/atlasH,v1=float(sy[i]+48)/atlasH;
            std::array<InputVertex,4> q{{
                {-xh,0.f,z-zh,0.f,v1,light,1.f,1.f,1.f,1.f},
                {-xh,0.f,z+zh,0.f,v0,light,1.f,1.f,1.f,1.f},
                { xh,0.f,z+zh,1.f,v0,light,1.f,1.f,1.f,1.f},
                { xh,0.f,z-zh,1.f,v1,light,1.f,1.f,1.f,1.f}
            }};
            appendPolygon(q.data(),q.size(),itemCamera,projection,items);
        }
        copyBatch(items,menuItemVertices_,menuItemIndices_);
    }

    // 0x411EF0: LOGO (logical texture 8) and LAXY (logical texture 2).
    // The first object is X-rotated +pi/2, then radian-Y rotated by the exact
    // reveal/phase expression. Its translation is phase*(-.0058,-.0083) and
    // Z=.01+drop. The second object is X-rotated -pi/2 and has its own yaw.
    // DIRECT EXE 0x412F90: while the menu entry phase rises -0.5 -> 0,
    // camera Y is .018 - .1*phase. r166 incorrectly froze it at steady .018.
    // r194 DIRECT EXE: 0x411EF0 does not set a camera. The last 0x40C250 call
    // before it is 0x411DE1 inside 0x411D60: (0,0,0; 0,0,[0x440D1C]) where
    // 0x440D1C=round(0x2545898*entryPhase*2048) and 0x4147BC zeroes
    // 0x2545898. LOGO/LAXY are therefore projected by the identity camera.
    // r166..r193 reused the M1 item camera (.018, pitch -512), which viewed
    // the logo edge-on and produced the thin arc across the sky.
    if(mainMenuCaller){
    const CameraState titleCamera{0.f,0.f,0.f,0,0,0};
    OutputBatch logoBatch,laxyBatch;
    LegacyTransform::Matrix34 lm=LegacyTransform::identity();
    LegacyTransform::rotateXRad(lm,1.5707963267948966f);
    LegacyTransform::rotateYRad(lm,LegacyMainMenuDecorationTrace::logoYaw(ts));
    LegacyTransform::setScale(lm,LegacyMainMenuDecorationTrace::logoScale(ts));
    appendLegacyMesh(menuLogoModel_.logo,lm,
                     LegacyMainMenuDecorationTrace::logoTranslateX(ts),
                     LegacyMainMenuDecorationTrace::logoTranslateY(ts),
                     LegacyMainMenuDecorationTrace::logoTranslateZ(ts),titleCamera,projection,logoBatch,1.f,1.f,1.f,1.f,-1.f,&kMenuTitleLighting);
    if(ts.reveal!=0.f){
        LegacyTransform::Matrix34 lx=LegacyTransform::identity();
        LegacyTransform::rotateXRad(lx,-1.5707963267948966f);
        LegacyTransform::rotateYRad(lx,LegacyMainMenuDecorationTrace::laxyYaw(ts));
        // r194 DIRECT EXE 0x4120B6: the LAXY matrix scalar (+0x24) is set to
        // 0.05 before 0x401610. r166..r193 omitted it, so the AxySoft plaque
        // filled the whole screen once the title reveal completed.
        LegacyTransform::setScale(lx,0.05000000074505806f);
        appendLegacyMesh(menuLogoModel_.laxy,lx,LegacyMainMenuDecorationTrace::laxyX(),LegacyMainMenuDecorationTrace::laxyY(),LegacyMainMenuDecorationTrace::laxyZ(ts),titleCamera,projection,laxyBatch,1.f,1.f,1.f,1.f,-1.f,&kMenuTitleLighting);
    }
    copyBatch(logoBatch,menuLogoVertices_,menuLogoIndices_);copyBatch(laxyBatch,menuLaxyVertices_,menuLaxyIndices_);
    }

    // 0x412150 owns a separate decorative camera: (0,-.13,entryPhase-.3).
    // r194 DIRECT EXE 0x412222..0x41226E: 0x40C250(0,-.13,entry-.3, 0,0,[0x440D1C]).
    // [0x440D1C] is the runtime value written by 0x411D60 this frame (zero),
    // not its 1024 static initializer, and it is the sixth (roll) argument.
    // r166..r193 passed 1024 as the first angle: the basis turned 180 degrees,
    // every vertex got negative depth and the whole enemy/bonus scene, its
    // reflections, the IN2$ plaque and the selector markers were culled.
    const CameraState sceneCamera{0.f,-0.12999999523162842f,menuDecorState_.entryPhase-0.30000001192092896f,0,0,0};
    OutputBatch base,reflect; base.vertices.reserve(4096);base.indices.reserve(8192);reflect.vertices.reserve(4096);reflect.indices.reserve(8192);
    const LegacyReflection::CameraState reflectionCamera{sceneCamera.x,sceneCamera.y,sceneCamera.z,0.f,0.f};
    auto addReflection=[&](const LegacyMesh& mesh,const LegacyTransform::Matrix34& m,float x,float y,float z,float intensity){
        const auto rv=LegacyReflection::buildVertices(mesh,m,x,y,z,reflectionCamera);
        const float brightness=LegacyReflection::queuedBrightness(intensity);
        for(const auto& face:mesh.faces){if(face.index.size()!=3&&face.index.size()!=4)continue;std::array<InputVertex,4> p{};bool ok=true;
            for(std::size_t i=0;i<face.index.size();++i){const auto idx=face.index[i];if(idx>=rv.size()){ok=false;break;}const auto& v=rv[idx];p[i]={v.x,v.y,v.z,v.u,v.v,brightness,1.f,1.f,1.f,1.f};}
            if(ok)appendPolygon(p.data(),face.index.size(),sceneCamera,projection,reflect);
        }
    };
    auto matrixWave=[&](int zAngle=0,float scale=1.f){LegacyTransform::Matrix34 m=LegacyTransform::identity();if(zAngle)LegacyTransform::rotateZ(m,zAngle);LegacyTransform::rotateX(m,LegacyMainMenuDecorationTrace::legacyAngle(d.pitchWave));LegacyTransform::rotateY(m,LegacyMainMenuDecorationTrace::legacyAngle(d.yawWave));LegacyTransform::setScale(m,scale);return m;};
    const float cy=std::cos(d.yawWave),sy=std::sin(d.yawWave);

    // r177 DIRECT EXE 0x4122C1..0x4124D0. Slot0 is not a three-copy
    // shorthand: the EXE prepares it twice. Z=-0x100 produces the first two
    // base draws/reflections; Z=+0x100 repeats those two and adds the .015
    // third copy, with all three reflected. Total ownership is 5 base + 5
    // deferred reflection submissions.
    {
        auto submitPair=[&](int zAngle,bool includeThird){
            auto m=matrixWave(zAngle);
            const float xA=sy*LegacyMainMenuDecorationTrace::slot0RadiusA;
            const float zA=cy*LegacyMainMenuDecorationTrace::slot0RadiusA;
            const float xB=sy*LegacyMainMenuDecorationTrace::slot0RadiusB;
            const float zB=cy*LegacyMainMenuDecorationTrace::slot0RadiusB;
            appendLegacyMesh(menuDecorationModels_[0],m,xA,0.f,zA,sceneCamera,projection,base,1.f,1.f,1.f,1.f,-1.f,&kMenuSceneLighting);
            appendLegacyMesh(menuDecorationModels_[0],m,xB,0.f,zB,sceneCamera,projection,base,1.f,1.f,1.f,1.f,-1.f,&kMenuSceneLighting);
            addReflection(menuDecorationModels_[0],m,xA,0.f,zA,0.6f);
            addReflection(menuDecorationModels_[0],m,xB,0.f,zB,0.6f);
            if(includeThird){
                // r179 DIRECT EXE stack trace: at 0x412446/0x412454 the
                // source values resolve to the same persistent F+4/F+0 pair
                // written at 0x4122A5/0x412299: sin(yawWave), cos(yawWave).
                // The old native port incorrectly used the pitch pair here.
                const auto pC=LegacyMainMenuDecorationTrace::yawOrbit(d.yawWave,LegacyMainMenuDecorationTrace::slot0RadiusC);
                const float xC=pC.x;
                const float zC=pC.z;
                appendLegacyMesh(menuDecorationModels_[0],m,xC,0.f,zC,sceneCamera,projection,base,1.f,1.f,1.f,1.f,-1.f,&kMenuSceneLighting);
                addReflection(menuDecorationModels_[0],m,xC,0.f,zC,0.6f);
            }
        };
        submitPair(LegacyMainMenuDecorationTrace::slot0FirstZRotation,false);
        submitPair(LegacyMainMenuDecorationTrace::slot0SecondZRotation,true);
    }

    // 0x4124D5..0x41256B: slot1 has the otherwise easy-to-miss constant
    // X rotation -0x200 added to pitchWave before yawWave.
    {
        LegacyTransform::Matrix34 m=LegacyTransform::identity();
        LegacyTransform::rotateX(m,LegacyMainMenuDecorationTrace::legacyAngle(d.pitchWave)+LegacyMainMenuDecorationTrace::slot1PitchOffset);
        LegacyTransform::rotateY(m,LegacyMainMenuDecorationTrace::legacyAngle(d.yawWave));
        // r179: 0x41252E/0x412545 read F+4/F+0, i.e. sin/cos(yawWave).
        // pitchWave-512 belongs to orientation only, not the orbit position.
        const auto p1=LegacyMainMenuDecorationTrace::yawOrbit(d.yawWave,LegacyMainMenuDecorationTrace::slot1Radius);
        const float x=p1.x;
        const float z=p1.z;
        addReflection(menuDecorationModels_[1],m,x,0.f,z,0.6f);
        appendLegacyMesh(menuDecorationModels_[1],m,x,0.f,z,sceneCamera,projection,base,1.f,1.f,1.f,1.f,-1.f,&kMenuSceneLighting);
    }

    // 0x41257E..0x412676: slot2 has two base copies AND two reflection
    // requests. r176 accidentally reflected only the first copy.
    {
        auto m=matrixWave();
        const float xA=sy*LegacyMainMenuDecorationTrace::slot2RadiusA;
        const float zA=cy*LegacyMainMenuDecorationTrace::slot2RadiusA;
        // 0x412604/0x412618 also resolve to F+4/F+0 after accounting for
        // the four pushed arguments of the first base draw. Both slot2 copies
        // therefore orbit in the yaw plane; r178 incorrectly used pitch here.
        const auto pB=LegacyMainMenuDecorationTrace::yawOrbit(d.yawWave,LegacyMainMenuDecorationTrace::slot2RadiusB);
        const float xB=pB.x;
        const float zB=pB.z;
        appendLegacyMesh(menuDecorationModels_[2],m,xA,0.f,zA,sceneCamera,projection,base,1.f,1.f,1.f,1.f,-1.f,&kMenuSceneLighting);
        appendLegacyMesh(menuDecorationModels_[2],m,xB,0.f,zB,sceneCamera,projection,base,1.f,1.f,1.f,1.f,-1.f,&kMenuSceneLighting);
        addReflection(menuDecorationModels_[2],m,xA,0.f,zA,0.6f);
        addReflection(menuDecorationModels_[2],m,xB,0.f,zB,0.6f);
    }

    // 0x412676..0x412719: slot3, one base + one .6 reflection.
    {
        auto m=matrixWave();
        const float x=sy*LegacyMainMenuDecorationTrace::slot3Radius;
        const float z=cy*LegacyMainMenuDecorationTrace::slot3Radius;
        appendLegacyMesh(menuDecorationModels_[3],m,x,0.f,z,sceneCamera,projection,base,1.f,1.f,1.f,1.f,-1.f,&kMenuSceneLighting);
        addReflection(menuDecorationModels_[3],m,x,0.f,z,0.6f);
    }

    // r178 DIRECT EXE 0x41271E..0x412981: slot4 is a local orbit around
    // (0.11,0.05,0), not an origin-centered .04 circle. The local phase is
    // then transformed by the shared pitch/yaw wave. Orientation order is
    // literal: X(+0x200) -> Y(angle) -> X(pitchWave) -> Y(yawWave).
    for(int copy=0;copy<2;++copy){
        const int a=(ss.angle2048+(copy?LegacyMainMenuDecorationTrace::slot4SecondPhaseOffset:0))&0x7ff;
        auto m=LegacyTransform::identity();
        LegacyTransform::rotateX(m,LegacyMainMenuDecorationTrace::slot4BaseRotateX);
        LegacyTransform::rotateY(m,a);
        LegacyTransform::rotateX(m,LegacyMainMenuDecorationTrace::legacyAngle(d.pitchWave));
        LegacyTransform::rotateY(m,LegacyMainMenuDecorationTrace::legacyAngle(d.yawWave));
        const auto p4=LegacyMainMenuDecorationTrace::slot4Position(ss.angle2048,d.pitchWave,d.yawWave,copy!=0);
        appendLegacyMesh(menuDecorationModels_[4],m,p4.x,p4.y,p4.z,sceneCamera,projection,base,1.f,1.f,1.f,1.f,-1.f,&kMenuSceneLighting);
        addReflection(menuDecorationModels_[4],m,p4.x,p4.y,p4.z,LegacyMainMenuDecorationTrace::slot4Reflection);
    }

    // 0x4129xx: airborne subtype 2 is enlarged by 3.0 and uses the same
    // independently rotating 11-bit phase. This is one of the visible enemies.
    {
        auto m=LegacyTransform::identity();LegacyTransform::rotateX(m,(ss.angle2048*2)&0x7ff);LegacyTransform::rotateZ(m,ss.angle2048);LegacyTransform::rotateX(m,LegacyMainMenuDecorationTrace::legacyAngle(d.pitchWave));LegacyTransform::rotateY(m,LegacyMainMenuDecorationTrace::legacyAngle(d.yawWave));LegacyTransform::setScale(m,3.f);
        const auto pAir=LegacyMainMenuDecorationTrace::yawOrbit(d.yawWave,LegacyMainMenuDecorationTrace::airborneSubtype2Radius);
        appendLegacyMesh(airEnemyModels_[2],m,pAir.x,0.f,pAir.z,sceneCamera,projection,base,1.f,1.f,1.f,1.f,-1.f,&kMenuSceneLighting);
    }

    // 0x412A2F..0x412BE1: crawler pair. Master 0x02585A5C is prepared with
    // scale 3.5, then the normal prepared crawler 0x0257F5B8 is submitted at
    // the exact derived translation; the reflection request is .5.
    {
        auto m=LegacyTransform::identity();LegacyTransform::rotateY(m,-ss.angle2048);LegacyTransform::rotateX(m,LegacyMainMenuDecorationTrace::legacyAngle(d.pitchWave));LegacyTransform::rotateY(m,LegacyMainMenuDecorationTrace::legacyAngle(d.yawWave));LegacyTransform::setScale(m,3.5f);
        const auto crawlerPrimary=LegacyMainMenuDecorationTrace::crawlerPrimaryPosition(d.pitchWave,d.yawWave);
        appendLegacyMesh(groundEnemyModel_,m,crawlerPrimary.x,crawlerPrimary.y,crawlerPrimary.z,sceneCamera,projection,base,1.f,1.f,1.f,1.f,-1.f,&kMenuSceneLighting);
        addReflection(groundEnemyModel_,m,crawlerPrimary.x,crawlerPrimary.y,crawlerPrimary.z,LegacyMainMenuDecorationTrace::crawlerPrimaryReflection);
        LegacyTransform::Matrix34 m2=LegacyTransform::identity();LegacyTransform::rotateZ(m2,LegacyMainMenuDecorationTrace::legacyAngle(ss.c*3183.098876953125f/325.949310302734375f));LegacyTransform::rotateX(m2,-500);
        appendLegacyMesh(groundEnemyModel_,m2,std::cos(2.f*ss.c)*.03f,-0.15f,std::sin(2.f*ss.c)*.03f-0.25f,sceneCamera,projection,base,1.f,1.f,1.f,1.f,-1.f,&kMenuSceneLighting);
    }

    // 0x412C04..0x412EDE: four gameplay pickups orbit the menu scene. These
    // are exact phase offsets and world-position equations from the EXE.
    const std::array<int,4> pickupType{{5,1,3,2}};
    const std::array<float,4> phaseOffset{{0.f,1.5707963267948966f,4.71238898038469f,3.141592653589793f}};
    const std::array<float,4> reflIntensity{{0.95f,0.99f,0.35f,0.15f}};
    for(std::size_t i=0;i<4;++i){
        const float theta=phaseOffset[i]-3.f*ss.c;
        const int ownSpin=LegacyMainMenuDecorationTrace::legacyAngle(ss.c*9549.2958984375f/325.949310302734375f);
        const LegacyTransform::Matrix34 m=LegacyMainMenuDecorationTrace::menuPickupOrientation(i,ownSpin);
        const float x=LegacyMainMenuDecorationTrace::orbitX(theta),y=LegacyMainMenuDecorationTrace::orbitY(),z=LegacyMainMenuDecorationTrace::orbitZ(theta);
        const auto& mesh=pickupModels_[std::size_t(pickupType[i])];
        appendLegacyMesh(mesh,m,x,y,z,sceneCamera,projection,base,1.f,1.f,1.f,1.f,-1.f,&kMenuPickupLighting);
        addReflection(mesh,m,x,y,z,reflIntensity[i]);
    }

    // 0x412F0F..0x412F78: slot6 is NOT part of the texture-3 batch. The EXE
    // switches to logical texture 7 first and submits it only while 0x025459B0
    // is zero (the normal main-menu state). Slot5 is built by 0x4225A0 but has
    // no draw reference in 0x412150. The literal slot6 transform is X(-0x200),
    // x=yawWave*.003, y=-.123, z=-.29.
    // r194 DIRECT EXE 0x423C30..0x423E06: the M1 atlas constructor loads
    // only FourCC 'nreg' into logical texture 7 (x=0,y=0), never IN2$/IN2T.
    // 'nreg' is absent from BMPPACK, and the 0x025459B0 guard belongs to the
    // registration path (r183). The object is therefore the unregistered-
    // version banner, not a "$1000 / TIME" plaque; the native atlas7 used here
    // is the gameplay HUD atlas and showed IN2$+IN2T. The autonomous port has
    // no registration state and runs the full selector, i.e. the registered
    // (0x025459B0 != 0) branch, which skips this submission entirely.
    constexpr bool kLegacyUnregisteredBanner=false;
    OutputBatch texture7;
    if(kLegacyUnregisteredBanner){
        LegacyTransform::Matrix34 m=LegacyTransform::identity();
        LegacyTransform::rotateX(m,-0x200);
        const float x=d.yawWave*0.003000000026077032f;
        appendLegacyMesh(menuDecorationModels_[6],m,x,-0.12300000339746475f,-0.28999999165534973f,sceneCamera,projection,texture7,1.f,1.f,1.f,1.f,-1.f,&kMenuPostPickupLighting);
    }

    // r183 direct caller tail 0x4132B7..0x413444. 0x412150 returns with
    // logical texture 3 restored; the caller then prepares airborne subtype 0
    // at scale .2 and submits mirrored selector markers at x=+/-0.009 around
    // the currently selected M1 row. Rotation is X by a local phase += 2*dt.
    OutputBatch selector;
    if(mainMenuCaller){
    menuDecorState_.selectorAngle=(menuDecorState_.selectorAngle +
        std::max(0,menuFrameDt_)*LegacyMainMenuDecorationTrace::selectorLegacyUnitsPerMs)&0x7ff;
    {
        LegacyTransform::Matrix34 m=LegacyTransform::identity();
        LegacyTransform::rotateX(m,menuDecorState_.selectorAngle);
        LegacyTransform::setScale(m,LegacyMainMenuDecorationTrace::selectorScale);
        const float z=game.mainMenu().selectorOffset+kLegacyMainMenuSelectorSlideTrace.markerZBias;
        // r194 DIRECT EXE: the markers are submitted at 0x41342E/0x413444,
        // after 0x4133E9 installed the M1 item camera, not the 0x412150 camera.
        const CameraState markerCamera{kLegacyMainMenuCameraTrace.steadyCameraX,
            kLegacyMainMenuCameraTrace.steadyCameraY-kLegacyMainMenuCameraTrace.cameraYPhaseScale*menuDecorState_.entryPhase,
            kLegacyMainMenuCameraTrace.steadyCameraZ,
            kLegacyMainMenuCameraTrace.steadyAngle1,kLegacyMainMenuCameraTrace.steadyAngle2,kLegacyMainMenuCameraTrace.steadyAngle3};
        appendLegacyMesh(airEnemyModels_[0],m,-LegacyMainMenuDecorationTrace::selectorX,0.f,z,markerCamera,projection,selector,1.f,1.f,1.f,1.f,-1.f,&kMenuSelectorLighting);
        appendLegacyMesh(airEnemyModels_[0],m,+LegacyMainMenuDecorationTrace::selectorX,0.f,z,markerCamera,projection,selector,1.f,1.f,1.f,1.f,-1.f,&kMenuSelectorLighting);
    }
    }

    copyBatch(base,menuDecorationVertices_,menuDecorationIndices_);
    copyBatch(texture7,menuDecorationTexture7Vertices_,menuDecorationTexture7Indices_);
    copyBatch(reflect,menuDecorationReflectionVertices_,menuDecorationReflectionIndices_);
    copyBatch(selector,menuSelectorVertices_,menuSelectorIndices_);
    if(std::getenv("AIRXONIX_DEBUG_MENU")){static int n=0;if((n++%60)==0){auto rep=[&](const char* nm,const std::vector<ScreenVertex>& v,const std::vector<std::uint16_t>& ix){float x0=1e9,x1=-1e9,y0=1e9,y1=-1e9,z0=1e9,z1=-1e9;for(auto&q:v){float w=q.w?q.w:1;x0=std::min(x0,q.x/w);x1=std::max(x1,q.x/w);y0=std::min(y0,q.y/w);y1=std::max(y1,q.y/w);z0=std::min(z0,q.z/w);z1=std::max(z1,q.z/w);}std::fprintf(stderr,"AXDBG %s v=%zu i=%zu x[%.3f,%.3f] y[%.3f,%.3f] z[%.3f,%.3f]\n",nm,v.size(),ix.size(),x0,x1,y0,y1,z0,z1);};
        rep("logo",menuLogoVertices_,menuLogoIndices_);rep("laxy",menuLaxyVertices_,menuLaxyIndices_);rep("decor",menuDecorationVertices_,menuDecorationIndices_);rep("refl",menuDecorationReflectionVertices_,menuDecorationReflectionIndices_);rep("tex7",menuDecorationTexture7Vertices_,menuDecorationTexture7Indices_);rep("sel",menuSelectorVertices_,menuSelectorIndices_);rep("items",menuItemVertices_,menuItemIndices_);rep("bg0",menuBg0Vertices_,menuBg0Indices_);}}
}


void Renderer::rebuildModeSelectDecorationBatch(const Game& game){
    using namespace LegacyScreenPipeline;
    modeSelectSlot0Vertices_.clear();modeSelectSlot0Indices_.clear();
    modeSelectDecorationVertices_.clear();modeSelectDecorationIndices_.clear();
    modeSelectReflectionVertices_.clear();modeSelectReflectionIndices_.clear();
    const auto& t=kLegacyModeSelectPresentationTrace;
    const auto& ms=game.modeSelect();
    const CameraState camera{t.cameraX,t.cameraY,t.cameraZ,t.cameraAngle1,t.cameraAngle2,t.cameraAngle3};
    const auto projection=LegacyCamera::projectionState(640,480);
    const LegacyReflection::CameraState rc{camera.x,camera.y,camera.z,0.f,0.f};
    OutputBatch slot0,base,reflect;
    const float light=float(LegacyModeSelectPresentationTrace::lightByte(ms.fadeCounter))/255.f;
    auto appendReflect=[&](const LegacyMesh& mesh,const LegacyTransform::Matrix34& m,float x,float y,float z,float intensity){
        const auto rv=LegacyReflection::buildVertices(mesh,m,x,y,z,rc);
        const float br=LegacyReflection::queuedBrightness(intensity)*light;
        for(const auto& face:mesh.faces){
            if(face.index.size()!=3&&face.index.size()!=4)continue;
            std::array<InputVertex,4> q{};bool ok=true;
            for(std::size_t i=0;i<face.index.size();++i){
                const auto idx=face.index[i]; if(idx>=rv.size()){ok=false;break;}
                const auto& v=rv[idx]; q[i]={v.x,v.y,v.z,v.u,v.v,br,1.f,1.f,1.f,1.f};
            }
            if(ok)appendPolygon(q.data(),face.index.size(),camera,projection,reflect);
        }
    };
    // r320 DIRECT EXE + r61: slot0 is M101 from the common 0x4229B0 menu
    // model array. 0x403860 gives local bounds x=+/-0.006, z=+/-0.001 at
    // y=0; 0x411BB1 submits it at z=0.005 under logical texture 4. Native
    // packs M101 at v=0..48 of the 256-high M1 atlas.
    {
        constexpr float hx=0.006000000052154064f,hz=0.0010000000474974513f;
        constexpr float v0=0.f,v1=48.f/256.f;
        std::array<InputVertex,4> q{{
            {-hx,0.f,t.slot0Z-hz,0.f,v1,light,1.f,1.f,1.f,1.f},
            {-hx,0.f,t.slot0Z+hz,0.f,v0,light,1.f,1.f,1.f,1.f},
            { hx,0.f,t.slot0Z+hz,1.f,v0,light,1.f,1.f,1.f,1.f},
            { hx,0.f,t.slot0Z-hz,1.f,v1,light,1.f,1.f,1.f,1.f}
        }};
        appendPolygon(q.data(),q.size(),camera,projection,slot0);
    }

    // 0x411BBD..0x411C21: subtype-0 airborne, X rotation = anglePhase,
    // uniform scale .13 and row-selected Z.
    LegacyTransform::Matrix34 air=LegacyTransform::identity();
    LegacyTransform::rotateX(air,t.airborneAngle(ms.anglePhase));
    LegacyTransform::setScale(air,t.airborneScale);
    appendLegacyMesh(airEnemyModels_[0],air,t.airborneX,0.f,t.airborneZ(ms.selected),camera,projection,base,light,1.f,1.f,1.f);

    // 0x411C2D..0x411CFE: crawler pair, shared matrix and .75 deferred
    // environment/reflection requests.
    LegacyTransform::Matrix34 crawler=LegacyTransform::identity();
    LegacyTransform::rotateX(crawler,t.crawlerXAngle());
    LegacyTransform::rotateZ(crawler,t.crawlerZAngle(ms.anglePhase));
    LegacyTransform::setScale(crawler,t.crawlerScale(ms.anglePhase));
    appendLegacyMesh(groundEnemyModel_,crawler,t.crawlerXA,0.f,t.crawlerZ,camera,projection,base,light,1.f,1.f,1.f);
    appendLegacyMesh(groundEnemyModel_,crawler,t.crawlerXB,0.f,t.crawlerZ,camera,projection,base,light,1.f,1.f,1.f);
    appendReflect(groundEnemyModel_,crawler,t.crawlerXA,0.f,t.crawlerZ,t.crawlerReflectionIntensity());
    appendReflect(groundEnemyModel_,crawler,t.crawlerXB,0.f,t.crawlerZ,t.crawlerReflectionIntensity());

    auto copy=[](const OutputBatch& in,std::vector<ScreenVertex>& vo,std::vector<std::uint16_t>& io){
        vo.reserve(in.vertices.size());
        for(const auto& q:in.vertices)vo.push_back({q.clipX,q.clipY,q.clipZ,q.clipW,q.u,q.v,q.light,q.r,q.g,q.b,q.a});
        io=in.indices;
    };
    copy(slot0,modeSelectSlot0Vertices_,modeSelectSlot0Indices_);
    copy(base,modeSelectDecorationVertices_,modeSelectDecorationIndices_);
    copy(reflect,modeSelectReflectionVertices_,modeSelectReflectionIndices_);
}

void Renderer::rebuildHudBatch(const Game& game){
    hud3Vertices_.clear();hud3Indices_.clear();hud4Vertices_.clear();hud4Indices_.clear();hud7Vertices_.clear();hud7Indices_.clear();hudM1Vertices_.clear();hudM1Indices_.clear();hudM2Vertices_.clear();hudM2Indices_.clear();hudFont5Vertices_.clear();hudFont5Indices_.clear();
    LegacyHudState state;
    state.lives=game.lives();state.timeRemaining=game.hudDisplayTimer();state.levelNumber=static_cast<int>(game.levelIndex())+1;
    state.score=game.hudDisplayScore();state.capturePercent=game.capturePercent();state.pulseCounter=game.hudPulseCounter();state.paused=game.paused();state.menuSelected=game.mainMenu().selected;state.menuBrightness=game.mainMenu().brightness;state.menuScale=game.mainMenu().scale;
    state.modeSelected=game.modeSelect().selected;
    state.modeAnglePhase=game.modeSelect().anglePhase;
    state.modeFadeCounter=game.modeSelect().fadeCounter;
    for(std::size_t i=0;i<state.modeLevelCounts.size() && i<game.database().modes().size();++i){
        state.modeLevelCounts[i]=static_cast<int>(game.database().modes()[i].levels.size());
        state.modeNames[i]=game.database().modes()[i].name;
    }
    state.settingsSelected=game.settings().selected;state.settingsBrightness=game.settings().brightness;state.settingsScale=game.settings().scale;
    state.settingsSpeed=game.settings().speed;state.settingsSfx=game.settings().sfx;state.settingsMusic=game.settings().music;state.settingsSpeech=game.settings().speech;
    state.settingsSelectorOffset=game.settings().selectorOffset;state.settingsFadeScale=LegacySettingsTrace::modelLightScale(game.settings().fadeCounter);
    state.testInitialLives=game.settings().testInitialLives;state.testInitialTimeSeconds=game.settings().testInitialTimeSeconds;
    state.recordsMode=static_cast<int>(game.records().mode);
    if(game.records().mode<game.database().modes().size())
        state.recordsModeName=game.database().modes()[game.records().mode].name;
    state.recordsFadeCounter=game.records().fadeCounter;
    state.recordsNameEntry=game.records().nameEntry;
    state.recordsCandidateRow=game.records().candidateRow;
    state.recordsTypedNameLength=game.records().typedNameLength;
    state.recordsNameCursor=game.records().nameCursor;
    state.recordsDpadNameEditing=game.records().dpadNameEditing;
    state.recordsNamePulseByte=LegacyRecordsTransition::namePulseByte(game.records().namePulsePhase);
    state.recordsHeadingPhase=game.records().headingPhase;
    {
        // r242: 0x40F0A0 renders the temporary 200-byte block. During
        // post-game name entry this intentionally differs from persistent data.
        const auto& high=game.recordsDisplayHighScores();
        state.recordsNames=high.names;
        state.recordsValues=high.values;
    }
    state.informationPage=game.information().page;
    state.informationFadeCounter=game.information().fadeCounter;
    state.informationPromptPhase=game.information().promptPhase;
    state.controlsBindings=game.controlsRemap().temporary;state.controlsAssigned=game.controlsRemap().assigned;state.controlsAwaitingConfirm=game.controlsRemap().awaitingConfirm;
    state.controlsCurrentRowColor=kLegacyControlsTrace.grayRgb(kLegacyControlsTrace.currentRowPulse(game.controlsRemap().pulsePhase));
    state.controlsPromptColor=kLegacyControlsTrace.promptRgb(kLegacyControlsTrace.promptPulse(game.controlsRemap().pulsePhase));
    state.controlsFadeScale=kLegacyControlsTrace.modelLightScale(game.controlsRemap().fadeCounter);
    state.controlsFadeCounter=game.controlsRemap().fadeCounter;
    switch(game.phase()){
        case GamePhase::Gameplay: state.screen=LegacyHudScreen::Gameplay;break;
        case GamePhase::Dying: state.screen=LegacyHudScreen::Gameplay;break;
        case GamePhase::InterLevel: state.screen=LegacyHudScreen::InterLevel; /* 0x41A980 -> 0x4201E0 -> 0x4247E0 keeps the completed-level HUD alive */ break;
        case GamePhase::FinalSequence: state.screen=LegacyHudScreen::FinalSequence;break;
        case GamePhase::MainMenu: state.screen=LegacyHudScreen::MainMenu;break;
        case GamePhase::ModeSelect: state.screen=LegacyHudScreen::ModeSelect;break;
        case GamePhase::Records: state.screen=LegacyHudScreen::Records;break;
        case GamePhase::Information: state.screen=LegacyHudScreen::Information;break;
        case GamePhase::Settings: state.screen=LegacyHudScreen::Settings;break;
        case GamePhase::Controls: state.screen=LegacyHudScreen::Controls;break;
        case GamePhase::Complete: state.screen=LegacyHudScreen::Complete;break;
        case GamePhase::GameOver: state.screen=LegacyHudScreen::GameOver;break;
        case GamePhase::Abort: state.screen=LegacyHudScreen::Abort;break;
    }
    const auto commands=LegacyHud::compose(state);
    auto append=[&](const LegacyHudSprite& q,std::vector<ScreenVertex>& vertices,std::vector<std::uint16_t>& indices){
        if(vertices.size()>std::numeric_limits<std::uint16_t>::max()-4u)return;
        const auto base=static_cast<std::uint16_t>(vertices.size());
        const float x0=q.x/320.f-1.f,x1=(q.x+q.w)/320.f-1.f;
        const float y0=1.f-q.y/240.f,y1=1.f-(q.y+q.h)/240.f;
        const float u0=q.u0>=0.f?q.u0:float(q.sx)/256.f,v0=q.v0>=0.f?q.v0:float(q.sy)/256.f;
        const float u1=q.u1>=0.f?q.u1:float(q.sx+q.sw)/256.f,v1=q.v1>=0.f?q.v1:float(q.sy+q.sh)/256.f;
        vertices.push_back({x0,y0,0.f,1.f,u0,v0,q.brightness,q.r,q.g,q.b,1.f});
        vertices.push_back({x1,y0,0.f,1.f,u1,v0,q.brightness,q.r,q.g,q.b,1.f});
        vertices.push_back({x1,y1,0.f,1.f,u1,v1,q.brightness,q.r,q.g,q.b,1.f});
        vertices.push_back({x0,y1,0.f,1.f,u0,v1,q.brightness,q.r,q.g,q.b,1.f});
        indices.insert(indices.end(),{base,static_cast<std::uint16_t>(base+1),static_cast<std::uint16_t>(base+2),base,static_cast<std::uint16_t>(base+2),static_cast<std::uint16_t>(base+3)});
    };
    for(const auto& q:commands){
        if(q.atlas==LegacyHudAtlas::Atlas3)append(q,hud3Vertices_,hud3Indices_);
        else if(q.atlas==LegacyHudAtlas::Atlas4)append(q,hud4Vertices_,hud4Indices_);
        else if(q.atlas==LegacyHudAtlas::Atlas7)append(q,hud7Vertices_,hud7Indices_);
        else if(q.atlas==LegacyHudAtlas::MenuM1)append(q,hudM1Vertices_,hudM1Indices_);
        else if(q.atlas==LegacyHudAtlas::MenuM2)append(q,hudM2Vertices_,hudM2Indices_);
        else append(q,hudFont5Vertices_,hudFont5Indices_);
    }
}


void Renderer::rebuildSettingsWidgetBatch(const Game& game){
    settingsTrackVertices_.clear();settingsTrackIndices_.clear();
    settingsKnobVertices_.clear();settingsKnobIndices_.clear();
    using namespace LegacyScreenPipeline;
    // r350 DIRECT EXE 0x413CB0..0x413CD4: Settings has its own camera.
    // Z follows the smoothed selected-row offset rather than staying at -0.003.
    const CameraState camera{0.f,LegacySettingsTrace::cameraY,
                             LegacySettingsTrace::cameraZ(game.settings().selectorOffset),
                             LegacySettingsTrace::cameraAngle1,
                             LegacySettingsTrace::cameraAngle2,
                             LegacySettingsTrace::cameraAngle3};
    const auto projection=LegacyCamera::projectionState(640,480);
    OutputBatch trackBatch,knobBatch;
    trackBatch.vertices.reserve(settingsTrackModel_.vertices.size()*3u);
    knobBatch.vertices.reserve(settingsKnobModel_.vertices.size()*3u);

    LegacyTransform::Matrix34 track=LegacyTransform::identity();
    LegacyTransform::rotateZ(track,0x200); // 0x413EFF..0x413F05
    LegacyTransform::setScale(track,LegacySettingsWidgets::kTrace.trackScale);

    LegacyTransform::Matrix34 knob=LegacyTransform::identity();
    // r350 DIRECT EXE 0x413FF3..0x41400D: every knob gets a fixed Z=0x200
    // quarter-turn first, then the shared 2*dt X spin. Missing the Z turn made
    // all three handles visibly mis-oriented.
    LegacyTransform::rotateZ(knob,0x200);
    settingsWidgetAngle_+=float(menuFrameDt_)*2.f;
    const int knobAngle=static_cast<int>(settingsWidgetAngle_);
    LegacyTransform::rotateX(knob,knobAngle);
    LegacyTransform::setScale(knob,LegacySettingsWidgets::kTrace.knobScale);

    const float values[3]={game.settings().speed,game.settings().sfx,game.settings().music};
    const float screenFade=LegacySettingsTrace::modelLightScale(game.settings().fadeCounter);
    for(int i=0;i<3;++i){
        // 0x413F5A multiplies the row brightness by the screen light
        // established from fadeCounter>>3 at 0x413C55.
        const float bright=game.settings().brightness[std::size_t(i)]*screenFade;
        appendLegacyMesh(settingsTrackModel_,track,
                         kLegacySettingsVisualTrace.trackX,0.f,kLegacySettingsVisualTrace.rowZ(i),
                         camera,projection,trackBatch,bright,bright,bright,1.f);
        appendLegacyMesh(settingsKnobModel_,knob,
                         kLegacySettingsVisualTrace.knobX(values[i]),0.f,kLegacySettingsVisualTrace.rowZ(i),
                         camera,projection,knobBatch,bright,bright,bright,1.f);
    }
    auto convert=[](const OutputBatch& in,std::vector<ScreenVertex>& v,std::vector<std::uint16_t>& i){
        v.reserve(in.vertices.size());
        for(const auto& q:in.vertices)v.push_back({q.clipX,q.clipY,q.clipZ,q.clipW,q.u,q.v,q.light,q.r,q.g,q.b,q.a});
        i=in.indices;
    };
    convert(trackBatch,settingsTrackVertices_,settingsTrackIndices_);
    convert(knobBatch,settingsKnobVertices_,settingsKnobIndices_);
}


void Renderer::rebuildRecordsDecorationBatch(const Game& game){
    (void)game;
    recordsAlwaysVertices_.clear();recordsAlwaysIndices_.clear();
    recordsDecorationVertices_.clear();recordsDecorationIndices_.clear();
    recordsDecorationReflectionVertices_.clear();recordsDecorationReflectionIndices_.clear();
    using namespace LegacyScreenPipeline;
    const int dt=std::clamp(menuFrameDt_,0,100);
    if(!recordsSceneActive_){
        recordsSceneActive_=true;
        recordsDecorPhase_=0.f;
        recordsCrawlerLane_={{0.08f,0.026666667f,-0.026666667f,0.053333335f,0.f,-0.053333335f}};
    }
    // DIRECT EXE 0x40FB17..0x40FC67: common presentation angle advances by
    // floor(3*dt/2).  Pickup0 uses +angle and 0xC8-angle, then every crawler
    // uses the same Z angle plus X=angle/2.
    recordsDecorPhase_+=float((dt*3)/2);
    while(recordsDecorPhase_>=2048.f)recordsDecorPhase_-=2048.f;
    const int angle=int(recordsDecorPhase_)&0x7ff;

    // DIRECT EXE 0x40FC91..0x40FD08: six lane scalars form two groups of
    // three.  First group moves +0.00004*dt and wraps +.08 -> -.08; second
    // moves the opposite direction and wraps -.08 -> +.08.
    const float laneStep=float(dt)*0.00003999999910593033f;
    for(int i=0;i<3;++i){recordsCrawlerLane_[std::size_t(i)]+=laneStep;if(recordsCrawlerLane_[std::size_t(i)]>=0.08f)recordsCrawlerLane_[std::size_t(i)]-=0.16f;}
    for(int i=3;i<6;++i){recordsCrawlerLane_[std::size_t(i)]-=laneStep;if(recordsCrawlerLane_[std::size_t(i)]< -0.08f)recordsCrawlerLane_[std::size_t(i)]+=0.16f;}

    const CameraState camera{0.f,0.f,0.f,0,0,0};
    const auto projection=LegacyCamera::projectionState(640,480);
    OutputBatch alwaysBase,base,reflect;
    const LegacyReflection::CameraState rc{camera.x,camera.y,camera.z,0.f,0.f};
    auto addTo=[&](OutputBatch& target,const LegacyMesh& mesh,const LegacyTransform::Matrix34& m,float x,float y,float z,float refl){
        appendLegacyMesh(mesh,m,x,y,z,camera,projection,target);
        if(refl<=0.f)return;
        const auto rv=LegacyReflection::buildVertices(mesh,m,x,y,z,rc);
        const float br=LegacyReflection::queuedBrightness(refl);
        for(const auto& face:mesh.faces){
            if(face.index.size()!=3&&face.index.size()!=4)continue;
            std::array<InputVertex,4> q{};bool ok=true;
            for(std::size_t i=0;i<face.index.size();++i){const auto idx=face.index[i];if(idx>=rv.size()){ok=false;break;}const auto& v=rv[idx];q[i]={v.x,v.y,v.z,v.u,v.v,br,1.f,1.f,1.f,1.f};}
            if(ok)appendPolygon(q.data(),face.index.size(),camera,projection,reflect);
        }
    };

    auto add=[&](const LegacyMesh& mesh,const LegacyTransform::Matrix34& m,float x,float y,float z,float refl){
        addTo(base,mesh,m,x,y,z,refl);
    };
    auto addAlways=[&](const LegacyMesh& mesh,const LegacyTransform::Matrix34& m,float x,float y,float z,float refl){
        addTo(alwaysBase,mesh,m,x,y,z,refl);
    };

    // r235 DIRECT EXE 0x40FB17..0x40FC33: two mirrored TIME/pickup0 base
    // models are submitted while ZFUNC is still ALWAYS. Their 0x40E710
    // reflection entries are only queued here and flush later under LEQUAL.
    LegacyTransform::Matrix34 p0=LegacyTransform::identity();
    LegacyTransform::rotateZ(p0,angle);LegacyTransform::rotateX(p0,0x190);
    addAlways(pickupModels_[0],p0,+0.035f,0.048f,0.08f,0.400000006f);
    LegacyTransform::Matrix34 p1=LegacyTransform::identity();
    LegacyTransform::rotateZ(p1,(0xC8-angle)&0x7ff);LegacyTransform::rotateX(p1,0x190);
    addAlways(pickupModels_[0],p1,-0.035f,0.048f,0.08f,0.400000006f);

    // 0x40FC33..0x40FE9F: six crawler base passes followed by six matching
    // environment/reflection passes.  They stream vertically on x=+/-0.08.
    LegacyTransform::Matrix34 crawler=LegacyTransform::identity();
    LegacyTransform::rotateZ(crawler,angle);LegacyTransform::rotateX(crawler,angle/2);
    for(int i=0;i<6;++i){
        const float x=i<3?+0.08f:-0.08f;
        add(groundEnemyModel_,crawler,x,recordsCrawlerLane_[std::size_t(i)],0.1f,0.5f);
    }

    // 0x40FEA4..0x40FF24: final symmetric airborne pair, subtype 0 / 2.
    LegacyTransform::Matrix34 air=LegacyTransform::identity();
    LegacyTransform::rotateZ(air,angle);LegacyTransform::rotateX(air,angle/2);
    add(airEnemyModels_[0],air,+0.08f,-0.095f,0.14f,0.f);
    add(airEnemyModels_[2],air,-0.08f,-0.095f,0.14f,0.f);

    auto copy=[](const OutputBatch& in,std::vector<ScreenVertex>& vo,std::vector<std::uint16_t>& io){vo.reserve(in.vertices.size());for(const auto& q:in.vertices)vo.push_back({q.clipX,q.clipY,q.clipZ,q.clipW,q.u,q.v,q.light,q.r,q.g,q.b,q.a});io=in.indices;};
    copy(alwaysBase,recordsAlwaysVertices_,recordsAlwaysIndices_);
    copy(base,recordsDecorationVertices_,recordsDecorationIndices_);
    copy(reflect,recordsDecorationReflectionVertices_,recordsDecorationReflectionIndices_);
}

void Renderer::rebuildRecordsBackdropBatch(const Game& game){
    recordsBackdropVertices_.clear();
    recordsBackdropIndices_.clear();

    // r234 DIRECT EXE 0x40F3E1 + 0x40FA79..0x40FAE7: this phase is local to
    // the Records routine, starts at zero on entry and advances by dt*0.0003.
    const std::uint32_t now=SDL_GetTicks();
    int dt=0;
    if(menuLastTicks_!=0u)dt=static_cast<int>(std::min<std::uint32_t>(now-menuLastTicks_,100u));
    menuFrameDt_=dt;
    menuLastTicks_=now;
    if(!recordsBackdropActive_){recordsBackdropPhase_=0.f;recordsBackdropActive_=true;}
    recordsBackdropPhase_=LegacyRecordsBackgroundTrace::advancePhase(recordsBackdropPhase_,dt);

    const float u0=-recordsBackdropPhase_;
    const float v0=-recordsBackdropPhase_;
    const float u1=u0+kLegacyRecordsBackgroundTrace.extentX;
    const float v1=v0+kLegacyRecordsBackgroundTrace.extentY;
    const float z=2.f*kLegacyRecordsBackgroundTrace.uvInset0-1.f; // 0x40E3D0 sz=.99
    const float c=LegacyRecordsTransition::backdropGray(game.records().fadeCounter);
    recordsBackdropVertices_={
        {-1.f, 1.f,z,1.f,u0,v0,1.f,c,c,c,1.f},
        { 1.f, 1.f,z,1.f,u1,v0,1.f,c,c,c,1.f},
        { 1.f,-1.f,z,1.f,u1,v1,1.f,c,c,c,1.f},
        {-1.f,-1.f,z,1.f,u0,v1,1.f,c,c,c,1.f}
    };
    recordsBackdropIndices_={0,1,2,0,2,3};
}

void Renderer::rebuildInformationBackdropBatch(const Game& game){
    informationBackdropVertices_.clear();
    informationBackdropIndices_.clear();

    const std::uint32_t now=SDL_GetTicks();
    int dt=0;
    if(menuLastTicks_!=0u)dt=static_cast<int>(std::min<std::uint32_t>(now-menuLastTicks_,100u));
    menuFrameDt_=dt;
    menuLastTicks_=now;

    const int page=game.information().page;
    if(page!=informationBackdropLastPage_){
        informationBackdropPhase_=0.f;
        informationBackdropLastPage_=page;
    }
    if(page<0 || page>1)return; // page 2 has no 0x40E3D0 backdrop submit

    informationBackdropPhase_=LegacyInformationBackdrop::advancePhase(page,informationBackdropPhase_,dt);
    const float u0=informationBackdropPhase_;
    const float v0=-informationBackdropPhase_;
    const float u1=u0+LegacyInformationBackdrop::UvExtent;
    const float v1=v0+LegacyInformationBackdrop::UvExtent;
    const float z=2.f*LegacyInformationBackdrop::TlDepth-1.f;
    const float c=LegacyInformationBackdrop::grayscale01(game.information().fadeCounter);
    informationBackdropVertices_={
        {-1.f, 1.f,z,1.f,u0,v0,1.f,c,c,c,1.f},
        { 1.f, 1.f,z,1.f,u1,v0,1.f,c,c,c,1.f},
        { 1.f,-1.f,z,1.f,u1,v1,1.f,c,c,c,1.f},
        {-1.f,-1.f,z,1.f,u0,v1,1.f,c,c,c,1.f}
    };
    informationBackdropIndices_={0,1,2,0,2,3};
}

void Renderer::rebuildInformationDecorationBatch(Game& game){
    informationVertices_.clear();informationIndices_.clear();
    informationAlwaysVertices_.clear();informationAlwaysIndices_.clear();
    informationFlyingMineVertices_.clear();informationFlyingMineIndices_.clear();
    informationReflectionVertices_.clear();informationReflectionIndices_.clear();
    informationXonixVertices_.clear();informationXonixIndices_.clear();
    const int page=game.information().page;
    if(page<=0){informationLastPage_=0;return;}
    if(page!=informationLastPage_){
        // 0x410CF0 enters three distinct page routines, so their local phase
        // variables restart at each page boundary.  Page 3 (0x414170) also
        // initializes its local fountain head/timer to zero at 0x41423D/240.
        informationSpin2048_=0; informationLeadPhase_=0; informationSwayPhase_=0.f; informationSpecialPhase_=0.f; informationXonixPhase_=0.f;
        informationFountainHead_=0; informationFountainSpawnMs_=0;
        // The particle records themselves are NOT local: 0x414170 updates the
        // resident global pool at 0x0257E8D8 and never clears it on entry.
        // Preserve it across page changes and subsequent Information visits.
        informationLastPage_=page;
    }
    using namespace LegacyScreenPipeline;
    const int dt=std::clamp(menuFrameDt_,0,100);
    // 0x4106E3 and 0x4143E1: both Information object loops advance their
    // integer presentation angle by floor(3*dt/2).
    informationSpin2048_=(informationSpin2048_+(dt*3)/2)&0x7ff;
    informationLeadPhase_=(informationLeadPhase_+dt)&0x7fff;
    informationSwayPhase_=LegacyInformationTransition::advanceSwayPhase(informationSwayPhase_,dt);
    informationSpecialPhase_+=float(dt)*0.008999999612569809f;
    constexpr float kTwoPi=6.28318530717958647692f;
    while(informationSpecialPhase_>=kTwoPi)informationSpecialPhase_-=kTwoPi;

    const CameraState camera{0.f,0.f,0.f,0,0,0};
    const auto projection=LegacyCamera::projectionState(640,480);
    OutputBatch base,alwaysBase,flyingMine,reflect,xonixLate;
    auto addM=[&](const LegacyMesh& mesh,const LegacyTransform::Matrix34& m,float x,float y,float z,const CameraState& cam){
        appendLegacyMesh(mesh,m,x,y,z,cam,projection,base);
    };
    auto addAlwaysM=[&](const LegacyMesh& mesh,const LegacyTransform::Matrix34& m,float x,float y,float z,const CameraState& cam){
        appendLegacyMesh(mesh,m,x,y,z,cam,projection,alwaysBase);
    };
    auto addReflectionM=[&](const LegacyMesh& mesh,const LegacyTransform::Matrix34& m,float x,float y,float z,float intensity,const CameraState& cam){
        const LegacyReflection::CameraState rc{cam.x,cam.y,cam.z,0.f,0.f};
        const auto rv=LegacyReflection::buildVertices(mesh,m,x,y,z,rc);
        const float brightness=LegacyReflection::queuedBrightness(intensity);
        for(const auto& face:mesh.faces){
            if(face.index.size()!=3 && face.index.size()!=4)continue;
            std::array<InputVertex,4> v{};bool valid=true;
            for(std::size_t i=0;i<face.index.size();++i){
                const auto idx=face.index[i];if(idx>=rv.size()){valid=false;break;}
                const auto& q=rv[idx];v[i]={q.x,q.y,q.z,q.u,q.v,brightness,1.f,1.f,1.f,1.f};
            }
            if(valid)appendPolygon(v.data(),face.index.size(),cam,projection,reflect);
        }
    };

    if(page==1){
        // 0x41073E..0x410ADB: standard enemy/bonus information page.
        LegacyTransform::Matrix34 airA=LegacyTransform::identity();
        LegacyTransform::rotateZ(airA,informationSpin2048_);
        // r230 DIRECT EXE 0x41073E..0x410798: both airborne enemies are
        // submitted while ZFUNC is still ALWAYS; LEQUAL is restored only after
        // the second 0x40C350 call. Keep them out of the later LEQUAL batch.
        addAlwaysM(airEnemyModels_[2],airA,-0.1000000015f,0.0130000003f,0.1199999973f,camera);
        LegacyTransform::Matrix34 airB=LegacyTransform::identity();
        LegacyTransform::rotateZ(airB,informationSpin2048_);
        addAlwaysM(airEnemyModels_[0],airB,-0.1000000015f,0.0370000005f,0.1199999973f,camera);

        // 0x41079D..0x41080D: crawler/mine rotates around Z and receives the
        // second X tilt from half the shared information angle.
        LegacyTransform::Matrix34 mine=LegacyTransform::identity();
        LegacyTransform::rotateZ(mine,informationSpin2048_);
        LegacyTransform::rotateX(mine,informationSpin2048_/2);
        addM(groundEnemyModel_,mine,0.0250000004f,0.0120000001f,0.1000000015f,camera);
        addReflectionM(groundEnemyModel_,mine,0.0250000004f,0.0120000001f,0.1000000015f,0.5500000119f,camera);

        // r194 DIRECT EXE 0x410AFC..0x410CB8: "Ваше устройство" Xonix, which
        // r6x..r193 never drew. Own camera 0x40C250(0,0,0, 0,-260,0) under
        // texture 3. Local phase p += dt*.005 (never wrapped in the EXE):
        //   propeller master 0x2583738 is Y-rotated by -5p (0x401160);
        //   centre X=.04+.03*cos(-.2p), Z=.1+.02*sin(-.2p), Y=-.05 for the
        //   propeller 0x25849D8 and body 0x2585A98;
        //   four nodes 0x25B5B28 at Y=-.044 offset by (c,s),(s,-c),(-s,c),
        //   (-c,-s) with c=.004*cos p, s=.004*sin p.
        {
            informationXonixPhase_=LegacyInformationTransition::advanceXonixPhase(informationXonixPhase_,dt);
            const float ph=informationXonixPhase_;
            const CameraState xonixCamera{0.f,0.f,0.f,0,-260,0};
            const float c=std::cos(ph)*0.004000000189989805f, sn=std::sin(ph)*0.004000000189989805f;
            const float q=ph*-0.20000000298023224f;
            const float cx=std::cos(q)*0.029999999329447746f+0.03999999910593033f;
            const float cz=std::sin(q)*0.019999999552965164f+0.10000000149011612f;
            LegacyTransform::Matrix34 prop=LegacyTransform::identity();
            LegacyTransform::rotateYRad(prop,ph*-5.f);
            const LegacyTransform::Matrix34 id=LegacyTransform::identity();
            // Match the gameplay submit order: body first, then central propeller, then four nodes.
            // DIRECT EXE 0x410AE2..0x410AFE flushes the reflection pass and
            // restores texture3 before this Xonix presentation. Keep it in a
            // distinct late batch instead of mixing it with earlier objects.
            appendLegacyMesh(xonixModel_.body,id,cx,-0.05000000074505806f,cz,xonixCamera,projection,xonixLate);
            appendLegacyMesh(xonixModel_.propeller,prop,cx,-0.05000000074505806f,cz,xonixCamera,projection,xonixLate);
            const float off[4][2]={{c,sn},{sn,-c},{-sn,c},{-c,-sn}};
            for(const auto& o:off)
                appendLegacyMesh(xonixModel_.rotorNode,id,cx+o[0],-0.04399999976158142f,cz+o[1],xonixCamera,projection,xonixLate);
        }

        struct Item {int type;float x,y,z,refl;};
        const Item p[]={{0,0.025f,-0.030f,0.100f,0.65f},{5,-0.080f,-0.0099f,0.100f,0.95f},{3,0.025f,-0.050f,0.100f,0.75f},{1,-0.080f,-0.050f,0.100f,0.95f},{4,0.025f,-0.0099f,0.100f,0.95f},{2,-0.080f,-0.030f,0.100f,0.30f}};
        for(const auto& q:p){
            LegacyTransform::Matrix34 m=LegacyTransform::identity();
            // DIRECT EXE: 0x401400 is Z rotation; the old native page had
            // mistaken 0x190 for a Y rotation, putting the pickups on their side.
            if(q.type==2){
                LegacyTransform::rotateY(m,-informationSpin2048_);
            }else{
                if(q.type==5)LegacyTransform::rotateY(m,0x32);
                int rz=informationSpin2048_;
                if(q.type==3)rz=(rz+0x12c)&0x7ff;
                if(q.type==1 || q.type==4)rz=(0xc8-rz)&0x7ff;
                LegacyTransform::rotateZ(m,rz);
                LegacyTransform::rotateX(m,0x190);
            }
            addM(pickupModels_[std::size_t(q.type)],m,q.x,q.y,q.z,camera);
            addReflectionM(pickupModels_[std::size_t(q.type)],m,q.x,q.y,q.z,q.refl,camera);
        }
    }else{
        // DIRECT EXE 0x414337..0x4145F7. Reconstruct the three additional
        // enemy presentations from their actual matrices instead of separating
        // compound parts into guessed screen positions.
        const CameraState leadCamera{0.f,0.f,0.f,0,-512,0};
        LegacyTransform::Matrix34 flying=LegacyTransform::identity();
        LegacyTransform::rotateY(flying,(informationLeadPhase_>>4)&0x7ff);
        LegacyTransform::rotateX(flying,0xa0);
        // 0x414337..0x41439E: dedicated model 0x25B5ADC/0x257F56C
        // from builder 0x422CBB, under logical texture 0.
        appendLegacyMesh(informationFlyingMineModel_,flying,0.f,-0.5f,0.200000003f,
                         leadCamera,projection,flyingMine);

        const float sway=std::sin(informationSwayPhase_)*0.20000000298023224f;
        LegacyTransform::Matrix34 homing=LegacyTransform::identity();
        LegacyTransform::rotateY(homing,informationSpin2048_/2);
        LegacyTransform::rotateZRad(homing,sway);
        addM(homingModel_.body,homing,-0.005000000f,-0.020000000f,0.060000000f,camera);
        addReflectionM(homingModel_.body,homing,-0.005000000f,-0.020000000f,0.060000000f,0.1800000072f,camera);

        LegacyTransform::Matrix34 dome=LegacyTransform::identity();
        const float domeX=-0.0049999999f-std::sin(sway)*0.00300000003f;
        const float domeY=std::cos(informationSpecialPhase_)*0.00100000005f-0.0182999987f;
        addM(homingModel_.dome,dome,domeX,domeY,0.060000000f,camera);

        LegacyTransform::Matrix34 star=LegacyTransform::identity();
        const int phaseAngle=int(informationSpecialPhase_)&0x7ff;
        LegacyTransform::rotateY(star,phaseAngle);
        LegacyTransform::rotateZRad(star,-sway);
        LegacyTransform::Matrix34 core=LegacyTransform::identity();
        LegacyTransform::rotateY(core,(informationSpin2048_*2)&0x7ff);
        LegacyTransform::rotateZRad(core,-sway);
        LegacyTransform::setScale(core,1.0f+std::cos(informationSpecialPhase_*2.f)*0.40000000596f);
        const float coreY=std::sin(informationSpecialPhase_*2.f)*0.00150000001f-0.0299999993f;
        // Keep the visible layer order consistent with gameplay: radial star first, solid core second.
        addM(eraserModel_.star,core,0.0450000018f,coreY,0.060000000f,camera);
        addM(eraserModel_.core,star,0.0450000018f,-0.0299999993f,0.060000000f,camera);

        // DIRECT EXE 0x41460A..0x41477F: resident 128-record fountain.
        // The page-local timer is incremented once and a single 16-record
        // chunk is emitted only when timer > 80; it is then reset to zero
        // (there is no while/catch-up loop).  The records use the same CRT
        // rand() stream as gameplay, so presentation must consume Game::rng_.
        informationFountainSpawnMs_+=dt;
        if(informationFountainSpawnMs_>LegacyInformationFountain::EmitThresholdMs){
            informationFountainSpawnMs_=0;
            // Use the exact shared MSVC rand() stream; the three calls below
            // deliberately preserve the native vy -> vx -> vz order.
            informationFountainHead_=(informationFountainHead_+LegacyInformationFountain::Chunk)&0x7f;
            const float sx=LegacyInformationFountain::spawnX(sway);
            for(int j=0;j<LegacyInformationFountain::Chunk;++j){
                auto& p=informationFountain_[std::size_t(informationFountainHead_+j)];
                p.x=sx; p.y=LegacyInformationFountain::SpawnY; p.z=LegacyInformationFountain::SpawnZ;
                p.vy=LegacyInformationFountain::BaseVy+float((game.nextLegacyPresentationRandom()&0x3f)-0x20)*LegacyInformationFountain::VyRandomScale;
                p.vx=float((game.nextLegacyPresentationRandom()&0x3f)-0x0f)*LegacyInformationFountain::VxzRandomScale;
                p.vz=float((game.nextLegacyPresentationRandom()&0x3f)-0x20)*LegacyInformationFountain::VxzRandomScale;
            }
        }
        LegacyInformationFountain::updateAll(informationFountain_,dt);
        // Rendering is gated by the page fade counter > 0x100 (0x414768).
        if(game.information().fadeCounter>0x100){
            const auto eSetup=LegacyBillboard::eraserDebris(640);
            for(const auto& p:informationFountain_)
                LegacyBillboard::append(p.x,p.y,p.z,camera,projection,eSetup,base);
        }
    }
    auto copy=[](const OutputBatch& in,std::vector<ScreenVertex>& vo,std::vector<std::uint16_t>& io){
        vo.reserve(in.vertices.size());for(const auto& q:in.vertices)vo.push_back({q.clipX,q.clipY,q.clipZ,q.clipW,q.u,q.v,q.light,q.r,q.g,q.b,q.a});io=in.indices;
    };
    copy(base,informationVertices_,informationIndices_);
    copy(alwaysBase,informationAlwaysVertices_,informationAlwaysIndices_);
    copy(flyingMine,informationFlyingMineVertices_,informationFlyingMineIndices_);
    copy(reflect,informationReflectionVertices_,informationReflectionIndices_);
    copy(xonixLate,informationXonixVertices_,informationXonixIndices_);
}

void Renderer::drawBatch(const std::vector<ScreenVertex>& vertices,const std::vector<std::uint16_t>& indices,GLuint texture,bool diagnosticModelUv,bool alphaTest){
    if(vertices.empty()||indices.empty()) return;
    glBindTexture(GL_TEXTURE_2D,texture);
    glUniform1f(uUseTexture_,texture?1.f:0.f);
    if(uAlphaTest_>=0) glUniform1f(uAlphaTest_,alphaTest?1.f:0.f);
    const bool applyUv=diagnosticModelUv && textureLabActive_;
    glUniform1f(uUvFlip_,(applyUv && textureLabVFlip_)?1.f:0.f);
    const float half=(applyUv && textureLabHalfTexel_)?(0.5f/256.f):0.f;
    glUniform2f(uUvOffset_,half,half);
    glBindBuffer(GL_ARRAY_BUFFER,vbo_);
    glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(ScreenVertex),vertices.data(),GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size()*sizeof(std::uint16_t),indices.data(),GL_STREAM_DRAW);
    glEnableVertexAttribArray(aPos_);glEnableVertexAttribArray(aUv_);glEnableVertexAttribArray(aLight_);glEnableVertexAttribArray(aColor_);
    glVertexAttribPointer(aPos_,4,GL_FLOAT,GL_FALSE,sizeof(ScreenVertex),(void*)0);
    glVertexAttribPointer(aUv_,2,GL_FLOAT,GL_FALSE,sizeof(ScreenVertex),(void*)(sizeof(float)*4));
    glVertexAttribPointer(aLight_,1,GL_FLOAT,GL_FALSE,sizeof(ScreenVertex),(void*)(sizeof(float)*6));
    glVertexAttribPointer(aColor_,4,GL_FLOAT,GL_FALSE,sizeof(ScreenVertex),(void*)(sizeof(float)*7));
    glDrawElements(GL_TRIANGLES,static_cast<GLsizei>(indices.size()),GL_UNSIGNED_SHORT,(void*)0);
}


void Renderer::handleTextureLabInput(const InputState& input){
    // Diagnostic Texture Lab must never steal the retail Select/A path. Keep
    // it opt-in for development builds via AIRXONIX_TEXTURE_LAB=1.
    static const bool enabled=[](){const char* e=std::getenv("AIRXONIX_TEXTURE_LAB");return e && e[0]=='1';}();
    if(!enabled){textureLabActive_=false;textureLabPrevSelectAction_=false;return;}
    const bool toggle=input.select && input.action;
    if(toggle && !textureLabPrevSelectAction_){
        textureLabActive_=!textureLabActive_;
        std::fprintf(stderr,"AX_TEXLAB active=%d subject=%d filter=%s vflip=%d halftexel=%d\n",
                     textureLabActive_?1:0,textureLabSubject_,textureLabLinear_?"LINEAR":"NEAREST",
                     textureLabVFlip_?1:0,textureLabHalfTexel_?1:0);
    }
    textureLabPrevSelectAction_=toggle;
    if(!textureLabActive_){
        textureLabPrevLeft_=input.left;textureLabPrevRight_=input.right;
        textureLabPrevUp_=input.up;textureLabPrevDown_=input.down;textureLabPrevAction_=input.action;
        return;
    }
    constexpr int kSubjects=10;
    if(input.left && !textureLabPrevLeft_){textureLabSubject_=(textureLabSubject_+kSubjects-1)%kSubjects;std::fprintf(stderr,"AX_TEXLAB subject=%d\n",textureLabSubject_);}
    if(input.right && !textureLabPrevRight_){textureLabSubject_=(textureLabSubject_+1)%kSubjects;std::fprintf(stderr,"AX_TEXLAB subject=%d\n",textureLabSubject_);}
    if(input.up && !textureLabPrevUp_){
        textureLabLinear_=!textureLabLinear_;
        if(atlas3_){glBindTexture(GL_TEXTURE_2D,atlas3_);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,textureLabLinear_?GL_LINEAR:GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,textureLabLinear_?GL_LINEAR:GL_NEAREST);}
        std::fprintf(stderr,"AX_TEXLAB filter=%s\n",textureLabLinear_?"LINEAR":"NEAREST");
    }
    if(input.down && !textureLabPrevDown_){textureLabVFlip_=!textureLabVFlip_;std::fprintf(stderr,"AX_TEXLAB vflip=%d\n",textureLabVFlip_?1:0);}
    // Action alone changes half-texel; Select+Action is reserved for entering/leaving the lab.
    if(input.action && !input.select && !textureLabPrevAction_){textureLabHalfTexel_=!textureLabHalfTexel_;std::fprintf(stderr,"AX_TEXLAB halftexel=%d\n",textureLabHalfTexel_?1:0);}
    textureLabPrevLeft_=input.left;textureLabPrevRight_=input.right;
    textureLabPrevUp_=input.up;textureLabPrevDown_=input.down;textureLabPrevAction_=input.action;
}

void Renderer::rebuildTextureLabBatch(){
    textureLabAtlasVertices_.clear();textureLabAtlasIndices_.clear();
    textureLabLineVertices_.clear();textureLabLineIndices_.clear();
    textureLabTextVertices_.clear();textureLabTextIndices_.clear();
    auto quad=[&](std::vector<ScreenVertex>& v,std::vector<std::uint16_t>& i,float x,float y,float w,float h,float u0,float vv0,float u1,float vv1,float r,float g,float b){
        if(v.size()>65530u)return;const auto base=static_cast<std::uint16_t>(v.size());
        const float x0=x/320.f-1.f,x1=(x+w)/320.f-1.f,y0=1.f-y/240.f,y1=1.f-(y+h)/240.f;
        v.push_back({x0,y0,0.f,1.f,u0,vv0,1.f,r,g,b,1.f});v.push_back({x1,y0,0.f,1.f,u1,vv0,1.f,r,g,b,1.f});
        v.push_back({x1,y1,0.f,1.f,u1,vv1,1.f,r,g,b,1.f});v.push_back({x0,y1,0.f,1.f,u0,vv1,1.f,r,g,b,1.f});
        i.insert(i.end(),{base,(std::uint16_t)(base+1),(std::uint16_t)(base+2),base,(std::uint16_t)(base+2),(std::uint16_t)(base+3)});
    };
    // Atlas #3 preview, 256x256 pixels shown 1:1 in the 640x480 design space.
    quad(textureLabAtlasVertices_,textureLabAtlasIndices_,16.f,80.f,256.f,256.f,0.f,0.f,1.f,1.f,1.f,1.f,1.f);
    const LegacyMesh* mesh=&xonixModel_.body;
    static const char* names[]={"XONIX BODY","XONIX PROPELLER","XONIX ROTOR NODE","CRAWLER","BONUS SCORE","BONUS TIME","BONUS LIFE","BONUS SLOW","ACCELERATION","BONUS RANDOM"};
    switch(textureLabSubject_){
        case 1:mesh=&xonixModel_.propeller;break; case 2:mesh=&xonixModel_.rotorNode;break; case 3:mesh=&groundEnemyModel_;break;
        case 4:mesh=&pickupModels_[0];break;case 5:mesh=&pickupModels_[1];break;case 6:mesh=&pickupModels_[2];break;
        case 7:mesh=&pickupModels_[3];break;case 8:mesh=&pickupModels_[4];break;case 9:mesh=&pickupModels_[5];break;default:break;
    }
    float u0=1.f,v0=1.f,u1=0.f,v1=0.f;
    for(const auto& q:mesh->vertices){u0=std::min(u0,q.u);u1=std::max(u1,q.u);v0=std::min(v0,q.v);v1=std::max(v1,q.v);}
    float showV0=v0,showV1=v1;
    if(textureLabVFlip_){showV0=1.f-v1;showV1=1.f-v0;}
    const float off=textureLabHalfTexel_?0.5f/256.f:0.f;u0+=off;u1+=off;showV0+=off;showV1+=off;
    const float rx=16.f+u0*256.f,ry=80.f+showV0*256.f,rw=std::max(2.f,(u1-u0)*256.f),rh=std::max(2.f,(showV1-showV0)*256.f);
    // Four untextured red bars mark the selected model's actual UV footprint.
    quad(textureLabLineVertices_,textureLabLineIndices_,rx,ry,rw,2.f,0,0,0,0,1.f,.1f,.1f);
    quad(textureLabLineVertices_,textureLabLineIndices_,rx,ry+rh-2.f,rw,2.f,0,0,0,0,1.f,.1f,.1f);
    quad(textureLabLineVertices_,textureLabLineIndices_,rx,ry,2.f,rh,0,0,0,0,1.f,.1f,.1f);
    quad(textureLabLineVertices_,textureLabLineIndices_,rx+rw-2.f,ry,2.f,rh,0,0,0,0,1.f,.1f,.1f);
    auto text=[&](const std::string& t,int row,float rr,float gg,float bb){float x=292.f,y=80.f+row*24.f;for(unsigned char ch:t){const auto uv=kLegacyFnt4GlyphTrace.uvForGlyph(kLegacyFnt4GlyphTrace.glyphIndex(ch));quad(textureLabTextVertices_,textureLabTextIndices_,x,y,12.f,24.f,uv[0],uv[1],uv[2],uv[3],rr,gg,bb);x+=12.f;}};
    text("TEXTURE LAB",0,0.3f,1.f,1.f);text(names[textureLabSubject_],2,1.f,1.f,0.3f);
    text(textureLabLinear_?"FILTER LINEAR":"FILTER NEAREST",4,1.f,1.f,1.f);
    text(textureLabVFlip_?"V-FLIP ON":"V-FLIP OFF",5,1.f,1.f,1.f);
    text(textureLabHalfTexel_?"HALF-TEXEL ON":"HALF-TEXEL OFF",6,1.f,1.f,1.f);
    char uvbuf[96];std::snprintf(uvbuf,sizeof(uvbuf),"UV %.4f %.4f %.4f %.4f",u0,showV0,u1,showV1);text(uvbuf,8,.8f,.9f,1.f);
    text("LEFT/RIGHT MODEL",11,.7f,.8f,.7f);text("UP FILTER",12,.7f,.8f,.7f);text("DOWN V-FLIP",13,.7f,.8f,.7f);text("A HALF-TEXEL",14,.7f,.8f,.7f);text("SELECT+A EXIT",15,.7f,.8f,.7f);
}

void Renderer::drawTextureLab(){
    rebuildTextureLabBatch();
    glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glDisable(GL_BLEND);glUniform1f(uBrightness_,1.f);
    drawBatch(textureLabAtlasVertices_,textureLabAtlasIndices_,atlas3_);
    drawBatch(textureLabLineVertices_,textureLabLineIndices_,0);
    drawBatch(textureLabTextVertices_,textureLabTextIndices_,font5_);
    glDepthMask(GL_TRUE);
}

void Renderer::drawGame(Game& game){
    if(!program_) return;
    glUseProgram(program_); glUniform3f(uTint_,1.f,1.f,1.f);
    if(game.phase()!=GamePhase::Records){recordsSceneActive_=false;recordsBackdropActive_=false;}
    if(game.phase()==GamePhase::MainMenu){
        // r198 direct xref audit: 0x440D10/14 and 0x0254593C/40/44 are
        // process-lifetime globals.  0x412F90 does not restore their static
        // initializers when returning from Settings/Information/gameplay.
        // The rocking Alawar intro and the -0.5 -> 0 M1 fly-in are therefore
        // one-shot startup presentation, not something replayed on every M1 entry.
        if(!mainMenuActive_){
            mainMenuActive_=true;
            menuLastTicks_=0u; // only resynchronise host dt after another screen
        }
        rebuildMenuBackgroundBatch();
        rebuildMainMenuDecorationBatch(game);
        rebuildHudBatch(game);
        glUseProgram(program_);
        glUniform1f(uBrightness_,1.f);
        glActiveTexture(GL_TEXTURE0);glUniform1i(uTexture_,0);

        // r198: exact visible consequence of 0x411D60 -> 0x411EF0 ordering.
        // While entryPhase is still its static -0.5 value, the original M1
        // presentation has not entered the viewport yet.  The only visible
        // object is the 0x411EF0 LOGO coin on a true black clear.  Once title
        // reveal has become non-zero, 0x411D60 advances entryPhase and the
        // normal menu/background fly-in is allowed to appear.
        if(LegacyMainMenuDecorationTrace::introOnly(menuDecorState_.entryPhase)){
            glClearColor(0.f,0.f,0.f,1.f);
            glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);
            // r209 DIRECT EXE: no dynamic D3D ZWRITEENABLE owner exists; the
            // 0x411EF0 LOGO/LAXY title writes depth just like every other 3-D
            // submission. r190's GLES-only suppression was a stale workaround.
            glDepthMask(LegacyMainMenuDecorationTrace::titleDepthWriteEnabled?GL_TRUE:GL_FALSE);
            drawBatch(menuLogoVertices_,menuLogoIndices_,menuLogo_,false,LegacyMainMenuDecorationTrace::titleAlphaTestEnabled);
            drawBatch(menuLaxyVertices_,menuLaxyIndices_,menuLaxy_,false,LegacyMainMenuDecorationTrace::titleAlphaTestEnabled);
            return;
        }

        // r224 DIRECT EXE 0x413338..0x413351 -> 0x411D60: the two menu
        // background layers and 0x405E40 black frame are all submitted under
        // D3DCMP_ALWAYS with normal depth writes. The caller restores LEQUAL
        // only after 0x411D60 returns.
        glEnable(GL_DEPTH_TEST);glDepthMask(GL_TRUE);glDepthFunc(GL_ALWAYS);
        const std::size_t menuTheme=std::min<std::size_t>(game.mainMenu().themeIndex,menuEnvironmentTextures_.size()-1u);
        drawBatch(menuBg0Vertices_,menuBg0Indices_,menuEnvironmentTextures_[menuTheme][0],false,false);
        drawBatch(menuBg1Vertices_,menuBg1Indices_,menuEnvironmentTextures_[menuTheme][1],false,false);
        drawBatch(legacyFrameVertices_,legacyFrameIndices_,0,false,false);
        // r166 direct-EXE order: title texture 8, LAXY texture 2, world decor
        // texture 3, deferred reflection texture 6, then M1 menu items.
        glDepthFunc(GL_LEQUAL);
        // r209 DIRECT EXE 0x405AB0/0x405A20 + whole-.text state census:
        // ZWRITEENABLE is never changed. The r190 depth-write suppression was
        // a native workaround from before the exact r194 LOGO/LAXY meshes; it
        // diverges from D3D7 and can make later 0x412150 geometry bleed through.
        glDepthMask(LegacyMainMenuDecorationTrace::titleDepthWriteEnabled?GL_TRUE:GL_FALSE);
        drawBatch(menuLogoVertices_,menuLogoIndices_,menuLogo_,false,LegacyMainMenuDecorationTrace::titleAlphaTestEnabled);
        drawBatch(menuLaxyVertices_,menuLaxyIndices_,menuLaxy_,false,LegacyMainMenuDecorationTrace::titleAlphaTestEnabled);
        drawBatch(menuDecorationVertices_,menuDecorationIndices_,menuAtlas3M1_?menuAtlas3M1_:atlas3_,false,kLegacyColorKeyDiscard);
        // r180 direct state trace 0x412EFC..0x412F7D: 0x405A60(1) only
        // enables additive alpha blending.  Legacy ZWRITE remains enabled, and
        // blending stays enabled across the texture-7 / IN2$ ($1000) draw.
        glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE);glDepthMask(GL_TRUE);
        drawBatch(menuDecorationReflectionVertices_,menuDecorationReflectionIndices_,reflectionTexture_,false,false);
        // 0x412F0F selects logical texture 7 while ALPHABLENDENABLE is still 1.
        drawBatch(menuDecorationTexture7Vertices_,menuDecorationTexture7Indices_,atlas7_,false,true);
        // 0x412F7B: 0x405A60(0), only after slot6.
        glDisable(GL_BLEND);
        // r183 caller 0x41342E/0x413444: two airborne subtype-0 selector
        // markers inherit restored logical texture 3 before texture 4 is selected.
        drawBatch(menuSelectorVertices_,menuSelectorIndices_,menuAtlas3M1_?menuAtlas3M1_:atlas3_,false,kLegacyColorKeyDiscard);
        // r341: the original stays at D3DCMP_LESSEQUAL here. Disabling GLES
        // depth made the five M1 planes punch through LOGO/LAXY and 0x412150
        // decorations whenever their projected rectangles overlapped.
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        drawBatch(menuItemVertices_,menuItemIndices_,menuAtlasM1_,false,true);
        if(textureLabActive_)drawTextureLab();
        return;
    }
    mainMenuActive_=false;
    if(game.phase()==GamePhase::ModeSelect){
        rebuildMenuBackgroundBatch();
        rebuildModeSelectDecorationBatch(game);
        rebuildHudBatch(game);
        glUseProgram(program_);glUniform1f(uBrightness_,1.f);
        glActiveTexture(GL_TEXTURE0);glUniform1i(uTexture_,0);
        glDisable(GL_DEPTH_TEST);
        const std::size_t menuTheme=std::min<std::size_t>(game.mainMenu().themeIndex,menuEnvironmentTextures_.size()-1u);
        drawBatch(menuBg0Vertices_,menuBg0Indices_,menuEnvironmentTextures_[menuTheme][0],false,false);
        drawBatch(menuBg1Vertices_,menuBg1Indices_,menuEnvironmentTextures_[menuTheme][1],false,false);
        glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDepthMask(GL_TRUE);
        // r320 DIRECT EXE 0x411B9C..0x411BB6: logical texture 4 owns the
        // prepared M101/slot0 plane before the selector's enemy scene.
        drawBatch(modeSelectSlot0Vertices_,modeSelectSlot0Indices_,menuAtlasM1_,false,true);
        // r319 DIRECT EXE 0x411BB8..0x411D17: texture 3 owns the subtype-0
        // airborne and crawler base passes; queued crawler reflections flush
        // additively through logical texture 6 afterwards.
        drawBatch(modeSelectDecorationVertices_,modeSelectDecorationIndices_,menuAtlas3M1_?menuAtlas3M1_:atlas3_,false,kLegacyColorKeyDiscard);
        glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE);
        drawBatch(modeSelectReflectionVertices_,modeSelectReflectionIndices_,reflectionTexture_,false,false);
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        drawBatch(hudFont5Vertices_,hudFont5Indices_,font5_);
        glEnable(GL_DEPTH_TEST);
        return;
    }
    if(game.phase()==GamePhase::Settings){
        if(!settingsWidgetsActive_){settingsWidgetAngle_=0.f;settingsWidgetsActive_=true;}
        rebuildMenuBackgroundBatch();
        // r226 DIRECT EXE 0x413C9D..0x413CAB: after the shared background,
        // Settings selects logical texture 3 and executes the full 0x412150
        // enemies/bonuses scene. It does not execute 0x411EF0 or M1 selector.
        rebuildMainMenuDecorationBatch(game,false);
        rebuildHudBatch(game);
        rebuildSettingsWidgetBatch(game);
        glUseProgram(program_);
        glUniform1f(uBrightness_,1.f);
        glActiveTexture(GL_TEXTURE0);glUniform1i(uTexture_,0);
        // r225 DIRECT EXE 0x413C84..0x413C9D: Settings enters 0x411D60
        // with ZFUNC=ALWAYS (EBP=0), leaves depth writes enabled, submits the
        // same 0x405E40 2px frame, then restores LEQUAL before later 3-D work.
        glEnable(GL_DEPTH_TEST);glDepthMask(GL_TRUE);glDepthFunc(GL_ALWAYS);
        const std::size_t menuTheme=std::min<std::size_t>(game.settings().themeIndex,menuEnvironmentTextures_.size()-1u);
        drawBatch(menuBg0Vertices_,menuBg0Indices_,menuEnvironmentTextures_[menuTheme][0],false,false);
        drawBatch(menuBg1Vertices_,menuBg1Indices_,menuEnvironmentTextures_[menuTheme][1],false,false);
        drawBatch(legacyFrameVertices_,legacyFrameIndices_,0,false,false);
        glDepthFunc(GL_LEQUAL);
        // r226: 0x413C9D selects M2 logical texture 3, then 0x412150 owns
        // its normal texture-3 base submissions plus deferred texture-6
        // additive reflection flush. The autonomous registered path has no
        // texture-7 banner submission.
        // r350 DIRECT EXE 0x413C55 establishes the screen light from
        // counter>>3. The shared 0x412150 scene receives it here; each Settings
        // row/track/knob later multiplies this same screen light by its own
        // selected/unselected brightness.
        glUniform1f(uBrightness_,LegacySettingsTrace::modelLightScale(game.settings().fadeCounter));
        drawBatch(menuDecorationVertices_,menuDecorationIndices_,menuAtlas3M2_?menuAtlas3M2_:atlas3_,false,kLegacyColorKeyDiscard);
        glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE);glDepthMask(GL_TRUE);
        drawBatch(menuDecorationReflectionVertices_,menuDecorationReflectionIndices_,reflectionTexture_,false,false);
        drawBatch(menuDecorationTexture7Vertices_,menuDecorationTexture7Indices_,atlas7_,false,true);
        glDisable(GL_BLEND);
        glUniform1f(uBrightness_,1.f);
        // HUD text is native 2-D presentation; preserve its existing overlay
        // path while keeping the EXE depth buffer produced by 0x411D60.
        glDisable(GL_DEPTH_TEST);
        drawBatch(hudM2Vertices_,hudM2Indices_,menuAtlasM2_);
    drawBatch(hudFont5Vertices_,hudFont5Indices_,font5_);
        // r154: exact 0x402A50 Settings widgets. Track is submitted while
        // texture slot 4 is active; knob follows after 0x413F4E selects slot 3.
        glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDepthMask(GL_TRUE);
        drawBatch(settingsTrackVertices_,settingsTrackIndices_,menuAtlasM2_);
        drawBatch(settingsKnobVertices_,settingsKnobIndices_,menuAtlas3M2_?menuAtlas3M2_:atlas3_);
        glDisable(GL_DEPTH_TEST);
        drawBatch(hud3Vertices_,hud3Indices_,menuAtlas3M2_?menuAtlas3M2_:atlas3_); // speech on++/off+ (M2 atlas #3)
        if(textureLabActive_)drawTextureLab();
        glEnable(GL_DEPTH_TEST);
        return;
    }
    if(game.phase()==GamePhase::Records){
        settingsWidgetsActive_=false;settingsWidgetAngle_=0.f;
        rebuildRecordsBackdropBatch(game);
        rebuildHudBatch(game);
        rebuildRecordsDecorationBatch(game);
        glUseProgram(program_);glUniform1f(uBrightness_,1.f);
        glActiveTexture(GL_TEXTURE0);glUniform1i(uTexture_,0);
        const std::size_t menuTheme=std::min<std::size_t>(game.mainMenu().themeIndex,menuEnvironmentTextures_.size()-1u);
        // r234 DIRECT EXE 0x40FA55..0x40FAE7: Records owns one fullscreen
        // 0x40E3D0 TL quad on logical texture 1. It is submitted with
        // ZFUNC=ALWAYS and normal depth writes; the two 0x411D60 M1 layers do
        // not participate in this screen.
        glEnable(GL_DEPTH_TEST);glDepthMask(GL_TRUE);glDepthFunc(GL_ALWAYS);glDisable(GL_BLEND);
        drawBatch(recordsBackdropVertices_,recordsBackdropIndices_,menuEnvironmentTextures_[menuTheme][1],false,false);
        // r235 DIRECT EXE 0x40FAEF..0x40FB10: fnt4 grid is flushed with
        // additive alpha while ZFUNC remains ALWAYS.
        glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE);
        drawBatch(hudFont5Vertices_,hudFont5Indices_,font5_);
        glDisable(GL_BLEND);
        // 0x40FB7E / 0x40FC07: the two pickup0 base meshes are still ALWAYS.
        // 0x40F948 called 0x40C190(counter>>3) before the scene, so base-model
        // diffuse is faded in the same 0..248 byte domain as the EXE.
        const float recordsModelFade=LegacyRecordsTransition::modelLightScale(game.records().fadeCounter);
        glUniform1f(uBrightness_,recordsModelFade);
        drawBatch(recordsAlwaysVertices_,recordsAlwaysIndices_,atlas3_,false,kLegacyColorKeyDiscard);
        // 0x40FC33 is the first and only restore of D3DCMP_LESSEQUAL before
        // crawlers, airborne models and the final deferred reflection flush.
        glDepthFunc(GL_LEQUAL);
        drawBatch(recordsDecorationVertices_,recordsDecorationIndices_,atlas3_,false,kLegacyColorKeyDiscard);
        glUniform1f(uBrightness_,1.f);
        glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE);
        drawBatch(recordsDecorationReflectionVertices_,recordsDecorationReflectionIndices_,reflectionTexture_,false,true);
        glDisable(GL_BLEND);
        if(textureLabActive_)drawTextureLab();
        glEnable(GL_DEPTH_TEST);
        return;
    }
    if(game.phase()==GamePhase::Information){
        settingsWidgetsActive_=false;settingsWidgetAngle_=0.f;
        rebuildInformationBackdropBatch(game);
        rebuildHudBatch(game);
        rebuildInformationDecorationBatch(game);
        glUseProgram(program_);glUniform1f(uBrightness_,1.f);
        glActiveTexture(GL_TEXTURE0);glUniform1i(uTexture_,0);
        const std::size_t menuTheme=std::min<std::size_t>(game.mainMenu().themeIndex,menuEnvironmentTextures_.size()-1u);
        // r231 DIRECT EXE: Information does not use 0x411D60's two prepared
        // menu layers. Pages 0/1 submit one fullscreen 0x40E3D0 TL quad under
        // logical texture 0/1; page 2 has no such backdrop call.
        glEnable(GL_DEPTH_TEST);glDepthMask(GL_TRUE);glDepthFunc(GL_ALWAYS);glDisable(GL_BLEND);
        if(game.information().page<=1){
            const int slot=game.information().page==0?LegacyInformationBackdrop::Page0TextureSlot:LegacyInformationBackdrop::Page1TextureSlot;
            drawBatch(informationBackdropVertices_,informationBackdropIndices_,menuEnvironmentTextures_[menuTheme][std::size_t(slot)],false,false);
        }
        // r227 DIRECT EXE 0x414319..0x4143F2. 0x405A20(0) selects
        // D3DCMP_ALWAYS only; ZENABLE and ZWRITE remain enabled. Disabling GL
        // depth here lost the original flying-mine depth-buffer write.
        const float informationModelFade=LegacyInformationTransition::modelLightScale(game.information().fadeCounter);
        const float informationLeadFade=LegacyInformationTransition::leadModelLightScale(game.information().fadeCounter);
        // DIRECT EXE 0x41431E..0x4143A3: the page-3 lead object is deliberately
        // half-lit (counter>>4); normal page models use counter>>3.
        glUniform1f(uBrightness_,informationLeadFade);
        drawBatch(informationFlyingMineVertices_,informationFlyingMineIndices_,
                  menuEnvironmentTextures_[menuTheme][0],false,true);
        glUniform1f(uBrightness_,1.f);
        // r229 DIRECT EXE: all three Information routines flush the 40x15
        // texture-5 text grid while ZFUNC is still ALWAYS, with alpha blending
        // enabled: 0x410470, 0x4106D8 and 0x4143CF respectively.  The old
        // native path deferred pages 0/1 until after their 3D decoration, which
        // changed both ordering and depth-buffer contents.
        glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE);
        drawBatch(hudFont5Vertices_,hudFont5Indices_,font5_);
        glDisable(GL_BLEND);
        // r230: Information page 1 submits its two airborne-enemy models at
        // ZFUNC=ALWAYS after the text and before 0x410798 restores LEQUAL.
        glUniform1f(uBrightness_,informationModelFade);
        drawBatch(informationAlwaysVertices_,informationAlwaysIndices_,atlas3_,true,kLegacyColorKeyDiscard);

        glDepthFunc(GL_LEQUAL);
        drawBatch(informationVertices_,informationIndices_,atlas3_,true,kLegacyColorKeyDiscard);
        glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE);
        drawBatch(informationReflectionVertices_,informationReflectionIndices_,reflectionTexture_,false,true);
        glDisable(GL_BLEND);
        drawBatch(informationXonixVertices_,informationXonixIndices_,atlas3_,true,kLegacyColorKeyDiscard);
        glUniform1f(uBrightness_,1.f);

        if(textureLabActive_)drawTextureLab();
        glEnable(GL_DEPTH_TEST);
        return;
    }
    if(game.phase()==GamePhase::Controls){
        settingsWidgetsActive_=false;settingsWidgetAngle_=0.f;
        rebuildHudBatch(game);
        glUseProgram(program_);glUniform1f(uBrightness_,1.f);
        glActiveTexture(GL_TEXTURE0);glUniform1i(uTexture_,0);
        glDisable(GL_DEPTH_TEST);
        drawBatch(hudFont5Vertices_,hudFont5Indices_,font5_);
        if(textureLabActive_)drawTextureLab();
        glEnable(GL_DEPTH_TEST);
        return;
    }
    settingsWidgetsActive_=false;
    settingsWidgetAngle_=0.f;
    menuLastTicks_=0u;
    if(const char* f=std::getenv("AIRXONIX_DEBUG_FOCUS")){ // debug-only
        const auto& sp=game.specialObjects();
        if(std::strcmp(f,"homing")==0&&sp.homing().active)LegacyCamera::debugFocus()={sp.homing().worldX,sp.homing().worldZ,sp.homing().height};
        else if(std::strcmp(f,"eraser")==0&&sp.eraser().active)LegacyCamera::debugFocus()={sp.eraser().worldX,sp.eraser().worldZ,0.f};
    }
    rebuildBoard(game);
    rebuildLegacyScreenBatch(game);
    rebuildLegacyModelBatch(game);
    rebuildHudBatch(game);
    glUseProgram(program_);
    glUniform1f(uBrightness_,std::clamp(game.brightnessScale()*game.presentationLightScale(),0.f,1.f));
    float tintR=1.f,tintG=1.f,tintB=1.f;
    if(game.paused()){
        // r303 DIRECT EXE 0x41DF8E..0x41DFAD: PAUS is blue-family lighting.
        const float q=std::clamp(kLegacyPauseTrace.lightGray(game.pauseScene().panelY)/255.f,0.f,1.f);
        tintR=q;tintG=q;tintB=1.f;
    }else if(game.phase()==GamePhase::Abort){
        // r304 DIRECT EXE 0x41E630..0x41E6E2. A confirmed Yes switches to
        // the second green-family exit curve; No keeps the normal curve.
        const float y=game.abortConfirm().panelY;
        if(game.abortConfirm().stage==AbortConfirmState::Stage::Leaving && game.abortConfirm().confirmed){
            const float a=std::clamp(LegacyAbortConfirm.yesOuter(y)/255.f,0.f,1.f);
            const float b=std::clamp(LegacyAbortConfirm.yesGreen(y)/255.f,0.f,1.f);
            tintR=a;tintG=b;tintB=a;
        }else{
            const float q=std::clamp(LegacyAbortConfirm.normalGray(y)/255.f,0.f,1.f);
            tintR=q;tintG=1.f;tintB=q;
        }
    }else if(game.phase()==GamePhase::Dying && game.lives()<=0){
        const float q=std::clamp(game.deathScene().zeroLivesGray/255.f,0.f,1.f);
        tintR=1.f;tintG=q;tintB=q;
    }else if(game.phase()==GamePhase::GameOver){
        const float q=std::clamp(game.gameOverScene().visual.zeroLivesGray/255.f,0.f,1.f);
        tintR=1.f;tintG=q;tintB=q;
    }
    glUniform3f(uTint_,tintR,tintG,tintB);
    glActiveTexture(GL_TEXTURE0);glUniform1i(uTexture_,0);

    const auto& env=environmentTextures_[std::min<std::size_t>(game.environmentThemeIndex(),environmentTextures_.size()-1u)];

    // r220 DIRECT EXE 0x42043F..0x420542. D3D ZFUNC=ALWAYS still performs
    // depth testing and depth writes; represent it with GL_ALWAYS rather than
    // glDisable(GL_DEPTH_TEST), which would suppress depth-buffer updates.
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_ALWAYS);
    static const std::string dbgSkip=[](){const char* e=std::getenv("AIRXONIX_DEBUG_SKIP");return std::string(e?e:"");}(); // debug-only

    // texture 2: prepared rim, then SAFE top
    if(dbgSkip.find("walls")==std::string::npos)
        drawBatch(safeScreenVertices_,safeScreenIndices_,env[LegacyFieldRenderState::SafeAndBoundaryTextureSlot],false,false);
    // still ALWAYS: crawler pass #1, then texture-0 non-SAFE floor
    drawBatch(crawlerPass1Vertices_,crawlerPass1Indices_,env[LegacyModels::GroundEnemyPresentation.firstTextureSlot],false,false);
    drawBatch(floorScreenVertices_,floorScreenIndices_,env[LegacyFieldRenderState::FloorTextureSlot],false,false);

    // 0x420482 restores LEQUAL before crawler pass #2.
    glDepthFunc(GL_LEQUAL);
    drawBatch(crawlerPass2Vertices_,crawlerPass2Indices_,env[LegacyModels::GroundEnemyPresentation.secondTextureSlot],false,false);

    // 0x420498 switches back to ALWAYS for the prepared background ring.
    glDepthFunc(GL_ALWAYS);
    drawBatch(backgroundVertices_,backgroundIndices_,env[LegacyFieldRenderState::BackgroundPreparedTextureSlot],false,false);
    // r223 DIRECT EXE 0x4204B6 -> 0x405E40: untextured black 2-pixel TL frame,
    // still under ZFUNC=ALWAYS and with normal depth writes.
    drawBatch(legacyFrameVertices_,legacyFrameIndices_,0,false,false);

    // 0x4204BB restores LEQUAL: low edges (texture 0), then high walls and
    // capture-marker tops (texture 2).
    glDepthFunc(GL_LEQUAL);
    drawBatch(lowEdgeScreenVertices_,lowEdgeScreenIndices_,env[LegacyFieldRenderState::FloorTextureSlot],false,false);
    if(dbgSkip.find("walls")==std::string::npos)
        drawBatch(wallScreenVertices_,wallScreenIndices_,env[LegacyFieldRenderState::SafeAndBoundaryTextureSlot],false,false);
    // r190: XON1 is a physical face of the field in the native renderer, not
    // a foreground overlay. Keep the literal world placement but let normal
    // field depth participate so the fascia remains visually attached to the
    // board instead of appearing permanently in front of the camera.
    // DIRECT EXE 0x420527..0x420542: XON1 is world-positioned but is
    // submitted after 0x405A20(0), i.e. legacy ZFUNC=ALWAYS. Keep the
    // world-space projection from r190, restore the original depth function.
    glDepthFunc(GL_ALWAYS);
    drawBatch(frontLogoScreenVertices_,frontLogoScreenIndices_,atlas3_,false,kLegacyColorKeyDiscard);
    glDepthFunc(GL_LEQUAL);
    drawBatch(screenVertices_,screenIndices_,0);
    // r116: 0x41ADCF owns logical texture slot 4 around the inter-level
    // GAME/GAM2 + COMP/CMP2 cinematic pair. Atlas4 is intentionally separate
    // from the normal texture-3 gameplay model batch.
    if(!game.paused()) drawBatch(interLevelCinematicVertices_,interLevelCinematicIndices_,game.phase()==GamePhase::FinalSequence?atlas4Game_:atlas4_);
    // r150 direct EXE state audit: 0x422DE0 is an opaque diffuse decal.
    // The caller owns ALPHABLENDENABLE=FALSE and ZFUNC=LESSEQUAL; no ZWRITE
    // disable surrounds the submit, so preserve depth writes as well.
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    drawBatch(shadowVertices_,shadowIndices_,env[LegacyFieldRenderState::ShadowTextureSlot]);
    drawBatch(xonixVertices_,xonixIndices_,atlas3_,true,kLegacyColorKeyDiscard);
    drawBatch(modelVertices_,modelIndices_,atlas3_,true,kLegacyColorKeyDiscard);
    // r339 0x415887 selects logical texture 7 for BONU/TIME/LIFE/SLOW/ACCE/TOU2.
    drawBatch(auxiliaryVertices_,auxiliaryIndices_,atlas7_,true,kLegacyColorKeyDiscard);
    // r250+r251 DIRECT EXE 0x41A4E6..0x41A5EC: 0x405A60(1) enables
    // ONE/ONE blending before the complete 0x4055E0 helper pass; logical
    // texture 3 remains selected. Order in the batch is airborne, crawler,
    // then Xonix rotor-tip glints, exactly matching the caller.
    glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE);glDepthMask(GL_TRUE);
    drawBatch(directionalGlintVertices_,directionalGlintIndices_,atlas3_,false,false);
    glDisable(GL_BLEND);
    // r73/r85/r86 direct caller-state trace: shared debris and pickup-smash
    // inherit logical texture 3 with alpha blending OFF. 0x40E120 itself owns
    // neither state. Drawing these as untextured blended quads was a native bug.
    glDisable(GL_BLEND);
    drawBatch(particleVertices_,particleIndices_,atlas3_,false,kLegacyColorKeyDiscard);
    // r340 0x41BEE0 owns ONE/ONE blending around the impact quad and inherits
    // logical texture 3 from the live-world special-object pass.
    if(!deathOverlayVertices_.empty()){
        glEnable(GL_BLEND); glBlendFunc(GL_ONE,GL_ONE); glDepthMask(GL_TRUE);
        drawBatch(deathOverlayVertices_,deathOverlayIndices_,atlas3_,false,false);
        glDisable(GL_BLEND);
    }
    // Deferred environment mapping follows the caller-owned state around
    // legacy 0x40E960. r69 confirms 0x40E960 itself only replays queued models
    // with per-entry brightness and restores legacy brightness to 1.0; it does
    // not toggle alpha/depth/texture state. The surrounding records/gameplay
    // call sites own those switches. 1111 uses the recovered black colour-key
    // as alpha=0 in the native replacement path.
    // r73 U09 literal state contract: alpha blending is globally configured as
    // ONE/ONE and only toggled by the caller. Legacy 0x40E960 never changes
    // depth-write state. Keep depth writes enabled and reproduce only the
    // caller-owned additive-alpha enable around the deferred 1111 pass.
    glEnable(GL_BLEND);
    drawBatch(reflectionVertices_,reflectionIndices_,reflectionTexture_);
    glDisable(GL_BLEND);

    // r287 caller-owned startup overlay state: 0x405A60(1) enables ONE/ONE
    // additive blending and 0x405A20(0) selects ZFUNC=ALWAYS before LEV2/CNT3.
    if(game.levelIntroActive()){
        glDepthFunc(GL_ALWAYS);
        glEnable(GL_BLEND); glBlendFunc(GL_ONE,GL_ONE); glDepthMask(GL_TRUE);
        drawBatch(levelIntroPlaqueVertices_,levelIntroPlaqueIndices_,atlas7_);
        drawBatch(levelIntroDigitsVertices_,levelIntroDigitsIndices_,atlas4_);
        glDisable(GL_BLEND);
        glDepthFunc(GL_LEQUAL);
    }

    // Pause is a modal foreground presentation. Drawing PAUS before the world
    // allowed later depth-tested gameplay geometry to hide it on GLES. Render
    // the prepared pause slot once more as the final world overlay.
    if(game.paused()){
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        drawBatch(interLevelCinematicVertices_,interLevelCinematicIndices_,atlas3_);
        glDepthMask(GL_TRUE);
    }

    // Legacy HUD is a true screen-space pass. It must not participate in the
    // 3D depth buffer and is intentionally kept at full UI brightness.
    glDisable(GL_DEPTH_TEST);
    glUniform1f(uBrightness_,1.f);
    glUniform3f(uTint_,1.f,1.f,1.f);
    drawBatch(hud3Vertices_,hud3Indices_,atlas3_);
    drawBatch(hud4Vertices_,hud4Indices_,atlas4_);
    drawBatch(hud7Vertices_,hud7Indices_,atlas7_);
    drawBatch(hudM1Vertices_,hudM1Indices_,menuAtlasM1_);
    drawBatch(hudM2Vertices_,hudM2Indices_,menuAtlasM2_);
    drawBatch(hudFont5Vertices_,hudFont5Indices_,font5_);
    if(textureLabActive_)drawTextureLab();
    glEnable(GL_DEPTH_TEST);
}
void Renderer::endFrame(SDL_Window* window){SDL_GL_SwapWindow(window);}
