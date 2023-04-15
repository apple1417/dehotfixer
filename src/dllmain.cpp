#include "pch.h"

#include "gui/hook.h"
#include "settings.h"
#include "time_travel.h"
#include "vault_cards.h"
#include "version.h"

namespace {

HMODULE this_module;

/**
 * @brief Main startup thread.
 * @note Instance of `LPTHREAD_START_ROUTINE`.
 *
 * @return unused.
 */
DWORD WINAPI startup_thread(LPVOID /*unused*/) {
    dhf::settings::init(this_module);

    std::cout << "[dhf] Initalizing\n";

    auto mh_ret = MH_Initialize();
    if (mh_ret != MH_OK) {
        std::cerr << "[dhf] Minhook initialization failed: " << mh_ret << "\n";
        return 0;
    }

    try {
        if (dhf::settings::is_bl3()) {
            std::cout << "[dhf] Detected BL3, injecting extra hooks\n";
            dhf::time::init();
            dhf::vault_cards::init();
        }

        dhf::gui::init();
    } catch (std::exception& ex) {
        std::cerr << "[dhf] Exception occured during initalization: " << ex.what() << "\n";
    }

    std::cout << "[dhf] Dehotfixer " VERSION_STRING " loaded\n";

    return 1;
}

}  // namespace

/**
 * @brief Main entry point.
 *
 * @param h_module Handle to module for this dll.
 * @param ul_reason_for_call Reason this is being called.
 * @return True if loaded successfully, false otherwise.
 */
// NOLINTNEXTLINE(readability-identifier-naming)  - for `DllMain`
BOOL APIENTRY DllMain(HMODULE h_module, DWORD ul_reason_for_call, LPVOID /*unused*/) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            this_module = h_module;
            DisableThreadLibraryCalls(h_module);
            CreateThread(nullptr, 0, &startup_thread, nullptr, 0, nullptr);
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
