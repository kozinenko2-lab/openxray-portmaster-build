#include "audio/legacy_sfx_bank.hpp"
#include "audio/legacy_audio_trace.hpp"
#include <cassert>
#include <cstdint>
#include <fstream>
#include <vector>
using namespace airxonix;
int main(){
    static_assert(LegacyWavePackDirectoryTrace::resourceBytes==416u);
    static_assert(LegacyWavePackDirectoryTrace::recordCount==52u);
    static_assert(LegacyWavePackDirectoryTrace::realSampleCount==50u);
    std::size_t total=0; for(const auto& e:LegacyWavePackDirectoryTrace::entries) total+=e.rawLength;
    assert(total==891221u);
    std::vector<std::uint8_t> fake(total+2048u,0x80u);
    const char* p="test_29.mus"; {std::ofstream f(p,std::ios::binary); f.write((char*)fake.data(),fake.size());}
    LegacySfxBank bank; assert(bank.load(p));
    auto bon0=bank.resolveLogicalId(0x0A); assert(bon0.valid && bon0.rawLength==12003u && bon0.playbackLength==12003u);
    auto levl=bank.findFourcc("levl"); assert(levl.valid && levl.rawLength==53263u && levl.playbackLength==51263u);
    auto game=bank.findFourcc("game"); assert(game.valid && game.playbackLength==51235u);
    auto whip=bank.resolveLogicalId(0x31); assert(whip.valid && whip.rawLength==12512u);
    auto empty=bank.resolveLogicalId(0x35); assert(!empty.valid);
    NativeLegacySfxMixer mix(&bank); int handles[7]{}; for(int i=0;i<7;++i){handles[i]=mix.playLogicalId(0x0A); assert(handles[i]==i+1);}
    assert(mix.playLogicalId(0x0A)==0); // safe reject instead of legacy OOB eighth slot
    std::uint8_t out[16]{}; mix.mixStereoU8(out,8);
    mix.stop(handles[0]); assert(mix.playLogicalId(0x0B)==1);
    std::remove(p);
}
