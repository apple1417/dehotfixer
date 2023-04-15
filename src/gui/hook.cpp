#include "pch.h"
#include <chrono>

#include "gui/dx11.h"
#include "gui/dx12.h"
#include "gui/hook.h"

namespace dhf::gui {

namespace {

WNDPROC window_proc_ptr{};

/**
 * @brief `WinProc` hook, used to pass input to imgui.
 */
LRESULT window_proc_hook(HWND h_wnd, UINT u_msg, WPARAM w_param, LPARAM l_param) {
    if (ImGui_ImplWin32_WndProcHandler(h_wnd, u_msg, w_param, l_param) > 0) {
        return 1;
    }

    // NOLINTNEXTLINE(readability-identifier-length)
    auto io = ImGui::GetIO();
    switch (u_msg) {
        case WM_MOUSELEAVE:
        case WM_NCMOUSELEAVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONDBLCLK:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
            if (io.WantCaptureMouse) {
                return 1;
            }
            break;
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_CHAR:
            if (io.WantCaptureKeyboard) {
                return 1;
            }
            break;
    }

    return CallWindowProcA(window_proc_ptr, h_wnd, u_msg, w_param, l_param);
}

}  // namespace

void init(void) {
    // We use `d3d11.dll` as the injector
    // This means `d3d12.dll` might not have been loaded yet (assuming we're using it), which'd
    // cause autodetection to fail.
    // As a workaround hack, we'll just sleep for a bit to let it get loaded

    // NOLINTNEXTLINE(readability-magic-numbers)
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Need to check dx12 first, since dx11 will still get a hit when running it
    if (kiero::init(kiero::RenderType::D3D12) == kiero::Status::Success) {
        dx12::hook();
    } else {
        auto ret = kiero::init(kiero::RenderType::D3D11);
        if (ret == kiero::Status::Success) {
            dx11::hook();
        } else {
            throw std::runtime_error("Failed to initalize graphics overlay: "
                                     + std::to_string(ret));
        }
    }
}

bool hook_keys(HWND h_wnd) {
    window_proc_ptr = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrA(h_wnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(window_proc_hook)));
    return window_proc_ptr != nullptr;
}

}  // namespace dhf::gui