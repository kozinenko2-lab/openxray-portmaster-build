#include "music.hpp"
#include "legacy_music_mix.hpp"
#include "legacy_audio_trace.hpp"
#include "core/cleanroom_archive.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <cctype>
#include <cmath>

using airxonix::LegacyDirectSoundFormatTrace;
using airxonix::LegacyUnifiedMixTrace;

MusicPlayer::MusicPlayer():sfxMixer_(&sfxBank_){musicTransition_.reset(false,1.0f);}
MusicPlayer::~MusicPlayer() { stop(); }

bool MusicPlayer::openLegacyDevice() {
    if(device_) return true;
    // The source contract remains original 22050 Hz unsigned-8 stereo, but
    // handheld ALSA/Mali stacks often resample that format very poorly.
    // Ask SDL for a clean 44.1 kHz signed-16 output and resample 2x ourselves.
    SDL_AudioSpec want{};
    want.freq=44100;want.format=AUDIO_S16SYS;want.channels=2;want.samples=1024;
    want.callback=&MusicPlayer::audioCallback;want.userdata=this;
    device_=SDL_OpenAudioDevice(nullptr,0,&want,&spec_,0);
    if(!device_){
        std::fprintf(stderr,"Audio: 44.1k/S16 open failed (%s), retrying legacy 22.05k/U8\n",SDL_GetError());
        SDL_zero(want);want.freq=static_cast<int>(LegacyDirectSoundFormatTrace::samplesPerSecond);want.format=AUDIO_U8;want.channels=2;want.samples=1024;want.callback=&MusicPlayer::audioCallback;want.userdata=this;
        device_=SDL_OpenAudioDevice(nullptr,0,&want,&spec_,0);
    }
    if (!device_) std::fprintf(stderr,"Audio: SDL_OpenAudioDevice: %s\n",SDL_GetError());
    else {std::fprintf(stderr,"AX_AUDIO output=%dHz format=0x%x channels=%u\n",spec_.freq,unsigned(spec_.format),unsigned(spec_.channels));SDL_PauseAudioDevice(device_,0);}
    return device_ != 0;
}

bool MusicPlayer::loadLegacyMus(const std::string& path, bool loop) {
    std::vector<std::uint8_t> data;
    if(path.rfind("zip://",0)==0){
        airxonix::CleanroomArchive::instance().read(path.substr(6),data);
    }else{
        std::ifstream f(path, std::ios::binary);
        if(f)data.assign(std::istreambuf_iterator<char>(f),{});
    }
    if (data.empty()) { std::fprintf(stderr,"Music: cannot open/empty legacy MUS %s\n",path.c_str()); return false; }
    if(!openLegacyDevice()) return false;
    SDL_LockAudioDevice(device_);
    pcm_=std::move(data); musicCursor_=0; musicFramePos_=0.0; loop_=loop;
    SDL_UnlockAudioDevice(device_);
    musicTransition_.markDirectStreamOpen(true);
    return true;
}

bool MusicPlayer::loadLegacySfxBank(const std::string& path){
    airxonix::LegacySfxBank bank;
    if(!bank.load(path)){std::fprintf(stderr,"SFX: cannot open legacy bank %s\n",path.c_str());return false;}
    if(!openLegacyDevice()) return false;
    SDL_LockAudioDevice(device_);
    sfxMixer_.stopAll();
    sfxBank_=std::move(bank);
    sfxMixer_.setBank(&sfxBank_);
    SDL_UnlockAudioDevice(device_);
    return true;
}

bool MusicPlayer::loadLegacySfxBank(const std::string& path,const std::vector<std::uint8_t>& wavePackDirectory){
    airxonix::LegacySfxBank bank;
    if(!bank.load(path,wavePackDirectory)){std::fprintf(stderr,"SFX: cannot open legacy bank/directory %s\n",path.c_str());return false;}
    if(!openLegacyDevice()) return false;
    SDL_LockAudioDevice(device_);sfxMixer_.stopAll();sfxBank_=std::move(bank);sfxMixer_.setBank(&sfxBank_);SDL_UnlockAudioDevice(device_);return true;
}

bool MusicPlayer::loadLegacySfxBankBytes(std::vector<std::uint8_t> bytes,const std::vector<std::uint8_t>& wavePackDirectory){
    airxonix::LegacySfxBank bank;
    if(!bank.loadBytes(std::move(bytes),wavePackDirectory)){std::fprintf(stderr,"SFX: invalid clean-room ZIP bank/directory\n");return false;}
    if(!openLegacyDevice())return false;
    SDL_LockAudioDevice(device_);sfxMixer_.stopAll();sfxBank_=std::move(bank);sfxMixer_.setBank(&sfxBank_);SDL_UnlockAudioDevice(device_);return true;
}

