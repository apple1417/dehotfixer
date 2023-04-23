#include "pch.h"

#include "d3d11_proxy.h"
#include "gui/gui.h"
#include "hfdat.h"
#include "hotfixes/hooks.h"
#include "settings.h"
#include "time_travel.h"
#include "vault_cards.h"
#include "version.h"

namespace {

/**
 * @brief Main startup thread.
 * @note Instance of `LPTHREAD_START_ROUTINE`.
 *
 * @return unused.
 */
DWORD WINAPI startup_thread(LPVOID /*unused*/) {
    std::cout << "[dhf] Initalizing\n";

    auto mh_ret = MH_Initialize();
    if (mh_ret != MH_OK) {
        std::cerr << "[dhf] Minhook initialization failed: " << mh_ret << "\n";
        return 0;
    }

    try {
        dhf::hotfixes::init();
        dhf::hfdat::init();

        if (dhf::settings::is_bl3) {
            std::cout << "[dhf] Detected BL3, injecting extra hooks\n";
            dhf::time_travel::init();
            dhf::vault_cards::init();
        }

        dhf::gui::init();
    } catch (std::exception& ex) {
        std::cerr << "[dhf] Exception occured during initalization: " << ex.what() << "\n";
    }

    std::cout << "[dhf] " FULL_PROJECT_NAME " loaded\n";

    return 1;
}

/**
 * @brief Creates a new console window, and redirects the standard streams to point to it
 * @note Taken from https://github.com/FromDarkHell/BL3DX11Injection/
 */
void create_console(void) {
    AllocConsole();
    SetConsoleTitleA("dehotfixer debug console");

    // All of this is necessary so that way we can properly use the output of the console
    FILE* new_stdin = nullptr;
    FILE* new_stdout = nullptr;
    FILE* new_stderr = nullptr;
    freopen_s(&new_stdin, "CONIN$", "r", stdin);
    freopen_s(&new_stdout, "CONOUT$", "w", stderr);
    freopen_s(&new_stderr, "CONOUT$", "w", stdout);

    HANDLE stdout_handle =
        CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    HANDLE stdin_handle =
        CreateFileA("CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    // Make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog point to console as well
    std::ios::sync_with_stdio(true);

    SetStdHandle(STD_INPUT_HANDLE, stdin_handle);
    SetStdHandle(STD_OUTPUT_HANDLE, stdout_handle);  // Set our STD handles
    SetStdHandle(STD_ERROR_HANDLE, stdout_handle);   // stderr is going back to STDOUT

    // Clear the error states for all of the C++ stream objects.
    // Attempting to access the streams before they're valid causes them to enter an error state.
    std::wcout.clear();
    std::cout.clear();
    std::wcerr.clear();
    std::cerr.clear();
    std::wcin.clear();
    std::cin.clear();
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
            DisableThreadLibraryCalls(h_module);

            // It's important to setup the dx11 proxy asap
            // To tell if we need to, we need settings initalized
            // We also want to have created the external console window (if required)
            // All other initalization can be done in a thread

            dhf::settings::init(h_module);
            if (dhf::settings::debug_console) {
                create_console();
            }
            if (!dhf::settings::loaded_via_plugin_loader) {
                dhf::d3d11_proxy::init();
            }

            CreateThread(nullptr, 0, &startup_thread, nullptr, 0, nullptr);
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
