#include "game/game.hpp"
#include "core/legacy_highscore.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

struct GameTestProbe {
    static void setBlock(Game& g,const airxonix::LegacyHighScoreBlock& b){g.highScores_.blocks[0]=b;g.mode_=0;}
    static void enterPost(Game& g,std::uint32_t score){g.enterPostGameRecords(score);}
};

static airxonix::LegacyHighScoreBlock ranked(){
    auto b=airxonix::makeLegacyDefaultHighScores().blocks[0];
    for(std::size_t i=0;i<10;++i)b.values[i]=1000u-static_cast<std::uint32_t>(i)*100u;
    return b;
}

static void release(Game& g){ InputState in{}; g.update(in,0); }
static void press(Game& g,void (*set)(InputState&)){
    InputState in{}; set(in); g.update(in,0);
}
static void controllerPress(Game& g,void (*set)(InputState&),int legacyCode){
    InputState in{}; set(in); in.legacyPressedCode=legacyCode;
    in.legacyPressedFromController=true; g.update(in,0);
}
static void up(InputState& i){i.up=true;}
static void down(InputState& i){i.down=true;}
static void left(InputState& i){i.left=true;}
static void right(InputState& i){i.right=true;}
static void action(InputState& i){i.action=true;}

int main(){
    using namespace airxonix;

    // Unit-level symbol order: filler -> А -> Б ... -> Я -> filler.
    assert(static_cast<unsigned char>(cycleLegacyHighScoreDpadChar('.',+1))==0xC0);
    assert(static_cast<unsigned char>(cycleLegacyHighScoreDpadChar(static_cast<char>(0xC0),+1))==0xC1);
    assert(static_cast<unsigned char>(cycleLegacyHighScoreDpadChar(static_cast<char>(0xC5),+1))==0xA8); // Е -> Ё
    assert(static_cast<unsigned char>(cycleLegacyHighScoreDpadChar(static_cast<char>(0xA8),+1))==0xC6); // Ё -> Ж
    assert(cycleLegacyHighScoreDpadChar(static_cast<char>(0xDF),+1)=='.');
    assert(static_cast<unsigned char>(cycleLegacyHighScoreDpadChar('.',-1))==0xDF);

    Game g; GameTestProbe::setBlock(g,ranked()); GameTestProbe::enterPost(g,950u);
    // Fade to input-ready; empty input also releases the inherited menu latch.
    InputState none{}; g.update(none,700);
    assert(g.records().readyForInput && !g.records().inputLatched);
    assert(g.records().candidateRow==1 && g.records().nameEntry);

    // Up from '.' selects А at cell 0. Holding the same press must not repeat.
    // Mirror the actual SDL controller event: D-Pad Up also carries 0x103.
    controllerPress(g,up,0x103);
    auto& name0=g.records().workingBlock.names[1];
    assert(static_cast<unsigned char>(name0[0])==0xC0);
    assert(g.records().typedNameLength==1);
    assert(g.records().dpadNameEditing && g.records().nameCursor==0);
    press(g,up);
    assert(static_cast<unsigned char>(name0[0])==0xC0);

    // Release + Up selects Б. Right chooses cell 1; Down from '.' wraps to Я.
    release(g); press(g,up);
    assert(static_cast<unsigned char>(name0[0])==0xC1);
    release(g); press(g,right);
    assert(g.records().nameCursor==1);
    release(g); press(g,down);
    assert(static_cast<unsigned char>(name0[1])==0xDF);
    assert(g.records().typedNameLength==2);

    // Up from Я wraps to '.', then Up selects А again.
    release(g); press(g,up);
    assert(name0[1]=='.' && g.records().typedNameLength==1);
    release(g); press(g,up);
    assert(static_cast<unsigned char>(name0[1])==0xC0);

    // Cursor is bounded at the fixed 16-byte field ends.
    for(int n=0;n<20;++n){release(g);press(g,right);}
    assert(g.records().nameCursor==15);
    for(int n=0;n<20;++n){release(g);press(g,left);}
    assert(g.records().nameCursor==0);

    // A confirms through the original Enter path and commits the working block.
    // SDL_CONTROLLER_BUTTON_A also carries a J1 legacy edge code; it must not
    // turn into '~' before the native A-confirm path sees it.
    release(g); controllerPress(g,action,0x104);
    assert(!g.records().nameEntry && g.records().dirty);
    const auto& saved=g.legacyHighScores().blocks[0].names[1];
    assert(static_cast<unsigned char>(saved[0])==0xC1);
    assert(static_cast<unsigned char>(saved[1])==0xC0);

    // PC keyboard path remains append-based and does not force D-Pad cursor mode.
    Game k; GameTestProbe::setBlock(k,ranked()); GameTestProbe::enterPost(k,950u);
    k.update(none,700);
    InputState key{}; key.legacyTextByte='z'; k.update(key,0);
    assert(k.records().typedNameLength==1);
    assert(k.records().workingBlock.names[1][0]=='z');
    assert(!k.records().dpadNameEditing);

    std::cout << "records name dpad r247 PASS\n";
}
