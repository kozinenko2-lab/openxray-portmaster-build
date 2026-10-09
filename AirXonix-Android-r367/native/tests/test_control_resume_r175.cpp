#include "game/player.hpp"
#include <cassert>
#include <cstdio>

struct PlayerHazardTestProbe {
    static void stall(Player& p){ p.speed_=0.0001f; p.progress_=0.0f; p.x_=32; p.prevX_=32; p.y_=10; p.prevY_=10; p.cutting_=false; }
    static float progress(const Player& p){ return p.progress_; }
};

int main(){
    Player p; p.reset(); PlayerHazardTestProbe::stall(p);
    Field f; // empty default field is sufficient for the first responsive fraction
    InputState in{}; in.right=true;
    const int x0=p.x();
    p.update(in,16,f,{});
    // r256: after a no-move crossing the original resets residual distance to 0,
    // so the next held direction is processed immediately even at 0.0001 speed.
    assert(p.speed()>0.001f);
    assert(PlayerHazardTestProbe::progress(p)<1.0f || p.x()!=x0);
    std::puts("control resume r175 ok");
}
