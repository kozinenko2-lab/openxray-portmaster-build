#include "legacy_sfx_bank.hpp"
#include "legacy_audio_trace.hpp"
#include <algorithm>
#include <fstream>
#include <iterator>

namespace airxonix {

bool LegacySfxBank::load(const std::string& path) {
    clear();
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    bytes_.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return !bytes_.empty();
}

bool LegacySfxBank::load(const std::string& path,const std::vector<std::uint8_t>& wavePackDirectory){
    if(!load(path)) return false;
    if(wavePackDirectory.size()%8u!=0u) return false;
    for(std::size_t p=0;p+8<=wavePackDirectory.size();p+=8){
        const auto* r=wavePackDirectory.data()+p;
        if((r[0]==0&&r[1]==0&&r[2]==0&&r[3]==0)||(r[0]==0xff&&r[1]==0xff&&r[2]==0xff&&r[3]==0xff)) break;
        std::string fourcc;fourcc.push_back(char(r[3]));fourcc.push_back(char(r[2]));fourcc.push_back(char(r[1]));fourcc.push_back(char(r[0]));
        const std::uint32_t len=std::uint32_t(r[4])|(std::uint32_t(r[5])<<8)|(std::uint32_t(r[6])<<16)|(std::uint32_t(r[7])<<24);
        directory_.push_back({fourcc,len});
    }
    return !directory_.empty();
}

bool LegacySfxBank::loadBytes(std::vector<std::uint8_t> bytes){
    clear();bytes_=std::move(bytes);return !bytes_.empty();
}

bool LegacySfxBank::loadBytes(std::vector<std::uint8_t> bytes,const std::vector<std::uint8_t>& wavePackDirectory){
    if(!loadBytes(std::move(bytes)))return false;
    if(wavePackDirectory.size()%8u!=0u)return false;
    for(std::size_t p=0;p+8<=wavePackDirectory.size();p+=8){
        const auto* r=wavePackDirectory.data()+p;
        if((r[0]==0&&r[1]==0&&r[2]==0&&r[3]==0)||(r[0]==0xff&&r[1]==0xff&&r[2]==0xff&&r[3]==0xff))break;
        std::string fourcc;fourcc.push_back(char(r[3]));fourcc.push_back(char(r[2]));fourcc.push_back(char(r[1]));fourcc.push_back(char(r[0]));
        const std::uint32_t len=std::uint32_t(r[4])|(std::uint32_t(r[5])<<8)|(std::uint32_t(r[6])<<16)|(std::uint32_t(r[7])<<24);
        directory_.push_back({fourcc,len});
    }
    return !directory_.empty();
}

void LegacySfxBank::clear() { bytes_.clear(); directory_.clear(); }

LegacySfxSampleView LegacySfxBank::findFourcc(std::string_view fourcc) const {
    std::size_t offset=0;
    if(!directory_.empty()){
        for(const auto& e:directory_){
            if(e.fourcc==fourcc){std::size_t playback=e.rawLength;if((fourcc=="levl"||fourcc=="game")&&playback>=LegacyWavePackTrace::shortenedSamples)playback-=LegacyWavePackTrace::shortenedSamples;const bool ok=!bytes_.empty()&&offset+e.rawLength<=bytes_.size();return {offset,e.rawLength,playback,ok};}
            offset+=e.rawLength;
        }
        return {};
    }
    for (const auto& e : LegacyWavePackDirectoryTrace::entries) {
        if (e.fourcc == fourcc) {
            std::size_t playback=e.rawLength;
            if ((fourcc=="levl" || fourcc=="game") && playback>=LegacyWavePackTrace::shortenedSamples) playback-=LegacyWavePackTrace::shortenedSamples;
            const bool ok=!bytes_.empty() && offset+e.rawLength<=bytes_.size();
            return {offset,e.rawLength,playback,ok};
        }
        offset += e.rawLength;
    }
    return {};
}

LegacySfxSampleView LegacySfxBank::resolveLogicalId(std::size_t logicalId) const {
    if (logicalId>=LegacySfxTrace::logicalSlots) return {};
    const auto key=LegacySfxTrace::fourcc[logicalId];
    if (key.empty()) return {};
    return findFourcc(key);
}

const std::uint8_t* LegacySfxBank::dataAt(std::size_t offset) const {
    return offset<bytes_.size() ? bytes_.data()+offset : nullptr;
}

} // namespace airxonix

namespace airxonix {
int NativeLegacySfxMixer::playLogicalId(std::size_t logicalId,int gainL,int gainR){
    if(!bank_ || !bank_->loaded()) return 0;
    gainL=static_cast<int>(float(gainL)*masterScale_);
    gainR=static_cast<int>(float(gainR)*masterScale_);
    const auto s=bank_->resolveLogicalId(logicalId); if(!s.valid) return 0;
    for(std::size_t i=0;i<voices_.size();++i) if(!voices_[i].active){
        voices_[i]={s,0,gainL,gainR,true}; return static_cast<int>(i+1);
    }
    return 0;
}
void NativeLegacySfxMixer::stop(int handle){ if(handle>=1 && handle<=static_cast<int>(voices_.size())) voices_[handle-1]=Voice{}; }
void NativeLegacySfxMixer::stopAll(){ for(auto& v:voices_) v=Voice{}; }
void NativeLegacySfxMixer::mixStereoCentered(std::int32_t* dst,std::size_t frames){
    if(!dst) return;
    for(std::size_t f=0;f<frames;++f){
        for(auto& v:voices_) if(v.active){
            if(v.cursor>=v.sample.playbackLength){v=Voice{}; continue;}
            const auto* p=bank_->dataAt(v.sample.offset+v.cursor);
            if(!p){v=Voice{}; continue;}
            const int sample=static_cast<int>(*p); ++v.cursor;
            const int gl=std::clamp(v.gainL,0,static_cast<int>(LegacyGainTableTrace::gainRows-1));
            const int gr=std::clamp(v.gainR,0,static_cast<int>(LegacyGainTableTrace::gainRows-1));
            dst[f*2] += LegacyGainTableTrace::contribution(gl,sample);
            dst[f*2+1] += LegacyGainTableTrace::contribution(gr,sample);
        }
    }
}
void NativeLegacySfxMixer::mixStereoU8(std::uint8_t* dst,std::size_t frames){
    if(!dst) return;
    std::vector<std::int32_t> accum(frames*2u,0);
    mixStereoCentered(accum.data(),frames);
    for(std::size_t i=0;i<accum.size();++i){
        const int v=std::max(-128,std::min(127,static_cast<int>(accum[i])));
        dst[i]=static_cast<std::uint8_t>(v+128);
    }
}

} // namespace airxonix

namespace airxonix {
int NativeLegacySfxMixer::playSpatialLogicalId(std::size_t logicalId,float x,float y,float z,float scalar){
    const auto g=legacySpatialGains(listener_,x,y,z,scalar);
    // playLogicalId applies the current master scale, mirroring 0x43FC68.
    return playLogicalId(logicalId,g.left,g.right);
}
bool NativeLegacySfxMixer::updateSpatial(int handle,float x,float y,float z,float scalar){
    if(handle<1||handle>static_cast<int>(voices_.size())||!voices_[handle-1].active)return false;
    const auto g=legacySpatialGains(listener_,x,y,z,scalar);
    voices_[handle-1].gainL=static_cast<int>(float(g.left)*masterScale_);
    voices_[handle-1].gainR=static_cast<int>(float(g.right)*masterScale_);return true;
}
} // namespace airxonix