int MusicPlayer::playSfx(std::size_t logicalId,int gainL,int gainR){
    if(!device_ && !openLegacyDevice()) return 0;
    SDL_LockAudioDevice(device_);
    const int h=sfxMixer_.playLogicalId(logicalId,gainL,gainR);
    SDL_UnlockAudioDevice(device_);
    return h;
}
void MusicPlayer::stopSfx(int handle){if(!device_)return;SDL_LockAudioDevice(device_);sfxMixer_.stop(handle);SDL_UnlockAudioDevice(device_);}
int MusicPlayer::playSpatialSfx(std::size_t logicalId,float x,float y,float z,float scalar){if(!device_&&!openLegacyDevice())return 0;SDL_LockAudioDevice(device_);const int h=sfxMixer_.playSpatialLogicalId(logicalId,x,y,z,scalar);SDL_UnlockAudioDevice(device_);return h;}
bool MusicPlayer::updateSpatialSfx(int handle,float x,float y,float z,float scalar){if(!device_)return false;SDL_LockAudioDevice(device_);const bool ok=sfxMixer_.updateSpatial(handle,x,y,z,scalar);SDL_UnlockAudioDevice(device_);return ok;}
void MusicPlayer::setSpatialListener(float x,float y,float z){
    // Listener state is pure mixer state and must be retained even before the
    // first spatial voice lazily opens the SDL device.
    if(!device_){sfxMixer_.setSpatialListener(x,y,z);return;}
    SDL_LockAudioDevice(device_);sfxMixer_.setSpatialListener(x,y,z);SDL_UnlockAudioDevice(device_);
}
void MusicPlayer::setSpatialBasisAngles(int ax,int ay,int az){
    const auto basis=airxonix::legacyBuildAudioBasis(ax,ay,az);
    if(!device_){sfxMixer_.setSpatialBasis(basis);return;}
    SDL_LockAudioDevice(device_);sfxMixer_.setSpatialBasis(basis);SDL_UnlockAudioDevice(device_);
}
void MusicPlayer::setSfxMasterScale(float scale){if(!device_){sfxMixer_.setMasterScale(scale);return;}SDL_LockAudioDevice(device_);sfxMixer_.setMasterScale(scale);SDL_UnlockAudioDevice(device_);}

bool MusicPlayer::loadAndPlay(const std::string& path, bool loop) {
    const auto dot=path.find_last_of('.');
    if(dot!=std::string::npos) {
        std::string ext=path.substr(dot);
        std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
        if(ext==".mus") return loadLegacyMus(path,loop);
    }
    SDL_AudioSpec wav{}; Uint8* data=nullptr; Uint32 len=0;
    if (!SDL_LoadWAV(path.c_str(), &wav, &data, &len)) {std::fprintf(stderr,"Music: SDL_LoadWAV(%s): %s\n",path.c_str(),SDL_GetError());return false;}
    const bool native=wav.freq==static_cast<int>(LegacyDirectSoundFormatTrace::samplesPerSecond) && wav.format==AUDIO_U8 && wav.channels==2;
    if(!native){SDL_FreeWAV(data);std::fprintf(stderr,"Music: WAV must already match legacy 22050/U8/stereo contract\n");return false;}
    if(!openLegacyDevice()){SDL_FreeWAV(data);return false;}
    SDL_LockAudioDevice(device_);pcm_.assign(data,data+len);musicCursor_=0;musicFramePos_=0.0;loop_=loop;SDL_UnlockAudioDevice(device_);SDL_FreeWAV(data);musicTransition_.markDirectStreamOpen(true);return true;
}

