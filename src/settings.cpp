#include "pch.h"
#include "settings.h"

namespace dhf::settings {

namespace {

const constexpr auto BL3_EXECUTABLE_NAME = "Borderlands3";

std::filesystem::path exe_path{};
std::filesystem::path dll_path_internal{};
bool debug_console_internal = false;
bool is_bl3_internal = false;
bool loaded_via_plugin_loader_internal = false;

}  // namespace

const std::filesystem::path& dll_path = dll_path_internal;
const bool& debug_console = debug_console_internal;
const bool& is_bl3 = is_bl3_internal;
const bool& loaded_via_plugin_loader = loaded_via_plugin_loader_internal;

void init(HMODULE this_module) {
    std::string cmd{GetCommandLineA()};

    char buf[MAX_PATH];
    if (GetModuleFileNameA(nullptr, &buf[0], sizeof(buf)) != 0) {
        exe_path = std::filesystem::path{buf};
    }
    if (GetModuleFileNameA(this_module, &buf[0], sizeof(buf)) != 0) {
        dll_path_internal = std::filesystem::path{buf};
    }

    std::string args{GetCommandLineA()};
    debug_console_internal = args.find("--debug") != std::string::npos;

    is_bl3_internal = exe_path.stem() == BL3_EXECUTABLE_NAME;
    loaded_via_plugin_loader_internal = exe_path.parent_path() != dll_path_internal.parent_path();
}

}  // namespace dhf::settings
