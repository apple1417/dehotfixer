#include "pch.h"
#include "settings.h"

namespace dhf::settings {

namespace {

const constexpr auto BL3_EXECUTABLE_NAME = "Borderlands3";

std::filesystem::path exe_path{};
std::filesystem::path dll_path{};

}  // namespace

void init(HMODULE this_module) {
    std::string cmd{GetCommandLineA()};

    char buf[FILENAME_MAX];
    if (GetModuleFileNameA(nullptr, &buf[0], sizeof(buf)) != 0) {
        exe_path = std::filesystem::path{buf};
    }
    if (GetModuleFileNameA(this_module, &buf[0], sizeof(buf)) != 0) {
        dll_path = std::filesystem::path{buf};
    }
}

bool is_bl3(void) {
    return exe_path.stem() == BL3_EXECUTABLE_NAME;
}

}  // namespace dhf::settings
