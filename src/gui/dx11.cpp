#include "pch.h"

#include "gui/dx11.h"
#include "gui/gui.h"
#include "gui/hook.h"
#include "imgui_impl_dx11.h"

namespace dhf::gui::dx11 {

namespace {

const constexpr auto PRESENT_FUNC_IDX = 8;
const constexpr auto RESIZE_BUFFERS_FUNC_IDX = 13;

ID3D11DeviceContext* context{};
ID3D11Device* device{};
ID3D11RenderTargetView* main_render_view = nullptr;

bool initalized = false;

/**
 * @brief Creates the main render view.
 *
 * @param swap_chain The in use swap chain.
 * @return True if the render view was successfull created.
 */
bool create_main_render_view(IDXGISwapChain* swap_chain) {
    ID3D11Texture2D* back_buffer{};
    auto ret =
        swap_chain->GetBuffer(0, IID_ID3D11Texture2D, reinterpret_cast<void**>(&back_buffer));
    if (ret != S_OK) {
        std::cerr << "[dhf] DX11 hook initalization failed: Couldn't get texture buffer (" << ret
                  << ")!\n";
        return false;
    }

    ret = device->CreateRenderTargetView(back_buffer, nullptr, &main_render_view);
    // Make sure to release regardless
    back_buffer->Release();

    if (ret != S_OK) {
        std::cerr << "[dhf] DX11 hook initalization failed: Couldn't create render target (" << ret
                  << ")!\n";
        return false;
    }

    return true;
}

/**
 * @brief Initalizes the dx11 hook if it isn't already.
 *
 * @param swap_chain The in use swap chain.
 * @return True if the hook was successfully initalized, false otherwise.
 */
bool ensure_initalized(IDXGISwapChain* swap_chain) {
    static bool run_once = false;
    if (run_once) {
        return initalized;
    }
    run_once = true;

    DXGI_SWAP_CHAIN_DESC desc;
    auto ret = swap_chain->GetDesc(&desc);
    if (ret != S_OK) {
        std::cerr << "[dhf] DX11 hook initalization failed: Couldn't get swap chain descriptor ("
                  << ret << ")!\n";
        return false;
    }

    ret = swap_chain->GetDevice(IID_ID3D11Device, reinterpret_cast<void**>(&device));
    if (ret != S_OK) {
        std::cerr << "[dhf] DX11 hook initalization failed: Couldn't get device (" << ret << ")!\n";
        return false;
    }

    // Returns void, no error checking needed
    device->GetImmediateContext(&context);

    // This function logs it's own errors
    if (!create_main_render_view(swap_chain)) {
        return false;
    }

    if (!hook_keys(desc.OutputWindow)) {
        std::cerr << "[dhf] DX11 hook initalization failed: Failed to replace winproc (" << ret
                  << ")!\n";
        return false;
    }

    ImGui::CreateContext();

    if (!ImGui_ImplWin32_Init(desc.OutputWindow)) {
        std::cerr << "[dhf] DX11 hook initalization failed: ImGui win32 init failed!\n";
        return false;
    }
    if (!ImGui_ImplDX11_Init(device, context)) {
        std::cerr << "[dhf] DX11 hook initalization failed: ImGui dx11 init failed!\n";
        return false;
    }

    initalized = true;
    return true;
}

using present_func = HRESULT (*)(IDXGISwapChain* self, UINT sync_interval, UINT flags);
present_func present_ptr;

/**
 * @brief Hook for `IDXGISwapChain::Present`, used to inject imgui.
 */
HRESULT present_hook(IDXGISwapChain* self, UINT sync_interval, UINT flags) {
    if (ensure_initalized(self)) {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        try {
            render();
        } catch (const std::exception& ex) {
            std::cerr << "[dhf] Exception occured during render loop: " << ex.what() << "\n";
        }

        ImGui::EndFrame();
        ImGui::Render();

        context->OMSetRenderTargets(1, &main_render_view, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    return present_ptr(self, sync_interval, flags);
}

using resize_buffers_func = HRESULT (*)(IDXGISwapChain* self,
                                        UINT buffer_count,
                                        UINT width,
                                        UINT height,
                                        DXGI_FORMAT new_format,
                                        UINT swap_chain_flags);
resize_buffers_func resize_buffers_ptr;

/**
 * @brief Hook for `IDXGISwapChain::ResizeBuffers`, used to handle resizing.
 */
HRESULT resize_buffers_hook(IDXGISwapChain* self,
                            UINT buffer_count,
                            UINT width,
                            UINT height,
                            DXGI_FORMAT new_format,
                            UINT swap_chain_flags) {
    if (initalized) {
        if (main_render_view != nullptr) {
            context->OMSetRenderTargets(0, nullptr, nullptr);
            main_render_view->Release();
        }

        ImGui_ImplDX11_InvalidateDeviceObjects();
    }

    auto ret = resize_buffers_ptr(self, buffer_count, width, height, new_format, swap_chain_flags);

    if (initalized) {
        create_main_render_view(self);
        context->OMSetRenderTargets(1, &main_render_view, nullptr);

        D3D11_VIEWPORT view;
        view.Width = (float)width;
        view.Height = (float)height;
        view.MinDepth = 0.0;
        view.MaxDepth = 1.0;
        view.TopLeftX = 0;
        view.TopLeftY = 0;
        context->RSSetViewports(1, &view);
    }

    return ret;
}

}  // namespace

void hook(void) {
    auto ret = kiero::bind(PRESENT_FUNC_IDX, reinterpret_cast<void**>(&present_ptr),
                           reinterpret_cast<void*>(present_hook));
    if (ret != kiero::Status::Success) {
        throw std::runtime_error("Failed to inject dx11 present hook: " + std::to_string(ret));
    }

    ret = kiero::bind(RESIZE_BUFFERS_FUNC_IDX, reinterpret_cast<void**>(&resize_buffers_ptr),
                      reinterpret_cast<void*>(resize_buffers_hook));
    if (ret != kiero::Status::Success) {
        throw std::runtime_error("Failed to inject dx11 resize buffers hook: "
                                 + std::to_string(ret));
    }
}

}  // namespace dhf::gui::dx11
