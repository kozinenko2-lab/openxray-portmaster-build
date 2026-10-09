#include "player.hpp"
#include <cstdlib>
#include <algorithm>
#include <cmath>

void Player::reset(){prevX_=x_=32;prevY_=y_=-2;progress_=0.f;speed_=maxSpeed_=0.03f;visualY_=0.008f;cutting_=dead_=false;direction_=0;hazardAccumulatorMs_=0;trail_.clear();}

void Player::resetForLevel(){
    // r266 DIRECT EXE: per-level init resets the max-speed/effect target but
    // does not write the current player speed global 0x257DA40. Preserve the
    // current speed across level transitions/restarts; session bootstrap reset()
    // remains the only path that initializes both values to 0.03.
    prevX_=x_=32; prevY_=y_=-2; progress_=0.f; maxSpeed_=0.03f; visualY_=0.008f;
    cutting_=dead_=false; direction_=0; hazardAccumulatorMs_=0; trail_.clear();
}

void Player::prepareRespawnCoordinates(){
    // r167 DIRECT EXE 0x41C599..0x41C5BA. These four logical grid globals are
    // written as soon as the death routine enters its final (<1000 ms) branch:
    // current=(32,0), previous=(32,0). Do not clear death/trail here: the x86
    // death world continues running until 0x41CE2C.
    prevX_=x_=32; prevY_=y_=0; progress_=0.f;
}

void Player::finishRespawn(){
    // 0x41CE2C completion clears the death presentation and returns to live
    // gameplay using the coordinates that were already prepared above.
    // r266: death/respawn does not write current speed or max-speed effect state.
    visualY_=0.00800000037997961f;
    cutting_=dead_=false; direction_=0; hazardAccumulatorMs_=0; trail_.clear();
}

void Player::resetAfterDeath(){
    // Compatibility helper for tests/older call sites: perform the two exact
    // phases back-to-back. Runtime Game now invokes them at their EXE timings.
    prepareRespawnCoordinates();
    finishRespawn();
}

void Player::stepCell(const InputState& in,int frameDtMs,Field& f,const std::vector<GridSeed>& seeds){
    const int oldX=x_,oldY=y_; prevX_=oldX;prevY_=oldY;progress_=1.f;
    if(cutting_){
        speed_=maxSpeed_;
        if(in.up){ if(direction_==2) --y_; else {++y_;direction_=1;} }
        if(in.down){ if(direction_==1) ++y_; else {--y_;direction_=2;} }
        if(in.left){ if(direction_==4) ++x_; else {--x_;direction_=3;} }
        if(in.right){ if(direction_==3) --x_; else {++x_;direction_=4;} }
        if(x_==oldX&&y_==oldY){progress_=0.f;return;}
        const uint8_t target=f.at(x_,y_);
        // DIRECT EXE 0x419992/0x419CBA -> 0x41A057: touching the active
        // trail raises the death flag, but the current cell transition still
        // completes its bookkeeping before the outer loop observes death.
        if(target==Field::Trail)dead_=true;
        if(target==Field::Safe){
            for(const auto& t:trail_) f.set(t.x,t.y,t.hazard?Field::Empty:Field::Safe);
            f.set(oldX,oldY,Field::Safe);
            f.beginCapture(seeds);
            cutting_=false; trail_.clear(); direction_=0; return;
        }
        if(f.inside(oldX,oldY)){
            TrailCell t; t.x=static_cast<uint8_t>(oldX);t.y=static_cast<uint8_t>(oldY);
            trail_.push_back(t);f.set(oldX,oldY,Field::Trail);
        }
        return;
    }

    trail_.clear();
    const float accel=static_cast<float>(frameDtMs)*0.0015f;
    if(in.up){y_=std::min(y_+1,67);speed_+=accel;}
    if(in.down){y_=std::max(y_-1,-4);speed_+=accel;}
    if(in.left){x_=std::max(x_-1,-4);speed_+=accel;}
    if(in.right){x_=std::min(x_+1,67);speed_+=accel;}
    speed_-=static_cast<float>(frameDtMs)*0.0002f;
    // 0x419BC6 compares against 0.001 (0x43B314), but if below that
    // threshold writes 0.0001 rather than clamping to 0.001. Preserve the
    // discontinuity: it is part of the original low-speed feel.
    if(speed_<0.001f)speed_=0.0001f;
    if(speed_>maxSpeed_)speed_=maxSpeed_;
    if(x_>0&&x_<63&&y_>0&&y_<63){
        if((f.at(x_,y_)&0x0f)!=0){x_=oldX;y_=oldY;progress_=0.f;return;}
        if(f.at(x_,y_)==Field::Empty){cutting_=true;direction_=0;}
    }
    // 0x41A057: a transition attempt that did not change the logical grid
    // position consumes no residual cell distance. This keeps input responsive
    // even after the original 0.0001 low-speed discontinuity.
    if(x_==oldX&&y_==oldY)progress_=0.f;
}

