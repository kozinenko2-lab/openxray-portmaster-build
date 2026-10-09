// Copyright (c) 2026 EKA2L1 PortMaster contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// Standalone hardware preflight; intentionally independent of EKA2L1 system state.
#include <portmaster/diagnostics.h>

#include <SDL.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace eka2l1::portmaster {
    namespace {
        constexpr unsigned gl_vendor = 0x1F00;
        constexpr unsigned gl_renderer = 0x1F01;
        constexpr unsigned gl_version = 0x1F02;
        using get_string_fn = const unsigned char *(*)(unsigned);

        const char *sdl_name(const char *s) { return s ? s : "<null>"; }

        void print_gamepad(SDL_GameController *pad, const int index) {
            SDL_Joystick *joy = SDL_GameControllerGetJoystick(pad);
            const SDL_JoystickGUID guid = SDL_JoystickGetGUID(joy);
            char guid_text[33]{};
            SDL_JoystickGetGUIDString(guid, guid_text, sizeof(guid_text));
            std::cout << "[INPUT] pad index=" << index
                      << " instance=" << SDL_JoystickInstanceID(joy)
                      << " name=" << sdl_name(SDL_GameControllerName(pad))
                      << " guid=" << guid_text << '\n';
            char *mapping = SDL_GameControllerMapping(pad);
            std::cout << "[INPUT] mapping=" << (mapping ? mapping : "<missing>") << '\n';
            if (mapping) SDL_free(mapping);
        }

        struct monitored_pad {
            SDL_GameController *pad = nullptr;
            std::array<Sint16, SDL_CONTROLLER_AXIS_MAX> axis{};
            std::array<Uint8, SDL_CONTROLLER_BUTTON_MAX> button{};
        };

        bool init_video(SDL_Window *&window, SDL_GLContext &context) {
            if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
                std::cerr << "[VIDEO] SDL_InitSubSystem: " << SDL_GetError() << '\n';
                return false;
            }
            std::cout << "[VIDEO] backend=" << sdl_name(SDL_GetCurrentVideoDriver()) << '\n';
            // Mali/fbdev cannot safely fall back to desktop OpenGL/GL4ES.
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
            SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
            SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
            const Uint32 fullscreen = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP;
            window = SDL_CreateWindow("EKA2L1 H700 hardware probe", SDL_WINDOWPOS_CENTERED,
                                      SDL_WINDOWPOS_CENTERED, 640, 480, fullscreen);
            if (!window) {
                std::cerr << "[VIDEO] fullscreen window: " << SDL_GetError() << '\n';
                window = SDL_CreateWindow("EKA2L1 H700 hardware probe", SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
            }
            if (!window) {
                std::cerr << "[VIDEO] SDL_CreateWindow: " << SDL_GetError() << '\n';
                return false;
            }
            context = SDL_GL_CreateContext(window);
            if (!context) {
                std::cerr << "[GLES] SDL_GL_CreateContext: " << SDL_GetError() << '\n';
                return false;
            }
            if (SDL_GL_MakeCurrent(window, context) != 0) {
                std::cerr << "[GLES] SDL_GL_MakeCurrent: " << SDL_GetError() << '\n';
                return false;
            }
            int w = 0, h = 0;
            SDL_GL_GetDrawableSize(window, &w, &h);
            std::cout << "[VIDEO] drawable=" << w << 'x' << h << '\n';
            const auto gl_get_string = reinterpret_cast<get_string_fn>(SDL_GL_GetProcAddress("glGetString"));
            if (!gl_get_string) {
                std::cerr << "[GLES] glGetString unavailable: " << SDL_GetError() << '\n';
                return false;
            }
            const char *vendor = reinterpret_cast<const char *>(gl_get_string(gl_vendor));
            const char *renderer = reinterpret_cast<const char *>(gl_get_string(gl_renderer));
            const char *version = reinterpret_cast<const char *>(gl_get_string(gl_version));
            std::cout << "[GLES] GL_VENDOR=" << sdl_name(vendor) << '\n'
                      << "[GLES] GL_RENDERER=" << sdl_name(renderer) << '\n'
                      << "[GLES] GL_VERSION=" << sdl_name(version) << '\n';
            if (!vendor || !renderer || !version ||
                std::string(version).find("OpenGL ES 3.") == std::string::npos) {
                std::cerr << "[GLES] ERROR: native OpenGL ES 3.x not available" << '\n';
                return false;
            }
            SDL_GL_SwapWindow(window);
            return true;
        }
    }

    int run_h700_probe(int seconds) {
        // Only the diagnostic path uses this SDL setup; normal boot is unchanged.
        const SDL_version version{SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL};
        SDL_version linked{};
        SDL_GetVersion(&linked);
        std::cout << "[PROBE] EKA2L1 H700 SDL/GLES/DualStick preflight\n"
                  << "[PROBE] SDL headers=" << int(version.major) << '.' << int(version.minor) << '.' << int(version.patch)
                  << " runtime=" << int(linked.major) << '.' << int(linked.minor) << '.' << int(linked.patch) << '\n';
        std::cout << "[PROBE] SDL_VIDEODRIVER=" << sdl_name(SDL_getenv("SDL_VIDEODRIVER")) << '\n';
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) {
            std::cerr << "[INPUT] SDL_Init gamecontroller: " << SDL_GetError() << '\n';
        }
        for (const char *path : {"resources/gamecontrollerdb.txt", "resources/gamecontrollerdb-h700.txt"}) {
            const int added = SDL_GameControllerAddMappingsFromFile(path);
            std::cout << "[INPUT] mapping file " << path << " count=" << added;
            if (added < 0) std::cout << " error=" << SDL_GetError();
            std::cout << '\n';
        }
        SDL_Window *window = nullptr;
        SDL_GLContext context = nullptr;
        const bool video_ok = init_video(window, context);
        const int count = SDL_NumJoysticks();
        std::cout << "[INPUT] SDL_NumJoysticks=" << count << '\n';
        std::vector<monitored_pad> pads;
        for (int i = 0; i < count; ++i) {
            char guid_text[33]{};
            SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(i), guid_text, sizeof(guid_text));
            std::cout << "[INPUT] device=" << i << " name=" << sdl_name(SDL_JoystickNameForIndex(i))
                      << " guid=" << guid_text << " controller=" << SDL_IsGameController(i) << '\n';
            if (!SDL_IsGameController(i)) continue;
            auto *pad = SDL_GameControllerOpen(i);
            if (!pad) {
                std::cerr << "[INPUT] open device " << i << " failed: " << SDL_GetError() << '\n';
                continue;
            }
            print_gamepad(pad, i);
            pads.push_back({pad, {}, {}});
        }
        std::cout << "[PROBE] Sample input for " << seconds << "s. Move both sticks and press A/B/L/R/L2/R2; ESC/Guide exits.\n";
        std::cout.flush();
        const Uint32 start = SDL_GetTicks();
        Uint32 last_axis_log = 0;
        bool quit = false;
        while (!quit && SDL_GetTicks() - start < static_cast<Uint32>(seconds) * 1000u) {
            SDL_Event event{};
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                    quit = true;
                }
            }
            SDL_GameControllerUpdate();
            for (auto &state : pads) {
                if (!SDL_GameControllerGetAttached(state.pad)) continue;
                for (int button = 0; button < SDL_CONTROLLER_BUTTON_MAX; ++button) {
                    const auto now = SDL_GameControllerGetButton(state.pad, static_cast<SDL_GameControllerButton>(button));
                    if (now != state.button[button]) {
                        state.button[button] = now;
                        std::cout << "[BUTTON] " << SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(button))
                                  << " " << (now ? "down" : "up") << '\n';
                        if (button == SDL_CONTROLLER_BUTTON_GUIDE && now) quit = true;
                    }
                }
                const Uint32 elapsed = SDL_GetTicks() - start;
                if (elapsed - last_axis_log >= 180) {
                    for (int axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; ++axis) {
                        const auto now = SDL_GameControllerGetAxis(state.pad, static_cast<SDL_GameControllerAxis>(axis));
                        if (std::abs(static_cast<int>(now) - static_cast<int>(state.axis[axis])) > 7000) {
                            state.axis[axis] = now;
                            std::cout << "[AXIS] " << SDL_GameControllerGetStringForAxis(static_cast<SDL_GameControllerAxis>(axis))
                                      << "=" << now << '\n';
                        }
                    }
                }
            }
            if (SDL_GetTicks() - start - last_axis_log >= 180) last_axis_log = SDL_GetTicks() - start;
            SDL_Delay(12);
        }
        for (auto &state : pads) SDL_GameControllerClose(state.pad);
        if (context) SDL_GL_DeleteContext(context);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
        std::cout << "[PROBE] done; GLES status=" << (video_ok ? "OK" : "FAILED") << '\n';
        return video_ok ? 0 : 4;
    }
}
