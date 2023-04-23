#include "pch.h"

// Based on https://github.com/FromDarkHell/BL3DX11Injection/

namespace dhf::d3d11_proxy {

namespace {

HMODULE dx11_dll_handle{};
FARPROC pointers[3] = {nullptr, nullptr, nullptr};

// NOLINTBEGIN(readability-identifier-naming, readability-identifier-length)

DLL_EXPORT HRESULT D3D11CoreCreateDevice(void* fact,
                                         void* adapt,
                                         unsigned int flag,
                                         void* fl,
                                         unsigned int featureLevels,
                                         void** ppDev) {
    return reinterpret_cast<decltype(&D3D11CoreCreateDevice)>(pointers[0])(fact, adapt, flag, fl,
                                                                           featureLevels, ppDev);
}

DLL_EXPORT HRESULT D3D11CreateDevice(void* adapt,
                                     unsigned int dt,
                                     void* soft,
                                     unsigned int flags,
                                     int* ft,
                                     unsigned int fl,
                                     unsigned int ver,
                                     void** ppDevice,
                                     void* featureLevel,
                                     void** context) {
    return reinterpret_cast<decltype(&D3D11CreateDevice)>(pointers[1])(
        adapt, dt, soft, flags, ft, fl, ver, ppDevice, featureLevel, context);
}

DLL_EXPORT HRESULT D3D11CreateDeviceAndSwapChain(void* adapt,
                                                 unsigned int dt,
                                                 void* soft,
                                                 unsigned int flags,
                                                 int* ft,
                                                 unsigned int fl,
                                                 unsigned int ver,
                                                 void* swapChainDesc,
                                                 void** swapChain,
                                                 void** ppDevice,
                                                 void* featureLevel,
                                                 void** context) {
    return reinterpret_cast<decltype(&D3D11CreateDeviceAndSwapChain)>(pointers[2])(
        adapt, dt, soft, flags, ft, fl, ver, swapChainDesc, swapChain, ppDevice, featureLevel,
        context);
}
// NOLINTEND(readability-identifier-naming, readability-identifier-length)

/**
 * @brief RAII class which suspends all other threads for it's lifespan.
 */
class ThreadSuspender {
   private:
    /**
     * @brief Suspends or resumes all other threads in the process.
     *
     * @param resume True if to resume, false if to suspend them.
     */
    static void adjust_running_status(bool resume) {
        HANDLE thread_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (thread_snapshot == nullptr) {
            CloseHandle(thread_snapshot);
            return;
        }

        THREADENTRY32 te32{};
        te32.dwSize = sizeof(THREADENTRY32);

        if (Thread32First(thread_snapshot, &te32) == 0) {
            CloseHandle(thread_snapshot);
            return;
        }

        do {
            if (te32.th32OwnerProcessID != GetCurrentProcessId()
                || te32.th32ThreadID == GetCurrentThreadId()) {
                continue;
            }

            HANDLE thread =
                OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME, 0,
                           te32.th32ThreadID);
            if (thread != nullptr) {
                CONTEXT context;
                context.ContextFlags = CONTEXT_DEBUG_REGISTERS;

                if (resume) {
                    ResumeThread(thread);
                } else {
                    SuspendThread(thread);
                }

                CloseHandle(thread);
            }
        } while (Thread32Next(thread_snapshot, &te32) != 0);

        CloseHandle(thread_snapshot);
    }

   public:
    ThreadSuspender(void) { adjust_running_status(false); }
    ~ThreadSuspender() { adjust_running_status(true); }

    ThreadSuspender(const ThreadSuspender&) = delete;
    ThreadSuspender(ThreadSuspender&&) = delete;
    ThreadSuspender& operator=(const ThreadSuspender&) = delete;
    ThreadSuspender& operator=(ThreadSuspender&&) = delete;
};

}  // namespace

void init(void) {
    // Suspend all other threads to prevent a giant race condition
    ThreadSuspender suspender{};

    char buf[MAX_PATH];
    if (GetSystemDirectoryA(&buf[0], sizeof(buf)) == 0) {
        std::cerr << "[dhf] Unable to find system dll directory! We're probably about to crash.\n";
        return;
    }

    auto system_dx11 = std::filesystem::path{buf} / "d3d11.dll";
    dx11_dll_handle = LoadLibraryA(system_dx11.generic_string().c_str());

    pointers[0] = GetProcAddress(dx11_dll_handle, "D3D11CoreCreateDevice");
    pointers[1] = GetProcAddress(dx11_dll_handle, "D3D11CreateDevice");
    pointers[2] = GetProcAddress(dx11_dll_handle, "D3D11CreateDeviceAndSwapChain");
}

}  // namespace dhf::d3d11_proxy
