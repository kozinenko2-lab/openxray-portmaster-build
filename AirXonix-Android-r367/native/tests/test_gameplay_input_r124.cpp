#include <cassert>
#include <iostream>
#include "game/legacy_input_trace.hpp"
int main(){
    const auto&t=kLegacyGameplayInputTrace;
    assert(t.function==0x00419270u && t.eventPoll==0x00409410u);
    assert(t.escapeCode==0x1B && t.pauseKeyCode==0x50 && t.pauseEventCode==0x13);
    assert(t.rightBinding==0x025B7888u && t.leftBinding==0x025B788Cu);
    assert(t.backBinding==0x025B7890u && t.forwardBinding==0x025B7894u);
    assert(t.movementFlags[0]==0x0257DAA0u && t.movementFlags[3]==0x0257DAACu);
    assert(t.arrowCodes[0]==0x26 && t.arrowCodes[1]==0x28 && t.arrowCodes[2]==0x25 && t.arrowCodes[3]==0x27);
    std::cout << "gameplay input r124 ok\n";
}
