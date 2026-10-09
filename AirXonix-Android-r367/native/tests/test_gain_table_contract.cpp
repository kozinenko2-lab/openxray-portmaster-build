#include "audio/legacy_audio_trace.hpp"
#include "audio/legacy_sfx_bank.hpp"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <vector>
using namespace airxonix;
int main(){
    static_assert(LegacyGainTableTrace::gainRows==96);
    assert(LegacyGainTableTrace::tableEnd-LegacyGainTableTrace::tableBase==96u*256u*4u);
    assert(LegacyGainTableTrace::contribution(63,255)==125);
    assert(LegacyGainTableTrace::contribution(63,0)==-126);
    assert(LegacyGainTableTrace::contribution(85,255)==168);
    assert(LegacyGainTableTrace::contribution(1,127)==0); // truncation toward zero
    // Verify native mixer uses the exact /64 table rule rather than old /63 scaling.
    std::size_t n=0; for(const auto&e:LegacyWavePackDirectoryTrace::entries)n+=e.rawLength;
    std::vector<unsigned char> fake(n,255); const char* path="test_gain_29.mus";
    {std::ofstream f(path,std::ios::binary);f.write((char*)fake.data(),fake.size());}
    LegacySfxBank bank;assert(bank.load(path));NativeLegacySfxMixer mix(&bank);assert(mix.playLogicalId(0x0A,63,63)==1);
    unsigned char out[2]{128,128};mix.mixStereoU8(out,1);assert(out[0]==253&&out[1]==253);
    std::remove(path);
}
