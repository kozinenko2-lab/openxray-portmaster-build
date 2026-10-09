#pragma once
#include <string>
#include <filesystem>
#include <initializer_list>

struct GamePaths {
    std::string root;
    std::string assets;
    std::string textures;
    std::string raw;
    std::string music;
    std::string saves;
    std::string originalExe;
    std::string originalMusic;
};

GamePaths makeGamePathsFromRoot(const std::filesystem::path& root);
GamePaths makeGamePaths(const char* argv0);
bool fileExists(const std::string& path);
std::string firstExistingFile(std::initializer_list<std::string> candidates);
