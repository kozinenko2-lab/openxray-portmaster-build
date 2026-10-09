#!/usr/bin/env python3
"""Finish EKA2L1 v3.2 main entrypoint against the exact v3.1 CI source."""
from pathlib import Path

file = Path("src/emu/portmaster/src/main.cpp")
source = file.read_text()
anchor = "    if (opt.probe) {"
assert source.count(anchor) == 1, "EKA2L1 v3.1 --probe entrypoint not found"
assert 'if (opt.install_firmware_path)' not in source, "v3.2 main logic already exists"

snippet = """    // Make firmware and games available before any Symbian OS is configured.
    std::error_code directory_error;
    std::filesystem::create_directories("firmware", directory_error);
    if (directory_error) std::cerr << "Unable to create firmware/: " << directory_error.message() << '\\n';
    directory_error.clear();
    std::filesystem::create_directories("games", directory_error);
    if (directory_error) std::cerr << "Unable to create games/: " << directory_error.message() << '\\n';

    if (opt.install_firmware_path) {
        return portmaster::install_device_from_file(*opt.install_firmware_path) ? 0 : 5;
    }
    if (opt.install_game_path) {
        return portmaster::install_sis_from_file(*opt.install_game_path) ? 0 : 6;
    }
    if (opt.menu || (!opt.uid && !opt.list_apps && !opt.probe)) {
        const portmaster::menu_selection selection =
            portmaster::open_portmaster_menu(std::filesystem::current_path());
        switch (selection.action) {
        case portmaster::menu_action::quit: return 0;
        case portmaster::menu_action::install_firmware:
            return portmaster::install_device_from_file(selection.path) ? 0 : 5;
        case portmaster::menu_action::install_game:
            return portmaster::install_sis_from_file(selection.path) ? 0 : 6;
        case portmaster::menu_action::list_apps:
            opt.list_apps = true;
            break;
        case portmaster::menu_action::probe:
            opt.probe = true;
            break;
        }
    }

"""
file.write_text(source.replace(anchor, snippet+anchor, 1))
assert 'opt.install_firmware_path' in file.read_text()
print("EKA2L1 v3.2 boot/menu entrypoint applied to CI source")