void MusicPlayer::audioCallback(void* userdata,Uint8* stream,int len){static_cast<MusicPlayer*>(userdata)->mixCallback(stream,len);}
void MusicPlayer::mixCallback(Uint8* stream,int len){
    if(!stream||len<=0)return;
    const auto mixLegacyFrames=[&](std::uint8_t* dst,std::size_t frames){
        std::vector<std::int32_t> accum(frames*2u,0);
        sfxMixer_.mixStereoCentered(accum.data(),frames);
        const int gainL=LegacyUnifiedMixTrace::musicGain(volume_,musicTransition_.currentGain(),musicStereo0_);
        const int gainR=LegacyUnifiedMixTrace::musicGain(volume_,musicTransition_.currentGain(),musicStereo1_);
        for(std::size_t f=0;f<frames;++f){
            if(!pcm_.empty()){
                if(musicCursor_+1>=pcm_.size()){if(loop_)musicCursor_=0;else pcm_.clear();}
                if(!pcm_.empty()){
                    accum[f*2]+=LegacyUnifiedMixTrace::contribution(gainL,pcm_[musicCursor_]);
                    accum[f*2+1]+=LegacyUnifiedMixTrace::contribution(gainR,pcm_[musicCursor_+1]);
                    musicCursor_+=2;
                }
            }
            dst[f*2]=LegacyUnifiedMixTrace::finalize(static_cast<int>(accum[f*2]));
            dst[f*2+1]=LegacyUnifiedMixTrace::finalize(static_cast<int>(accum[f*2+1]));
        }
    };
    if(spec_.format==AUDIO_U8 && spec_.freq==22050){
        const std::size_t frames=static_cast<std::size_t>(len)/2u;
        mixLegacyFrames(stream,frames);
        return;
    }
    // Firmware fallback: construct the exact legacy 22.05-kHz combined stream
    // first (SFX + music + one final saturation), then duplicate each frame to
    // 44.1 kHz. This preserves the original mix/clipping semantics and pitch.
    std::memset(stream,0,static_cast<std::size_t>(len));
    auto* out=reinterpret_cast<Sint16*>(stream);
    const std::size_t outFrames=static_cast<std::size_t>(len)/(sizeof(Sint16)*2u);
    const std::size_t srcFrames=(outFrames+1u)/2u;
    std::vector<std::uint8_t> legacy(srcFrames*2u,128u);
    mixLegacyFrames(legacy.data(),srcFrames);
    for(std::size_t f=0;f<outFrames;++f){
        const std::size_t si=std::min<std::size_t>(f/2u,srcFrames-1u);
        out[f*2]=Sint16((int(legacy[si*2])-128)*256);
        out[f*2+1]=Sint16((int(legacy[si*2+1])-128)*256);
    }
}

void MusicPlayer::setLegacyMusicSearchPaths(std::vector<std::string> paths){legacyMusicSearchPaths_=std::move(paths);}
void MusicPlayer::beginLegacyMusicFadeOut(float perMs){musicTransition_.beginFadeOut(perMs);}
void MusicPlayer::requestLegacyMusicTrack(std::size_t trackId,float fadeInPerMs){musicTransition_.requestTrack(trackId,fadeInPerMs);}
void MusicPlayer::setLegacyMusicStereo(float c0,float c1){musicStereo0_=c0;musicStereo1_=c1;}
std::string MusicPlayer::resolveLegacyTrack(std::size_t trackId) const{
    char lower[16],upper[16];std::snprintf(lower,sizeof(lower),"%02zu.mus",trackId);std::snprintf(upper,sizeof(upper),"%02zu.MUS",trackId);
    for(const auto& dir:legacyMusicSearchPaths_){
        for(const char* name:{lower,upper}){const std::string p=dir.empty()?std::string(name):(dir+"/"+name);std::ifstream f(p,std::ios::binary);if(f.good())return p;}
    }
    for(const char* name:{lower,upper}){
        const std::string entry=std::string("music/")+name;
        if(airxonix::CleanroomArchive::instance().exists(entry))return std::string("zip://")+entry;
    }
    return {};
}
void MusicPlayer::update(int dtMs){
    const auto sw=musicTransition_.update(dtMs,true);
    if(!sw.pending)return;
    const std::string path=resolveLegacyTrack(sw.trackId);
    bool opened=false;
    if(!path.empty()){
        // 0x40B0D0 closes the previous mapping immediately before opening the
        // pending track. Keep the SDL device/SFX mixer alive; replace music PCM only.
        std::vector<std::uint8_t> data;
        if(path.rfind("zip://",0)==0)airxonix::CleanroomArchive::instance().read(path.substr(6),data);
        else {std::ifstream f(path,std::ios::binary);if(f)data.assign(std::istreambuf_iterator<char>(f),{});}
        opened=!data.empty();
        if(opened){if(!device_)opened=openLegacyDevice();if(opened){SDL_LockAudioDevice(device_);pcm_=std::move(data);musicCursor_=0;musicFramePos_=0.0;loop_=true;SDL_UnlockAudioDevice(device_);}}
    }
    musicTransition_.completeSwitch(opened);
    if(!opened)std::fprintf(stderr,"Music: pending legacy track %zu could not be opened\n",sw.trackId);
}
void MusicPlayer::stop(){if(device_){SDL_LockAudioDevice(device_);pcm_.clear();musicCursor_=0;musicFramePos_=0.0;sfxMixer_.stopAll();SDL_UnlockAudioDevice(device_);SDL_CloseAudioDevice(device_);device_=0;}else pcm_.clear();musicTransition_.reset(false,1.0f);}
void MusicPlayer::setVolume(float v){volume_=std::clamp(v,0.0f,1.0f);}
