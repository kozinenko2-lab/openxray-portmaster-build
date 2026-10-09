#include "paths.hpp"
#include <SDL.h>
#if defined(__ANDROID__)
#include <SDL_system.h>
#endif
#include <filesystem>

GamePaths makeGamePaths(const char* argv0) {
    std::filesystem::path root;
#if defined(__ANDROID__)
    // The Android Activity extracts APK assets to internal app storage.
    // This is the only writable root; SDL_GetBasePath() points to /lib/ in APK.
    if (const char* internal = SDL_AndroidGetInternalStoragePath()) root = internal;
    else root = std::filesystem::current_path();
#else
    if (char* base = SDL_GetBasePath()) { root = base; SDL_free(base); }
    else if (argv0 && *argv0) root = std::filesystem::absolute(argv0).parent_path();
    else root = std::filesystem::current_path();
#endif
    return makeGamePathsFromRoot(root);
}
