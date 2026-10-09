#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include "level.hpp"

struct GridSeed { int x=0, y=0; };
struct FieldTestProbe;

class Field {
public:
    static constexpr int W=64, H=64;
    static constexpr uint8_t Empty=0x00, Safe=0x20, Trail=0x40;

    void build(const LevelRecord& level);
    uint8_t at(int x,int y) const;
    void set(int x,int y,uint8_t value);
    bool inside(int x,int y) const { return x>=0 && x<W && y>=0 && y<H; }
    const std::array<uint8_t,W*H>& cells() const { return cell_; }
    const std::array<float,16>& markerTimers() const { return markerTimer_; }

    uint8_t beginCapture(const std::vector<GridSeed>& enemySeeds);
    std::uint8_t consumeStartedCaptureMarker(){ const auto m=startedCaptureMarker_; startedCaptureMarker_=0; return m; }
    int markerCellCount(std::uint8_t marker) const;
    void updateCaptureAnimations(int dtMs);
    int occupiedMaskedCount() const;
    int nonzeroCount() const;
    bool markerActive(uint8_t marker) const;
    float markerPhase(uint8_t marker) const;
private:
    friend struct FieldTestProbe;
    void floodClear(int sx,int sy,uint8_t marker);
    std::array<uint8_t,W*H> cell_{};
    std::array<float,16> markerTimer_{};
    std::uint8_t startedCaptureMarker_=0;
};
