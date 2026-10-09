#pragma once
#include <cstdint>

// Pure logic: keeps virtual gamepad movement continuous in gameplay but
// exposes independent navigation pulses in Android menu screens.
class AndroidTouchMenu {
public:
    static constexpr unsigned kDirections=0x0fu;
    static constexpr unsigned kButtons=0x70u;
    static constexpr std::uint32_t kInitialRepeatMs=340;
    static constexpr std::uint32_t kRepeatMs=145;

    unsigned frame(unsigned held, unsigned pendingEdges, bool menu, std::uint32_t now) {
        held&=0x7fu;
        pendingEdges&=0x7fu;
        unsigned direction=held&kDirections;
        // When a quick flick has already ended, use its stored down edge once.
        if(!direction)direction=pendingEdges&kDirections;
        if(!menu){
            lastDirection_=0;
            repeatAt_=0;
            return held | pendingEdges;
        }
        unsigned pulses=0;
        if(direction!=lastDirection_){
            if(direction){pulses=direction;repeatAt_=now+kInitialRepeatMs;}
            lastDirection_=direction;
        }else if(direction && static_cast<std::int32_t>(now-repeatAt_)>=0){
            pulses=direction;
            repeatAt_=now+kRepeatMs;
        }
        // A/B/Pause must also survive taps shorter than an SDL frame.
        return pulses | ((held|pendingEdges)&kButtons);
    }
private:
    unsigned lastDirection_=0;
    std::uint32_t repeatAt_=0;
};
