#pragma once
#include <cstdint>
#include <string>
#include <vector>
struct WavePackEntry { std::string id; std::uint32_t value; };
std::vector<WavePackEntry> parseWavePack(const std::vector<std::uint8_t>& data);
