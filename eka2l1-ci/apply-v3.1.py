#!/usr/bin/env python3
"""Apply the v3.1 probe frontend on top of the known EKA2L1 H700 v3 CI patch."""
from pathlib import Path
import shutil

root = Path.cwd()
probe = root / "src/emu/portmaster/src/diagnostics.cpp"
shutil.copyfile(root.parent / "eka2l1-ci/v3.1-diagnostics.cpp", probe)

header = root / "src/emu/portmaster/include/portmaster/diagnostics.h"
header.write_text("""// Copyright (c) 2026 EKA2L1 PortMaster contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace eka2l1::portmaster {
    // Returns nonzero when SDL video/GLES 3 initialization failed.
    // No Symbian firmware is required.
    int run_h700_probe(int seconds);
}
""")

def change(relative_path, old, new):
    path = root / relative_path
    code = path.read_text()
    assert code.count(old) == 1, f"Expected unique pattern in {relative_path}: {old[:80]!r} (got {code.count(old)})"
    path.write_text(code.replace(old, new, 1))

cmake = "src/emu/portmaster/CMakeLists.txt"
change(cmake, "    include/portmaster/emu_window_sdl2.h", "    include/portmaster/diagnostics.h\n    include/portmaster/emu_window_sdl2.h")
change(cmake, "    src/emu_window_sdl2.cpp", "    src/diagnostics.cpp\n    src/emu_window_sdl2.cpp")

main = "src/emu/portmaster/src/main.cpp"
change(main, "#include <portmaster/state.h>", "#include <portmaster/state.h>\n#include <portmaster/diagnostics.h>")
change(main, "        bool list_apps = false;", "        bool list_apps = false;\n        bool probe = false;\n        int probe_seconds = 12;")
change(main, """            } else if (arg == "--list-apps") {
                out.list_apps = true;""", """            } else if (arg == "--list-apps") {
                out.list_apps = true;
            } else if (arg == "--probe") {
                out.probe = true;
            } else if (arg == "--probe-seconds" && i + 1 < argc) {
                const std::string number = argv[++i];
                try {
                    std::size_t used = 0;
                    const long seconds = std::stol(number, &used, 10);
                    if (used != number.size() || seconds < 1 || seconds > 120) return false;
                    out.probe_seconds = static_cast<int>(seconds);
                    out.probe = true;
                } catch (...) {
                    std::cerr << "Invalid probe duration: " << number << '\\n';
                    return false;
                }""")
change(main, '            << "  eka2l1_portmaster --list-apps\\n"', '            << "  eka2l1_portmaster --list-apps\\n"\n            << "  eka2l1_portmaster --probe [--probe-seconds 12] (no Symbian firmware required)\\n"')
change(main, "    eka2l1::portmaster::emulator_state state;", """    // Hardware probe runs before Symbian firmware initialization.
    if (opt.probe) {
        return eka2l1::portmaster::run_h700_probe(opt.probe_seconds);
    }

    eka2l1::portmaster::emulator_state state;""")

print("EKA2L1 H700 v3.1 probe sources staged successfully.")
