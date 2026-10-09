#include "core/legacy_highscore.hpp"

#include <algorithm>
#include <fstream>

namespace airxonix {

LegacyHighScoreFile makeLegacyDefaultHighScores() {
    LegacyHighScoreFile result{};
    for (auto& block : result.blocks) {
        for (auto& name : block.names) {
            name.fill('.');
        }
        block.values.fill(0u);
    }
    return result;
}

bool loadLegacyHighScores(const std::filesystem::path& path, LegacyHighScoreFile& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    in.read(reinterpret_cast<char*>(&out), static_cast<std::streamsize>(sizeof(out)));
    return in.good() || (in.eof() && in.gcount() == static_cast<std::streamsize>(sizeof(out)));
}

bool saveLegacyHighScores(const std::filesystem::path& path, const LegacyHighScoreFile& value) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(&value), static_cast<std::streamsize>(sizeof(value)));
    return out.good();
}

int insertLegacyHighScoreCandidate(LegacyHighScoreBlock& block,std::uint32_t score) {
    // 0x40F19E: qualification compares only against row 9 and uses JBE.
    if(score<=block.values[9])return -1;

    // 0x40F1BB..0x40F1D4 walks rows 8..0 backwards while score is strictly
    // greater. JBE stops on an equal score, therefore insertion is after it.
    int probe=8;
    while(probe>=0 && score>block.values[static_cast<std::size_t>(probe)])--probe;
    const int insertAt=probe+1;

    // 0x40F1DA..0x40F213 shifts both 32-bit values and complete 16-byte names.
    for(int row=9;row>insertAt;--row){
        block.values[static_cast<std::size_t>(row)]=block.values[static_cast<std::size_t>(row-1)];
        block.names[static_cast<std::size_t>(row)]=block.names[static_cast<std::size_t>(row-1)];
    }
    block.values[static_cast<std::size_t>(insertAt)]=score;
    // 0x40F21C..0x40F252 copies 16 literal '.' bytes from 0x440DDC.
    block.names[static_cast<std::size_t>(insertAt)].fill('.');
    return insertAt;
}


LegacyHighScoreNameEditResult applyLegacyHighScoreNameInput(
    std::array<char,16>& name,int& typedLength,int code) {
    LegacyHighScoreNameEditResult result{};

    // The stack local at [esp+0x14] is independently maintained by the EXE.
    // Clamp only defensively; valid game state is always 0..16.
    typedLength=std::clamp(typedLength,0,16);

    const bool acceptedByte=
        (code>=0x21 && code<=0x3F) ||
        (code>=0x41 && code<=0x5A) ||
        (code>=0x61 && code<=0x7A) ||
        code==0x20 ||
        (code>=0xC0 && code<=0xFF);

    if(acceptedByte){
        // 0x40F55F..0x40F57F: input at full 16 bytes is ignored entirely.
        if(typedLength>=16)return result;
        name[static_cast<std::size_t>(typedLength++)]=static_cast<char>(code&0xFF);
        result.action=LegacyHighScoreNameAction::Edited;
        result.sfx=0x15;
        return result;
    }

    if(code==0x08){
        // 0x40F58C..0x40F5AF: erase one byte back to the literal filler '.'.
        if(typedLength<=0)return result;
        --typedLength;
        name[static_cast<std::size_t>(typedLength)]='.';
        result.action=LegacyHighScoreNameAction::Edited;
        result.sfx=0x15;
        return result;
    }

    if(code==0x1B){
        // 0x40F5B4..0x40F5DD: Escape only cancels while the typed length is 0.
        if(typedLength!=0)return result;
        result.action=LegacyHighScoreNameAction::Cancelled;
        result.sfx=0x16;
        return result;
    }

    if(code==0x0D){
        // 0x40F5EB..0x40F60B: trim trailing spaces and dots to dots. A record
        // containing only those characters is considered empty and stays in
        // the editor with typedLength reset to zero.
        int last=15;
        while(last>=0 && (name[static_cast<std::size_t>(last)]==' ' ||
                          name[static_cast<std::size_t>(last)]=='.')){
            name[static_cast<std::size_t>(last)]='.';
            --last;
        }
        if(last<0){
            typedLength=0;
            return result;
        }
        result.action=LegacyHighScoreNameAction::Confirmed;
        result.sfx=0x16;
        return result;
    }

    // 0x40F535..0x40F552: every other non-zero legacy code is represented by
    // '~' (0x7E), rather than being ignored. This oddity is intentional.
    if(code!=0 && typedLength<16){
        name[static_cast<std::size_t>(typedLength++)]='~';
        result.action=LegacyHighScoreNameAction::Edited;
        result.sfx=0x15;
    }
    return result;
}

char cycleLegacyHighScoreDpadChar(char current,int direction) {
    // Russian uppercase CP1251 alphabet. Ё (0xA8) is not contiguous with
    // А..Я (0xC0..0xDF), so keep the user-visible alphabet order explicitly.
    static constexpr std::array<unsigned char,34> kSymbols{{
        0x2E, // '.' = untouched/empty cell
        0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xA8,0xC6,0xC7,0xC8,0xC9,
        0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,
        0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF
    }};
    const auto byte=static_cast<unsigned char>(current);
    std::size_t at=0;
    for(std::size_t i=0;i<kSymbols.size();++i){
        if(kSymbols[i]==byte){at=i;break;}
    }
    if(direction>0)at=(at+1u)%kSymbols.size();
    else if(direction<0)at=(at+kSymbols.size()-1u)%kSymbols.size();
    return static_cast<char>(kSymbols[at]);
}

int legacyHighScoreVisibleNameLength(const std::array<char,16>& name) {
    int last=-1;
    for(int i=0;i<16;++i){
        if(name[static_cast<std::size_t>(i)]!='.')last=i;
    }
    return last+1;
}

} // namespace airxonix
