#include "game/legacy_particles.hpp"
#pragma once
#include <SDL.h>
#include "platform/gles2_compat.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include "legacy_mesh.hpp"
#include "legacy_model_factory.hpp"
#include "core/legacy_original_resources.hpp"
#include "game/pickups.hpp"
#include "game/legacy_information_fountain.hpp"
#include "platform/input.hpp"
class Game;

class Renderer {
public:
    ~Renderer();
    bool init(SDL_Window* window,const std::string& texturesPath,const std::string& originalExePath={});
    void resize(SDL_Window* window);
    void beginFrame();
    void drawGame(Game& game);
    void setGameFrameDt(int dtMs){ gameFrameDt_=dtMs; }
    void endFrame(SDL_Window* window);
    void syncLevelLoad(std::uint64_t serial);
    void spawnPickupSmash(const PickupSmashEvent& event);
    void handleTextureLabInput(const InputState& input);
    bool textureLabActive() const { return textureLabActive_; }
private:
    struct Vertex { float x,y,z,r,g,b,a; };
    struct WorldTexVertex { float x,y,z,u,v,r,g,b,a; };
    struct ScreenVertex { float x,y,z,w,u,v,light,r,g,b,a; };
    bool initProgram();
    bool initGameplayAtlas(const std::string& texturesPath);
    bool initUiAtlas(const std::string& texturesPath);
    bool initAuxAtlas(const std::string& texturesPath);
    void initMenuAtlases(const std::string& texturesPath);
    bool initEnvironmentTextures(const std::string& texturesPath);
    bool initReflectionTexture(const std::string& texturesPath);
    bool initMenuEnvironmentTextures(const std::string& texturesPath);
    GLuint uploadTexture(const struct LegacyAtlasImage& image,bool repeat);
    void initLegacyModels();
    void rebuildBoard(const Game& game);
    void rebuildLegacyScreenBatch(const Game& game);
    void rebuildLegacyModelBatch(const Game& game);
    void rebuildBackgroundBatch();
    void rebuildLegacyScreenFrameBatch();
    void rebuildMenuBackgroundBatch();
    void rebuildMainMenuDecorationBatch(const Game& game,bool mainMenuCaller=true);
    void rebuildModeSelectDecorationBatch(const Game& game);
    void rebuildHudBatch(const Game& game);
    void rebuildSettingsWidgetBatch(const Game& game);
    void rebuildRecordsBackdropBatch(const Game& game);
    void rebuildInformationBackdropBatch(const Game& game);
    void rebuildInformationDecorationBatch(Game& game);
    void rebuildRecordsDecorationBatch(const Game& game);
    void drawBatch(const std::vector<ScreenVertex>& vertices,const std::vector<std::uint16_t>& indices,GLuint texture,bool diagnosticModelUv=false,bool alphaTest=true);
    void rebuildTextureLabBatch();
    void drawTextureLab();
    GLuint program_=0,vbo_=0,ibo_=0,atlas3_=0,atlas4_=0,atlas4Game_=0,atlas7_=0,menuAtlasM1_=0,menuAtlas3M1_=0,menuAtlas3M2_=0,menuAtlasM2_=0,font5_=0,menuLogo_=0,menuLaxy_=0,reflectionTexture_=0;
    std::array<std::array<GLuint,3>,12> environmentTextures_{}; // exact 12 gameplay themes x logical slots 0/1/2
    std::array<std::array<GLuint,2>,8> menuEnvironmentTextures_{}; // all exact M1/M2 least-used theme pairs
    GLint aPos_=-1,aUv_=-1,aLight_=-1,aColor_=-1,uBrightness_=-1,uTint_=-1,uUseTexture_=-1,uTexture_=-1,uUvFlip_=-1,uUvOffset_=-1,uAlphaTest_=-1;
    std::vector<Vertex> vertices_;
    std::vector<WorldTexVertex> floorWorldVertices_,wallWorldVertices_,frontLogoWorldVertices_;
    std::vector<ScreenVertex> screenVertices_,safeScreenVertices_,floorScreenVertices_,lowEdgeScreenVertices_,wallScreenVertices_,frontLogoScreenVertices_,backgroundVertices_,legacyFrameVertices_;
    std::vector<ScreenVertex> menuBg0Vertices_,menuBg1Vertices_,menuLogoVertices_,menuLaxyVertices_,menuItemVertices_,menuDecorationVertices_,menuDecorationTexture7Vertices_,menuDecorationReflectionVertices_,menuSelectorVertices_;
    std::vector<ScreenVertex> modeSelectSlot0Vertices_,modeSelectDecorationVertices_,modeSelectReflectionVertices_;
    std::vector<std::uint16_t> screenIndices_,safeScreenIndices_,floorScreenIndices_,lowEdgeScreenIndices_,wallScreenIndices_,frontLogoScreenIndices_,backgroundIndices_,legacyFrameIndices_;
    std::vector<std::uint16_t> menuBg0Indices_,menuBg1Indices_,menuLogoIndices_,menuLaxyIndices_,menuItemIndices_,menuDecorationIndices_,menuDecorationTexture7Indices_,menuDecorationReflectionIndices_,menuSelectorIndices_;
    std::vector<std::uint16_t> modeSelectSlot0Indices_,modeSelectDecorationIndices_,modeSelectReflectionIndices_;
    std::vector<ScreenVertex> modelVertices_,xonixVertices_,directionalGlintVertices_,shadowVertices_,reflectionVertices_,particleVertices_,auxiliaryVertices_,deathOverlayVertices_;
    std::vector<ScreenVertex> crawlerPass1Vertices_,crawlerPass2Vertices_;
    std::vector<ScreenVertex> interLevelCinematicVertices_;
    std::vector<ScreenVertex> levelIntroPlaqueVertices_,levelIntroDigitsVertices_;
    std::vector<std::uint16_t> modelIndices_,xonixIndices_,directionalGlintIndices_,shadowIndices_,reflectionIndices_,particleIndices_,auxiliaryIndices_,deathOverlayIndices_;
    std::vector<std::uint16_t> crawlerPass1Indices_,crawlerPass2Indices_;
    std::vector<std::uint16_t> interLevelCinematicIndices_;
    std::vector<std::uint16_t> levelIntroPlaqueIndices_,levelIntroDigitsIndices_;
    std::vector<ScreenVertex> hud3Vertices_,hud4Vertices_,hud7Vertices_,hudM1Vertices_,hudM2Vertices_,hudFont5Vertices_;
    std::vector<ScreenVertex> settingsTrackVertices_,settingsKnobVertices_;
    std::vector<ScreenVertex> recordsBackdropVertices_,recordsAlwaysVertices_,informationBackdropVertices_,informationVertices_,informationAlwaysVertices_,informationFlyingMineVertices_,informationReflectionVertices_,informationXonixVertices_;
    std::vector<ScreenVertex> recordsDecorationVertices_,recordsDecorationReflectionVertices_;
    std::vector<std::uint16_t> hud3Indices_,hud4Indices_,hud7Indices_,hudM1Indices_,hudM2Indices_,hudFont5Indices_;
    std::vector<std::uint16_t> settingsTrackIndices_,settingsKnobIndices_;
    std::vector<std::uint16_t> recordsBackdropIndices_,recordsAlwaysIndices_,informationBackdropIndices_,informationIndices_,informationAlwaysIndices_,informationFlyingMineIndices_,informationReflectionIndices_,informationXonixIndices_;
    std::vector<std::uint16_t> recordsDecorationIndices_,recordsDecorationReflectionIndices_;
    std::vector<ScreenVertex> textureLabAtlasVertices_,textureLabLineVertices_,textureLabTextVertices_;
    std::vector<std::uint16_t> textureLabAtlasIndices_,textureLabLineIndices_,textureLabTextIndices_;
    XonixModelParts xonixModel_{};
    LegacyMainMenuLogoParts menuLogoModel_{};
    std::array<LegacyMesh,7> menuDecorationModels_{};
    LegacyMesh groundEnemyModel_{};
    std::array<LegacyMesh,4> airEnemyModels_{};
    LegacyMesh airEnemyShadowModel_{};
    std::array<LegacyMesh,6> pickupModels_{};
    std::array<LegacyMesh,6> auxiliaryModels_{};
    LegacyMesh trailNormalModel_{};
    LegacyMesh trailDamagedModel_{};
    LegacyMesh settingsTrackModel_{},settingsKnobModel_{};
    LegacyMesh interLevelCinematicSlot0_{},levelIntroCinematicSlot1_{},pauseCinematicSlot2_{},gameOverCinematicSlot3_{},abortCinematicSlot4_{},interLevelCinematicSlot5_{};
    EraserModelParts eraserModel_{};
    HomingModelParts homingModel_{};
    LegacyMesh informationFlyingMineModel_{};
    struct PickupSmashParticleRuntime { float x=0.f,y=0.f,z=0.f,vx=0.f,vy=0.f,vz=0.f; };
    struct PickupSmashSlotRuntime {
        std::array<PickupSmashParticleRuntime,LegacyParticles::PickupCount> particles{};
        int ageMs=5000; // 0x419180 initializes all six native slot timers to 0x1388.
    };
    std::array<PickupSmashSlotRuntime,6> pickupSmashSlots_{};
    std::uint64_t observedLevelLoadSerial_=0;
    std::uint32_t particleLastTicks_=0u; // retained for ABI/source compatibility; gameplay particles use gameFrameDt_
    int gameFrameDt_=0;
    float menuPrimaryPhase_=0.f,menuSecondaryPhase_=0.f,menuDecorationPhase_=0.f;
    // Process-lifetime initializers copied from .data/BSS: 0x440D10=.06,
    // 0x440D14=-.5, 0x0254593C/40/44=0.  Do not reset on M1 re-entry.
    struct MenuDecorStateStorage { float titleDrop=.06f,titleReveal=0.f,titlePhase=0.f; int titleWait=0; bool titleChime=true; float sceneA=0.f,sceneB=0.f,sceneC=0.f; int sceneAngle=0,sceneTimer=0,selectorAngle=0; float entryPhase=-0.5f; } menuDecorState_{};
    float settingsWidgetAngle_=0.f; int menuFrameDt_=0; bool settingsWidgetsActive_=false;
    float recordsDecorPhase_=0.f,recordsBackdropPhase_=0.f;
    bool recordsSceneActive_=false,recordsBackdropActive_=false;
    std::array<float,6> recordsCrawlerLane_{{0.08f,0.026666667f,-0.026666667f,0.053333335f,0.f,-0.053333335f}};
    int informationSpin2048_=0; int informationLeadPhase_=0; float informationSwayPhase_=0.f,informationSpecialPhase_=0.f,informationXonixPhase_=0.f; int informationLastPage_=0;
    float informationBackdropPhase_=0.f; int informationBackdropLastPage_=-1;
    // Resident global pool 0x0257E8D8: unlike the page-3 local head/timer it
    // is BSS-backed and survives leaving/re-entering Information.
    std::array<LegacyInformationFountain::Particle,LegacyInformationFountain::Count> informationFountain_{};
    int informationFountainHead_=0, informationFountainSpawnMs_=0;
    std::uint32_t menuLastTicks_=0u;
    bool mainMenuActive_=false;
    airxonix::LegacyOriginalResources originalResources_{};
    int viewportX_=0,viewportY_=0,viewportW_=640,viewportH_=480;
    bool textureLabActive_=false,textureLabLinear_=false,textureLabVFlip_=false,textureLabHalfTexel_=false;
    bool textureLabPrevSelectAction_=false,textureLabPrevLeft_=false,textureLabPrevRight_=false,textureLabPrevUp_=false,textureLabPrevDown_=false,textureLabPrevAction_=false;
    int textureLabSubject_=0;
};
