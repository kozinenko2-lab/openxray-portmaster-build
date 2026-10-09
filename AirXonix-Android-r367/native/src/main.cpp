#include <array>
#include <algorithm>
#include <SDL.h>
#include "platform/gles2_compat.hpp"
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <csignal>
#include <unistd.h>
#if defined(__linux__) && !defined(__ANDROID__)
#include <ucontext.h>
#endif
#include <string>
#include <vector>
#include <fstream>
#include <iterator>
#include "core/legacy_original_resources.hpp"
#include "core/cleanroom_archive.hpp"
#include "audio/music.hpp"
#include "audio/legacy_audio_trace.hpp"
#include "core/legacy_clock.hpp"
#include "core/paths.hpp"
#include "game/game.hpp"
#include "platform/input.hpp"
#include "render/renderer.hpp"
#include "render/legacy_camera.hpp"
#include "render/legacy_main_menu_decor.hpp"


extern "C" void glReadPixels(GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,void*);
static inline void axReadPixels(GLint a,GLint b,GLsizei c,GLsizei d,GLenum e,GLenum f,void* g){glReadPixels(a,b,c,d,e,f,g);}
namespace {
void writeAll(int fd,const char* s,std::size_t n){
    while(n){const ssize_t w=::write(fd,s,n);if(w<=0)break;s+=w;n-=static_cast<std::size_t>(w);}
}
void writeText(int fd,const char* s){std::size_t n=0;while(s[n])++n;writeAll(fd,s,n);}
void writeHex(int fd,std::uintptr_t value){
    char b[2+sizeof(std::uintptr_t)*2+1]{};b[0]='0';b[1]='x';
    static constexpr char kHex[]="0123456789abcdef";
    for(std::size_t i=0;i<sizeof(std::uintptr_t)*2;++i){const unsigned shift=unsigned((sizeof(std::uintptr_t)*2-1-i)*4);b[2+i]=kHex[(value>>shift)&0xFu];}
    b[2+sizeof(std::uintptr_t)*2]='\n';writeAll(fd,b,sizeof(b)-1);
}
void writeUnsigned(int fd,unsigned value){
    char b[16]{};std::size_t n=0;do{b[n++]=char('0'+value%10u);value/=10u;}while(value&&n<sizeof(b));
    for(std::size_t i=0;i<n/2;++i){const char t=b[i];b[i]=b[n-1-i];b[n-1-i]=t;}writeAll(fd,b,n);
}
void crashSignalHandler(int sig,siginfo_t* info,void* context){
    writeText(STDERR_FILENO,"AX_CRASH signal=");writeUnsigned(STDERR_FILENO,static_cast<unsigned>(sig));writeText(STDERR_FILENO," addr=");
    writeHex(STDERR_FILENO,reinterpret_cast<std::uintptr_t>(info?info->si_addr:nullptr));
#if defined(__aarch64__) && defined(__linux__) && !defined(__ANDROID__)
    auto* uc=reinterpret_cast<ucontext_t*>(context);
    writeText(STDERR_FILENO,"AX_CRASH pc=");writeHex(STDERR_FILENO,static_cast<std::uintptr_t>(uc->uc_mcontext.pc));
    writeText(STDERR_FILENO,"AX_CRASH sp=");writeHex(STDERR_FILENO,static_cast<std::uintptr_t>(uc->uc_mcontext.sp));
#endif
    _exit(128+sig);
}
void installCrashHandlers(){
    struct sigaction sa{};sigemptyset(&sa.sa_mask);sa.sa_sigaction=crashSignalHandler;sa.sa_flags=SA_SIGINFO|SA_RESETHAND;
    const int signals[]={SIGILL,SIGSEGV,SIGBUS,SIGABRT,SIGFPE};for(int sig:signals)sigaction(sig,&sa,nullptr);
}

int envIntClamped(const char* name,int fallback,int lo,int hi){
    const char* v=std::getenv(name);
    if(!v||!*v)return fallback;
    char* end=nullptr;const long n=std::strtol(v,&end,10);
    if(end==v)return fallback;
    return static_cast<int>(std::clamp(n,long(lo),long(hi)));
}

bool createGlesWindow(SDL_Window*& window,SDL_GLContext& gl){
    struct Attempt { int r,g,b,depth; Uint32 flags; const char* name; };
    const Attempt attempts[]={
        {5,6,5,16,SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN_DESKTOP|SDL_WINDOW_ALLOW_HIGHDPI,"ES2 RGB565 fullscreen-desktop"},
        {8,8,8,16,SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN_DESKTOP,"ES2 RGB888 fullscreen-desktop"},
        {5,6,5,16,SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN,"ES2 RGB565 fullscreen"},
        {8,8,8,24,SDL_WINDOW_OPENGL,"ES2 RGB888 windowed fallback"}
    };
    for(std::size_t i=0;i<sizeof(attempts)/sizeof(attempts[0]);++i){
        const auto& a=attempts[i];SDL_GL_ResetAttributes();
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_ES);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,a.depth);SDL_GL_SetAttribute(SDL_GL_RED_SIZE,a.r);SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,a.g);SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,a.b);
        std::fprintf(stderr,"AX_GL attempt=%zu mode=%s\n",i,a.name);
        window=SDL_CreateWindow("AirXonix Native",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,640,480,a.flags);
        if(!window){std::fprintf(stderr,"AX_GL window failed: %s\n",SDL_GetError());continue;}
        gl=SDL_GL_CreateContext(window);
        if(gl){std::fprintf(stderr,"AX_GL context success attempt=%zu\n",i);return true;}
        std::fprintf(stderr,"AX_GL context failed: %s\n",SDL_GetError());SDL_DestroyWindow(window);window=nullptr;
    }
    return false;
}
}

