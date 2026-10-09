#pragma once
#include <SDL.h>
#include <cstdint>
#include <string>
#include <vector>
#include "legacy_sfx_bank.hpp"
#include "legacy_music_state.hpp"

class MusicPlayer {
public:
    MusicPlayer();
    ~MusicPlayer();
    bool loadAndPlay(const std::string& path, bool loop = true);
    bool loadLegacyMus(const std::string& path, bool loop = true);
    bool loadLegacySfxBank(const std::string& path);
    bool loadLegacySfxBank(const std::string& path,const std::vector<std::uint8_t>& wavePackDirectory);
    bool loadLegacySfxBankBytes(std::vector<std::uint8_t> bytes,const std::vector<std::uint8_t>& wavePackDirectory);
    int playSfx(std::size_t logicalId, int gainL=63, int gainR=63);
    int playSpatialSfx(std::size_t logicalId,float x,float y,float z,float scalar=1.f);
    bool updateSpatialSfx(int handle,float x,float y,float z,float scalar=1.f);
    void setSpatialListener(float x,float y,float z);
    void setSpatialBasisAngles(int ax,int ay,int az);
    void setSfxMasterScale(float scale);
    void stopSfx(int handle);
    void setLegacyMusicSearchPaths(std::vector<std::string> paths);
    void beginLegacyMusicFadeOut(float perMs);
    void requestLegacyMusicTrack(std::size_t trackId, float fadeInPerMs);
    void setLegacyMusicStereo(float channel0, float channel1);
    void update(int dtMs);
    void stop();
    void setVolume(float volume01);
    bool playing() const { return device_ != 0; }
private:
    SDL_AudioDeviceID device_ = 0;
    SDL_AudioSpec spec_{};
    std::vector<std::uint8_t> pcm_;
    std::size_t musicCursor_ = 0;
    double musicFramePos_ = 0.0;
    bool loop_ = true;
    float volume_ = 1.0f;
    float musicStereo0_ = 1.0f;
    float musicStereo1_ = 1.0f;
    std::vector<std::string> legacyMusicSearchPaths_;
    airxonix::LegacyMusicTransitionState musicTransition_;
    airxonix::LegacySfxBank sfxBank_;
    airxonix::NativeLegacySfxMixer sfxMixer_;
    bool openLegacyDevice();
    static void audioCallback(void* userdata, Uint8* stream, int len);
    void mixCallback(Uint8* stream, int len);
    std::string resolveLegacyTrack(std::size_t trackId) const;
};
