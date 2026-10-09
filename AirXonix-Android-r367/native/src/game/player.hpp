#pragma once
#include <vector>
#include "field.hpp"
#include "platform/input.hpp"

struct TrailCell { uint8_t x=0, progress=0, y=0, hazard=0; };

struct PlayerHazardTestProbe;

class Player {
public:
    void reset();
    void resetForLevel();
    void resetAfterDeath();
    void prepareRespawnCoordinates();
    void finishRespawn();
    void update(const InputState& input,int dtMs,Field& field,const std::vector<GridSeed>& captureSeeds);
    void setMaxSpeed(float value){ maxSpeed_=value; }
    float maxSpeed() const{return maxSpeed_;}
    int x() const {return x_;} int y() const{return y_;}
    int prevX() const{return prevX_;} int prevY() const{return prevY_;}
    float worldX() const; float worldZ() const;
    float visualY() const{return visualY_;}
    bool cutting() const{return cutting_;}
    bool dead() const{return dead_;}
    void kill(){dead_=true;}
    void clearDeath(){dead_=false;}
    float speed() const{return speed_;}
    float legacyDeathDriftX() const{return direction_==3?-0.00003f:(direction_==4?0.00003f:0.0f);}
    float legacyDeathDriftZ() const{return direction_==1?0.00003f:(direction_==2?-0.00003f:0.0f);}
    const std::vector<TrailCell>& trail() const{return trail_;}
    void advanceTrailPresentation(int dtMs);
    void clearTrailForDeath();
    bool markTrailHazardNear(int centerX,int centerY);
    bool updateTrailHazard(int dtMs);
private:
    friend struct PlayerHazardTestProbe;
    void stepCell(const InputState&,int frameDtMs,Field&,const std::vector<GridSeed>&);
    int prevX_=32,prevY_=-2,x_=32,y_=-2;
    float progress_=0.f;
    float speed_=0.03f,maxSpeed_=0.03f;
    float visualY_=0.008f;
    bool cutting_=false,dead_=false;
    int direction_=0; // 1 up,2 down,3 left,4 right
    int hazardAccumulatorMs_=0; // 0x41A6E5 cadence: sequential propagation every 0x11 ms.
    std::vector<TrailCell> trail_;
};