int main(int argc,char** argv){
    (void)argc;
    std::setvbuf(stdout,nullptr,_IONBF,0);std::setvbuf(stderr,nullptr,_IONBF,0);
    installCrashHandlers();
    std::fprintf(stderr,"AX_BOOT 00 main entered; crash handlers installed\n");
    std::fprintf(stderr,"AX_BOOT 01 SDL_Init begin\n"); std::fflush(stderr);
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_GAMECONTROLLER|SDL_INIT_TIMER)!=0){std::fprintf(stderr,"SDL_Init: %s\n",SDL_GetError());return 1;}
    std::fprintf(stderr,"AX_BOOT 02 SDL_Init OK\n");
    SDL_version compiled{},linked{};SDL_VERSION(&compiled);SDL_GetVersion(&linked);
    std::fprintf(stderr,"AX_SDL compiled=%u.%u.%u linked=%u.%u.%u\n",compiled.major,compiled.minor,compiled.patch,linked.major,linked.minor,linked.patch);
    const int videoDrivers=SDL_GetNumVideoDrivers();for(int i=0;i<videoDrivers;++i)std::fprintf(stderr,"AX_SDL video_driver[%d]=%s\n",i,SDL_GetVideoDriver(i));
    const int audioDrivers=SDL_GetNumAudioDrivers();for(int i=0;i<audioDrivers;++i)std::fprintf(stderr,"AX_SDL audio_driver[%d]=%s\n",i,SDL_GetAudioDriver(i));
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");SDL_JoystickEventState(SDL_ENABLE);SDL_GameControllerEventState(SDL_ENABLE);SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS,"0");
    std::fprintf(stderr,"AX_BOOT 03 creating window/GLES context\n");
    SDL_Window* window=nullptr;SDL_GLContext gl=nullptr;
    if(!createGlesWindow(window,gl)){std::fprintf(stderr,"AX_BOOT GLES creation exhausted all fallbacks: %s\n",SDL_GetError());SDL_Quit();return 3;}
    std::fprintf(stderr,"AX_BOOT 04 window+GLES context OK\n");
    const int swapRc=SDL_GL_SetSwapInterval(1);std::fprintf(stderr,"AX_GL swap_interval rc=%d err=%s\n",swapRc,swapRc==0?"<none>":SDL_GetError());
    std::fprintf(stderr,"AX_BOOT 05 GLES query begin\n");
    std::fprintf(stdout,"SDL video driver: %s\n",SDL_GetCurrentVideoDriver()?SDL_GetCurrentVideoDriver():"<unknown>");
    std::fprintf(stdout,"GL_VENDOR: %s\n",glGetString(GL_VENDOR)?reinterpret_cast<const char*>(glGetString(GL_VENDOR)):"<null>");
    std::fprintf(stdout,"GL_RENDERER: %s\n",glGetString(GL_RENDERER)?reinterpret_cast<const char*>(glGetString(GL_RENDERER)):"<null>");
    std::fprintf(stdout,"GL_VERSION: %s\n",glGetString(GL_VERSION)?reinterpret_cast<const char*>(glGetString(GL_VERSION)):"<null>");
    std::fprintf(stdout,"GLSL: %s\n",glGetString(GL_SHADING_LANGUAGE_VERSION)?reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)):"<null>");
    const GamePaths paths=makeGamePaths(argv&&argv[0]?argv[0]:nullptr);std::fprintf(stdout,"AirXonix root: %s\nAssets: %s\nOriginal EXE: %s\nOriginal MUSIC: %s\n",paths.root.c_str(),paths.assets.c_str(),paths.originalExe.empty()?"<none>":paths.originalExe.c_str(),paths.originalMusic.empty()?"<none>":paths.originalMusic.c_str());
    const bool forceCleanroom=std::getenv("AIRXONIX_FORCE_CLEANROOM")!=nullptr;
    const bool useOriginal=!forceCleanroom && !paths.originalExe.empty();
    std::fprintf(stdout,"AX_RESOURCE_MODE %s\n",useOriginal?"original-fidelity-with-cleanroom-fallback":"cleanroom-standalone");
    // r208: clean-room assets live in one private ZIP namespace so names such
    // as music/00.mus can never collide with an original loose MUSIC folder.
    std::string cleanZip=firstExistingFile({paths.root+"/AirXonix-cleanroom.zip",paths.assets+"/AirXonix-cleanroom.zip",paths.assets+"/cleanroom.zip"});
    if(!cleanZip.empty()){
        std::string zipError;
        if(airxonix::CleanroomArchive::instance().open(cleanZip,&zipError))
            std::fprintf(stdout,"AX_RESOURCE clean-room ZIP=%s entries=%zu\n",cleanZip.c_str(),airxonix::CleanroomArchive::instance().entryCount());
        else std::fprintf(stderr,"AX_RESOURCE clean-room ZIP open failed: %s\n",zipError.c_str());
    }
    std::fprintf(stderr,"AX_BOOT 06 renderer init begin\n"); std::fflush(stderr);
    Renderer renderer;if(!renderer.init(window,paths.textures,useOriginal?paths.originalExe:std::string{})){std::fprintf(stderr,"Renderer GLES2 init failed\n");SDL_GL_DeleteContext(gl);SDL_DestroyWindow(window);SDL_Quit();return 4;}std::fprintf(stdout,"Renderer GLES2 init: OK\n"); std::fprintf(stderr,"AX_BOOT 07 renderer init OK\n"); std::fflush(stderr);
    // r198 DIRECT EXE 0x424D09..0x424D13: after the legacy resources are
    // constructed, the inner game sleeps exactly 0x320 (800) ms before audio
    // setup and the first 0x412F90 M1 frame.  Present a true-black frame first
    // instead of hiding this original pre-roll behind host initialization.
    glClearColor(0.f,0.f,0.f,1.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    SDL_GL_SwapWindow(window);
    SDL_Delay(LegacyMainMenuDecorationTrace::startupBlackPreRollMs);
    std::fprintf(stderr,"AX_BOOT 07a original 800ms black pre-roll complete\n");
    // r198 wrapper + inner-EXE audit: RT_BITMAP/143 is still NOT the splash.
    // The outer Alawar wrapper reads partner.ini video=splash.exe, starts that
    // process and waits for it before RunApplication.  Independently, the inner
    // game owns the rocking LOGO coin at 0x411EF0.  r198 reconstructs that inner
    // presentation and its exact 800 ms pre-roll; do not resurrect the r174
    // RT_BITMAP/143 substitution.  The missing external splash.exe payload is
    // not required to model the observed 0x411EF0 logo/menu ordering correctly.
    InputSystem inputSystem;InputState input;MusicPlayer music;
    const bool vibrationEnabled=envIntClamped("AIRXONIX_VIBRATION_ENABLED",1,0,1)!=0;
    const int vibrationDurationMs=envIntClamped("AIRXONIX_VIBRATION_DURATION_MS",260,0,5000);
    const int vibrationStrengthPercent=envIntClamped("AIRXONIX_VIBRATION_STRENGTH",82,0,100);
    std::fprintf(stderr,"AX_RUMBLE enabled=%d duration_ms=%d strength=%d\n",vibrationEnabled?1:0,vibrationDurationMs,vibrationStrengthPercent);
    if(useOriginal)music.setLegacyMusicSearchPaths({paths.originalMusic,paths.music});
    else music.setLegacyMusicSearchPaths({paths.music});
    std::vector<std::uint8_t> wavePackDirectory;
    const std::string cleanWavePack=firstExistingFile({paths.raw+"/WAVEPACK.bin",paths.raw+"/wavepack.bin"});
    if(useOriginal){
        airxonix::LegacyOriginalResources raw;std::string error;
        if(raw.open(paths.originalExe,&error))raw.loadWavePack(wavePackDirectory,&error);
    }
    if(wavePackDirectory.empty() && !cleanWavePack.empty()){std::ifstream f(cleanWavePack,std::ios::binary);wavePackDirectory.assign(std::istreambuf_iterator<char>(f),{});}
    if(wavePackDirectory.empty())airxonix::CleanroomArchive::instance().read("raw/WAVEPACK.bin",wavePackDirectory);
    const std::string sfxBank=useOriginal
        ? firstExistingFile({paths.originalMusic+"/29.mus",paths.originalMusic+"/29.MUS",paths.music+"/29.mus",paths.music+"/29.MUS"})
        : firstExistingFile({paths.music+"/29.mus",paths.music+"/29.MUS"});
    if(!sfxBank.empty()){if(!wavePackDirectory.empty())music.loadLegacySfxBank(sfxBank,wavePackDirectory);else music.loadLegacySfxBank(sfxBank);}
    else {std::vector<std::uint8_t> zippedBank;if(airxonix::CleanroomArchive::instance().read("music/29.MUS",zippedBank) && !wavePackDirectory.empty())music.loadLegacySfxBankBytes(std::move(zippedBank),wavePackDirectory);}
    const std::string testTrack=useOriginal
        ? firstExistingFile({paths.originalMusic+"/00.mus",paths.originalMusic+"/00.MUS",paths.music+"/00.mus",paths.music+"/00.MUS",paths.music+"/00.wav"})
        : firstExistingFile({paths.music+"/00.mus",paths.music+"/00.MUS",paths.music+"/00.wav"});
    std::vector<std::uint8_t> soundInf;
    if(useOriginal){airxonix::LegacyOriginalResources raw;std::string error;if(raw.open(paths.originalExe,&error))raw.loadSoundInf(soundInf,&error);}
    const std::string cleanSoundInf=firstExistingFile({paths.raw+"/SOUNDINF.bin",paths.raw+"/soundinf.bin"});
    if(soundInf.empty() && !cleanSoundInf.empty()){std::ifstream f(cleanSoundInf,std::ios::binary);soundInf.assign(std::istreambuf_iterator<char>(f),{});}
    if(soundInf.empty())airxonix::CleanroomArchive::instance().read("raw/SOUNDINF.bin",soundInf);
    if(soundInf.empty()){
        soundInf=makeCleanroomSoundInf();
        std::fprintf(stdout,"AX_RESOURCE clean-room SOUNDINF generated in memory (%zu bytes)\n",soundInf.size());
    }
    Game game{soundInf};
    // r301 DIRECT EXE 0x4230C0/0x4230A0: M2 settings use the literal
    // 36-byte gameinf.bin block in the working directory.
    const std::filesystem::path settingsPath=std::filesystem::path(paths.root)/LegacySettingsTrace::fileName;
    if(!game.initializeLegacySettings(settingsPath))
        std::fprintf(stderr,"AX_SETTINGS unable to load/create %s\n",settingsPath.string().c_str());
    else
        std::fprintf(stdout,"AX_SETTINGS path=%s size=%zu\n",settingsPath.string().c_str(),LegacySettingsTrace::fileSize);
    // r303 DIRECT EXE 0x424B33..0x424BA4: master scales are derived from the
    // loaded gameinf.bin before the first music stream is started.
    music.setSfxMasterScale(game.legacySfxMasterScale());
    music.setVolume(game.legacyMusicMasterScale());
    if(!testTrack.empty())music.loadAndPlay(testTrack,true);
    else if(airxonix::CleanroomArchive::instance().exists("music/00.mus"))music.loadLegacyMus("zip://music/00.mus",true);
    // r239 DIRECT EXE 0x40F010/0x40EFF0: hscore.bin is a literal file in the
    // game working/root directory, not a platform-specific preference format.
    // Keeping the exact 1600-byte file also lets an original installation and
    // this native recompilation exchange records without conversion.
    const std::filesystem::path highScorePath=std::filesystem::path(paths.root)/std::string(airxonix::LegacyHighScoreTrace::fileName);
    if(!game.initializeLegacyHighScores(highScorePath))
        std::fprintf(stderr,"AX_HSCORE unable to load/create %s\n",highScorePath.string().c_str());
    else
        std::fprintf(stdout,"AX_HSCORE path=%s size=%zu\n",highScorePath.string().c_str(),airxonix::LegacyHighScoreTrace::fileSize);
    game.showMainMenuOnBoot();
    LegacyClock clock;
    std::fprintf(stderr,"AX_BOOT 08 game loop begin (main menu)\n");
    std::uint64_t frameCounter=0;
    while(!game.wantsQuit()){
        // r368 Android: transition-based soft keyboard; only during high-score
        // name entry. Game state remains the single source of truth.
        inputSystem.setRecordNameTextInput(game.recordNameEntryActive());
        // Menu screens need discrete events and delayed repeats from a held
        // virtual stick; gameplay retains continuous cardinal movement.
        const auto phase = game.phase();
        // ModeSelect has an original release-to-arm keyboard latch and needs
        // held key levels, unlike the repeating navigation in MainMenu.
        inputSystem.setMenuTouchNavigation(phase==GamePhase::MainMenu ||
            phase==GamePhase::Settings ||
            phase==GamePhase::Controls || phase==GamePhase::Records ||
            phase==GamePhase::Information);
        inputSystem.poll(input);
        {   // debug-only scripted input: AIRXONIX_INPUT="up@100-200,left@250-300"
            static const std::string script=[](){const char* e=std::getenv("AIRXONIX_INPUT");return std::string(e?e:"");}();
            std::size_t pos=0;
            while(pos<script.size()){
                std::size_t end=script.find(',',pos);if(end==std::string::npos)end=script.size();
                const std::string tok=script.substr(pos,end-pos);pos=end+1;
                const auto at=tok.find('@'),dash=tok.find('-');if(at==std::string::npos||dash==std::string::npos)continue;
                const std::string key=tok.substr(0,at);const unsigned long long a=std::stoull(tok.substr(at+1,dash-at-1)),b=std::stoull(tok.substr(dash+1));
                const bool on=frameCounter>=a&&frameCounter<=b;if(!on)continue;
                if(key=="up"){input.up=true;input.legacyHeld[0x26]=true;}
                else if(key=="down"){input.down=true;input.legacyHeld[0x28]=true;}
                else if(key=="left"){input.left=true;input.legacyHeld[0x25]=true;}
                else if(key=="right"){input.right=true;input.legacyHeld[0x27]=true;}
                else if(key=="enter"){input.action=true;input.legacyHeld[0x0D]=true;}
                else if(key=="esc"){input.back=true;input.legacyHeld[0x1B]=true;}
            }
        }const bool textureLabWasActive=renderer.textureLabActive();renderer.handleTextureLabInput(input);const int dtMs=clock.tick(game.timeScale());
        const int livesBeforeUpdate=game.lives();
        if(!textureLabWasActive && !renderer.textureLabActive())game.update(input,dtMs);
        // Also close the IME on the very frame A confirms / B cancels a name.
        inputSystem.setRecordNameTextInput(game.recordNameEntryActive());
        if(vibrationEnabled && game.lives()<livesBeforeUpdate)
            inputSystem.rumble(float(vibrationStrengthPercent)/100.0f,static_cast<std::uint32_t>(vibrationDurationMs));
        renderer.syncLevelLoad(game.levelLoadSerial());
        const auto fx=game.takeEffectEvents();
        auto smashVisuals=game.takePickupSmashEvents();
        for(const auto& e:smashVisuals)renderer.spawnPickupSmash(e);
        for(int i=0;i<fx.eraserSfx23Events;++i)music.playSfx(0x23);
        for(int i=0;i<fx.pickupSmashSfx0EEvents;++i)music.playSfx(0x0E);
        // r299: 0x40A5C0 receives the same scene camera that feeds 0x40C250.
        // This includes both startup substates and the finale camera; using the
        // gameplay fallback here produced incorrect spatial panning.
        const auto audioCam=(game.phase()==GamePhase::Settings || game.phase()==GamePhase::Controls)
            ? LegacyCamera::GameplayCameraState{0.f,0.f,0.f,0,0,0}
            : ((game.levelIntroActive() || game.levelEntryActive())
            ? LegacyCamera::GameplayCameraState{game.startupCameraX(),game.startupCameraY(),game.startupCameraZ(),game.startupCameraAngle1(),game.startupCameraAngle2(),game.startupCameraAngle3()}
            : (((game.phase()==GamePhase::Dying)||(game.phase()==GamePhase::GameOver))
                ? LegacyCamera::deathState(game.displayPlayerWorldX(),game.displayPlayerHeight(),game.displayPlayerWorldZ(),game.deathScene().triColorBurstStarted,game.displayCameraYawOffset())
                : (game.phase()==GamePhase::InterLevel
                    ? LegacyCamera::interLevelState(game.displayPlayerWorldX(),game.displayPlayerHeight(),game.displayPlayerWorldZ(),game.legacyCameraZoom(),game.displayCameraYawOffset())
                    : (game.phase()==GamePhase::FinalSequence
                        ? LegacyCamera::finaleState(game.displayPlayerWorldX(),game.finalePresentationY(),game.displayPlayerWorldZ(),game.legacyCameraZoom(),game.displayCameraYawOffset())
                        : LegacyCamera::gameplayState(game.displayPlayerWorldX(),game.displayPlayerWorldZ(),game.legacyCameraZoom(),game.displayCameraYawOffset())))));
        music.setSpatialListener(audioCam.x,audioCam.y,audioCam.z);
        music.setSpatialBasisAngles(0,game.legacyAudioBasisAngle2(),0);
        music.setSfxMasterScale(game.legacySfxMasterScale());
        music.setVolume(game.legacyMusicMasterScale());
        if(game.phase()==GamePhase::Gameplay || game.phase()==GamePhase::FinalSequence){
            const float x=game.displayPlayerWorldX();
            music.setLegacyMusicStereo(airxonix::LegacyMusicStereoTrace::channel0(x),airxonix::LegacyMusicStereoTrace::channel1(x));
        }else if(game.phase()==GamePhase::MainMenu){
            music.setLegacyMusicStereo(1.f,1.f);
        }
        static int deathVoiceInitial=0,deathVoiceRespawn=0,deathVoiceInterLevel=0,levelEntryVoice=0,settingsVoice=0,lowTimeWarningVoice=0;
        static std::array<int,6> pickupVoices{};
        for(const auto& e:game.takeDeathAudioEvents()){
            int* retained=nullptr;if(e.voice==DeathAudioVoiceTag::InitialDeathVoice)retained=&deathVoiceInitial;else if(e.voice==DeathAudioVoiceTag::RespawnVoice)retained=&deathVoiceRespawn;else if(e.voice==DeathAudioVoiceTag::InterLevelVoice)retained=&deathVoiceInterLevel;
            else if(e.voice==DeathAudioVoiceTag::LevelEntryVoice)retained=&levelEntryVoice;
            else if(e.voice==DeathAudioVoiceTag::SettingsVoice)retained=&settingsVoice;
            else if(e.voice==DeathAudioVoiceTag::LowTimeWarning)retained=&lowTimeWarningVoice;
            else if(e.voice>=DeathAudioVoiceTag::Pickup0 && e.voice<=DeathAudioVoiceTag::Pickup5)retained=&pickupVoices[std::size_t(e.voice)-std::size_t(DeathAudioVoiceTag::Pickup0)];
            switch(e.kind){
                case DeathAudioEventKind::SpatialPlay: music.playSpatialSfx(e.logicalId,e.x,e.y,e.z,e.scalar);break;
                case DeathAudioEventKind::SpatialStart: if(retained)*retained=music.playSpatialSfx(e.logicalId,e.x,e.y,e.z,e.scalar);break;
                case DeathAudioEventKind::SpatialUpdate: if(retained&&*retained)music.updateSpatialSfx(*retained,e.x,e.y,e.z,e.scalar);break;
                case DeathAudioEventKind::SpatialStop: if(retained&&*retained){music.stopSfx(*retained);*retained=0;}break;
                case DeathAudioEventKind::SimplePlay: music.playSfx(e.logicalId);break;
                case DeathAudioEventKind::MusicFadeOut: music.beginLegacyMusicFadeOut(e.fadePerMs);break;
                case DeathAudioEventKind::MusicRequest: music.requestLegacyMusicTrack(e.logicalId,e.fadePerMs);break;
            }
        }
        music.update(dtMs);renderer.setGameFrameDt(dtMs);renderer.resize(window);renderer.beginFrame();renderer.drawGame(game);
        {   // debug-only frame capture: AIRXONIX_SHOTS="60,300" AIRXONIX_SHOT_DIR=/tmp
            static std::vector<unsigned long long> shots=[](){std::vector<unsigned long long> v;const char* e=std::getenv("AIRXONIX_SHOTS");while(e&&*e){v.push_back(std::strtoull(e,const_cast<char**>(&e),10));if(*e==',')++e;else break;}return v;}();
            static int autoStart=[](){const char* e=std::getenv("AIRXONIX_AUTOSTART");return e?std::atoi(e):-1;}();
            if(autoStart>=0 && frameCounter==unsigned(autoStart)){game.debugStartSession();}
            static long long killAt=[](){const char* e=std::getenv("AIRXONIX_DEBUG_KILL");return e?std::atoll(e):-1ll;}();
            if(killAt>=0 && frameCounter==(unsigned long long)killAt)game.debugKill();
            if(std::getenv("AIRXONIX_DEBUG_TRACE")&&(game.phase()==GamePhase::Dying||frameCounter%20==0))
                std::fprintf(stderr,"AXTRACE f=%llu phase=%d x=%.6f y=%.6f z=%.6f\n",(unsigned long long)frameCounter,int(game.phase()),game.displayPlayerWorldX(),game.displayPlayerHeight(),game.displayPlayerWorldZ());
            for(auto f:shots)if(f==frameCounter+1){int w=0,h=0;SDL_GL_GetDrawableSize(window,&w,&h);std::vector<unsigned char> px(std::size_t(w)*h*4);axReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,px.data());
                const char* d=std::getenv("AIRXONIX_SHOT_DIR");std::string fn=std::string(d?d:"/tmp")+"/shot_"+std::to_string(f)+".ppm";FILE* fp=std::fopen(fn.c_str(),"wb");if(fp){std::fprintf(fp,"P6 %d %d 255\n",w,h);for(int y=h-1;y>=0;--y)for(int x=0;x<w;++x)std::fwrite(&px[(std::size_t(y)*w+x)*4],1,3,fp);std::fclose(fp);}}
            static unsigned long long quitAt=[](){const char* e=std::getenv("AIRXONIX_QUIT_AT");return e?std::strtoull(e,nullptr,10):0ull;}();
            if(quitAt&&frameCounter>=quitAt)break;
        }
        renderer.endFrame(window);
        ++frameCounter;if(frameCounter==1)std::fprintf(stderr,"AX_BOOT 09 first frame presented\n");
        else if((frameCounter%600u)==0u)std::fprintf(stderr,"AX_FRAME count=%llu\n",static_cast<unsigned long long>(frameCounter));
    }
    // r304 DIRECT EXE 0x424DD6 -> 0x4230A0: flush the exact 36-byte
    // gameinf.bin block again when the outer M1 loop returns.
    if(!game.saveLegacySettingsNow())
        std::fprintf(stderr,"AX_SETTINGS final save failed: %s\n",settingsPath.string().c_str());
    music.stop();SDL_GL_DeleteContext(gl);SDL_DestroyWindow(window);SDL_Quit();return 0;
}