void Player::update(const InputState& in,int dtMs,Field& f,const std::vector<GridSeed>& seeds){
    if(dead_||dtMs<=0)return;

    // 0x419849..0x419D04: the original stores a normalized distance still
    // remaining to the next grid crossing. It truncates distance/speed toward
    // zero, consumes that many milliseconds, performs one cell transition,
    // resets the distance to 1.0 and repeats while another crossing fits in
    // this frame. No artificial fixed-tick or minimum-one-ms step is used.
    int remain=dtMs;
    int guard=0;
    while(guard++<512){
        const int msToCell=(speed_>0.f)?static_cast<int>(progress_/speed_):0x7fffffff;
        if(msToCell>=remain){
            progress_-=static_cast<float>(remain)*speed_;
            if(progress_<0.f)progress_=0.f;
            break;
        }
        remain-=msToCell;
        progress_=1.0f;
        stepCell(in,dtMs,f,seeds);
        if(dead_)break;
    }

    const float target=cutting_?0.014f:0.008f;
    if(visualY_>target) visualY_=std::max(target,visualY_-dtMs*0.000007f);
    else if(visualY_<target) visualY_=std::min(target,visualY_+dtMs*0.00002f);
}

float Player::worldX() const { const float blend=1.f-progress_;return (prevX_+(x_-prevX_)*blend)*0.003125f+0.4015625f; }
float Player::worldZ() const { const float blend=1.f-progress_;return (prevY_+(y_-prevY_)*blend)*0.003125f+0.4015625f; }



void Player::advanceTrailPresentation(int dtMs){
    // 0x41A73F..0x41A756: every active trail record owns an 8-bit visual
    // descent byte. It advances by the frame delta and saturates at 100.
    // The renderer then uses .008/.010 - progress*.00008.
    if(dtMs<=0)return;
    const int add=std::min(dtMs,100);
    for(auto& t:trail_)t.progress=static_cast<std::uint8_t>(std::min(100,int(t.progress)+add));
}

void Player::clearTrailForDeath(){
    // 0x41C0C9 and 0x41C0EF: death entry immediately clears the active trail
    // count and cutting state. Field bytes are cleared by Game::handleDeath.
    trail_.clear();
    cutting_=false;
    hazardAccumulatorMs_=0;
}

bool Player::markTrailHazardNear(int centerX,int centerY){
    // 0x41A68D..0x41A6D3. The EXE uses unsigned boundary compares:
    //   center-2 < trail.x < center+2
    //   center-2 < trail.y < center+2
    // so on the integer grid this is the 3x3 neighbourhood centered on the
    // reported hazard coordinate, not an inclusive +/-2 radius.
    bool marked=false;
    for(auto& t:trail_){
        const int tx=int(t.x),ty=int(t.y);
        if(tx>centerX-2 && tx<centerX+2 && ty>centerY-2 && ty<centerY+2){
            t.hazard=1; marked=true;
        }
    }
    return marked;
}

bool Player::updateTrailHazard(int dtMs){
    if(dtMs<=0 || trail_.empty())return false;
    hazardAccumulatorMs_+=dtMs;
    bool lethal=false;
    while(hazardAccumulatorMs_>=0x11){
        hazardAccumulatorMs_-=0x11;
        const std::size_t n=trail_.size();
        if(n<2u)continue;

        // 0x41A6E5..0x41A714: forward pass over adjacent records.
        // This is array-order propagation, not a second spatial-neighbour scan.
        // The original tests AL after the final forward pair and raises
        // 0x257DAB8 only when the hazard wave has reached that end of the trail.
        std::uint8_t al=0;
        for(std::size_t i=0;i+1u<n;++i){
            trail_[i].hazard=std::uint8_t(trail_[i].hazard|trail_[i+1u].hazard);
            al=trail_[i].hazard;
        }
        if(al!=0)lethal=true;

        // 0x41A714..0x41A723: reverse pass, again only between adjacent
        // records, spreading the wave one record toward the opposite end.
        for(std::size_t i=n-1u;i>0u;--i)
            trail_[i].hazard=std::uint8_t(trail_[i].hazard|trail_[i-1u].hazard);
    }
    return lethal;
}
