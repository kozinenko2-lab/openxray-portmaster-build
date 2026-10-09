#include <cassert>
#include "platform/input.hpp"
int main(){
    struct Test {unsigned bit; int vk; bool InputState::*state;} tests[]={
        {1,0x26,&InputState::up},{2,0x28,&InputState::down},
        {4,0x25,&InputState::left},{8,0x27,&InputState::right},
        {16,0x0D,&InputState::action},{32,0x1B,&InputState::back},
        {64,0x50,&InputState::pause}
    };
    for(auto t:tests){
        InputState s{};AndroidTouchKeyboard::apply(s,t.bit,t.bit,false);
        assert(s.*(t.state));assert(s.legacyDown(t.vk));
        assert(s.legacyPressedCode==t.vk);
        assert(!s.legacyPressedFromController);
        assert(!s.legacyDown(0x104)&&!s.legacyDown(0x105));
    }
    InputState tap{};AndroidTouchKeyboard::apply(tap,0,16,false);
    assert(tap.legacyPressedCode==0x0D);
    InputState hold{};AndroidTouchKeyboard::apply(hold,2,0,false);
    assert(hold.down&&hold.legacyDown(0x28)&&hold.legacyPressedCode==-1);
    InputState repeated{};AndroidTouchKeyboard::apply(repeated,8,0,true);
    assert(repeated.right&&repeated.legacyPressedCode==0x27);
    InputState multi{};AndroidTouchKeyboard::apply(multi,2|16,2|16,true);
    assert(multi.down&&multi.action&&multi.legacyPressedCode==0x0D);
}