#include "pch.h"
#include "settings.h"

namespace dhf::settings {

namespace {

const constexpr auto BL3_EXECUTABLE_NAME = "Borderlands3";

std::filesystem::path exe_path{};
std::filesystem::path dll_path_internal{};
bool is_bl3_internal = false;

}  // namespace

const std::filesystem::path& dll_path = dll_path_internal;
const bool& is_bl3 = is_bl3_internal;

void init(HMODULE this_module) {
    char buf[MAX_PATH];
    if (GetModuleFileNameA(nullptr, &buf[0], sizeof(buf)) != 0) {
        exe_path = std::filesystem::path{buf};
        is_bl3_internal = exe_path.stem() == BL3_EXECUTABLE_NAME;
    } else {
        is_bl3_internal = false;
    }

    if (GetModuleFileNameA(this_module, &buf[0], sizeof(buf)) != 0) {
        dll_path_internal = std::filesystem::path{buf};
    }
}

}  // namespace dhf::settings
