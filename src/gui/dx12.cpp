#include "pch.h"

#include "gui/dx12.h"
#include "gui/gui.h"
#include "gui/hook.h"

namespace dhf::gui::dx12 {

namespace {

const constexpr auto EXEC_CMD_QUEUE_FUNC_IDX = 54;
const constexpr auto PRESENT_FUNC_IDX = 140;
const constexpr auto RESIZE_BUFFERS_FUNC_IDX = 145;

struct FrameContext {
    ID3D12CommandAllocator* command_allocator;
    ID3D12Resource* main_render_target_resource;
    D3D12_CPU_DESCRIPTOR_HANDLE main_render_target_descriptor;
};
std::vector<FrameContext> framebuffers;

ID3D12CommandQueue* command_queue = nullptr;

ID3D12Device* device{};
ID3D12DescriptorHeap* srv_heap_desc{};
ID3D12DescriptorHeap* rtv_heap_desc{};
ID3D12GraphicsCommandList* command_list{};

/**
 * @brief Creates the render resource objects for all frame buffers.
 *
 * @param swap_chain The in use swap chain.
 */
void create_render_resources(IDXGISwapChain* swap_chain) {
    const auto rtv_descriptor_size =
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = rtv_heap_desc->GetCPUDescriptorHandleForHeapStart();

    // NOLINTNEXTLINE(modernize-loop-convert)
    for (UINT i = 0; i < framebuffers.size(); i++) {
        framebuffers[i].main_render_target_descriptor = rtv_handle;

        swap_chain->GetBuffer(
            i, IID_ID3D12Resource,
            reinterpret_cast<void**>(&framebuffers[i].main_render_target_resource));
        device->CreateRenderTargetView(framebuffers[i].main_render_target_resource, nullptr,
                                       rtv_handle);
        rtv_handle.ptr += rtv_descriptor_size;
    }
}

/**
 * @brief Initalizes the dx12 hook if it isn't already.
 *
 * @param swap_chain The in use swap chain.
 * @return True if the hook has been successfully initalized.
 */
bool ensure_initalized(IDXGISwapChain3* swap_chain) {
    static bool initalized = false;
    static bool run_once = false;
    if (run_once) {
        return initalized;
    }
    run_once = true;

    // There are probably a bunch of memory leaks left in this if something fails, I don't really
    // understand it all properly

    auto ret = swap_chain->GetDevice(IID_ID3D12Device, reinterpret_cast<void**>(&device));
    if (ret != S_OK) {
        std::cerr << "[dhf] DX12 hook initalization failed: Couldn't get device (" << ret << ")!\n";
        return false;
    }

    DXGI_SWAP_CHAIN_DESC desc;
    ret = swap_chain->GetDesc(&desc);
    if (ret != S_OK) {
        std::cerr << "[dhf] DX12 hook initalization failed: Couldn't get swap chain descriptor ("
                  << ret << ")!\n";
        return false;
    }

    auto buffer_count = desc.BufferCount;
    framebuffers.resize(buffer_count);

    D3D12_DESCRIPTOR_HEAP_DESC srv_desc = {D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, buffer_count,
                                           D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};

    ret = device->CreateDescriptorHeap(&srv_desc, IID_ID3D12DescriptorHeap,
                                       reinterpret_cast<void**>(&srv_heap_desc));
    if (ret != S_OK) {
        std::cerr << "[dhf] DX12 hook initalization failed: Couldn't get srv heap descriptor ("
                  << ret << ")!\n";
        return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC rtv_desc = {D3D12_DESCRIPTOR_HEAP_TYPE_RTV, buffer_count,
                                           D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1};

    ret = device->CreateDescriptorHeap(&rtv_desc, IID_ID3D12DescriptorHeap,
                                       reinterpret_cast<void**>(&rtv_heap_desc));
    if (ret != S_OK) {
        std::cerr << "[dhf] DX12 hook initalization failed: Couldn't get srv heap descriptor ("
                  << ret << ")!\n";
        return false;
    }

    create_render_resources(swap_chain);

    ID3D12CommandAllocator* allocator{};
    ret = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_ID3D12CommandAllocator,
                                         reinterpret_cast<void**>(&allocator));
    if (ret != S_OK) {
        std::cerr << "[dhf] DX12 hook initalization failed: Couldn't get command allocator (" << ret
                  << ")!\n";
        return false;
    }

    for (auto& frame : framebuffers) {
        frame.command_allocator = nullptr;
        ret = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                             IID_ID3D12CommandAllocator,
                                             reinterpret_cast<void**>(&frame.command_allocator));
        if (ret != S_OK) {
            std::cerr << "[dhf] DX12 hook initalization failed: Couldn't create command allocator ("
                      << ret << ")!\n";
            return false;
        }
    }

    ret = device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, framebuffers[0].command_allocator, nullptr,
        IID_ID3D12GraphicsCommandList, reinterpret_cast<void**>(&command_list));
    if (ret != S_OK) {
        std::cerr << "[dhf] DX12 hook initalization failed: Couldn't create command list (" << ret
                  << ")!\n";
        return false;
    }

    if (!hook_keys(desc.OutputWindow)) {
        std::cerr << "[dhf] DX11 hook initalization failed: Failed to replace winproc (" << ret
                  << ")!\n";
        return false;
    }

    ImGui::CreateContext();

    ImGui_ImplWin32_Init(desc.OutputWindow);
    ImGui_ImplDX12_Init(device, (int)buffer_count, DXGI_FORMAT_R8G8B8A8_UNORM, srv_heap_desc,
                        srv_heap_desc->GetCPUDescriptorHandleForHeapStart(),
                        srv_heap_desc->GetGPUDescriptorHandleForHeapStart());

    initalized = true;
    return true;
}

using present_func = HRESULT (*)(IDXGISwapChain3* self, UINT sync_interval, UINT flags);
present_func present_ptr;

/**
 * @brief Hook for `IDXGISwapChain3::Present`, used to inject imgui.
 */
HRESULT present_hook(IDXGISwapChain3* self, UINT sync_interval, UINT flags) {
    if (command_queue != nullptr && ensure_initalized(self)) {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        render();

        ImGui::EndFrame();

        FrameContext& current_frame_context = framebuffers[self->GetCurrentBackBufferIndex()];
        current_frame_context.command_allocator->Reset();

        D3D12_RESOURCE_BARRIER barrier = {
            D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
            D3D12_RESOURCE_BARRIER_FLAG_NONE,
            {{current_frame_context.main_render_target_resource,
              D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_PRESENT,
              D3D12_RESOURCE_STATE_RENDER_TARGET}}};

        command_list->Reset(current_frame_context.command_allocator, nullptr);
        command_list->ResourceBarrier(1, &barrier);
        command_list->OMSetRenderTargets(1, &current_frame_context.main_render_target_descriptor,
                                         FALSE, nullptr);
        command_list->SetDescriptorHeaps(1, &srv_heap_desc);

        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), command_list);

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        command_list->ResourceBarrier(1, &barrier);
        command_list->Close();

        command_queue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(&command_list));
    }

    return present_ptr(self, sync_interval, flags);
}

using exec_cmd_queue_func = void (*)(ID3D12CommandQueue* self,
                                     UINT num_command_lists,
                                     ID3D12CommandList* const* commmand_lists);
exec_cmd_queue_func exec_cmd_queue_ptr;

/**
 * @brief Hook for `ID3D12CommandQueue::ExecuteCommandLists`, used to grab the command queue.
 */
void exec_cmd_queue_hook(ID3D12CommandQueue* self,
                         UINT num_command_lists,
                         ID3D12CommandList* const* commmand_lists) {
    if (command_queue == nullptr && self->GetDesc().Type == D3D12_COMMAND_LIST_TYPE_DIRECT) {
        command_queue = self;
    }

    exec_cmd_queue_ptr(self, num_command_lists, commmand_lists);
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
    for (auto& frame : framebuffers) {
        frame.main_render_target_resource->Release();
    }

    ImGui_ImplDX12_InvalidateDeviceObjects();

    auto ret = resize_buffers_ptr(self, buffer_count, width, height, new_format, swap_chain_flags);

    create_render_resources(self);

    return ret;
}

}  // namespace

void hook(void) {
    auto ret = kiero::bind(PRESENT_FUNC_IDX, reinterpret_cast<void**>(&present_ptr),
                           reinterpret_cast<void*>(present_hook));
    if (ret != kiero::Status::Success) {
        throw std::runtime_error("Failed to inject dx12 present hook:  " + std::to_string(ret));
    }

    ret = kiero::bind(RESIZE_BUFFERS_FUNC_IDX, reinterpret_cast<void**>(&resize_buffers_ptr),
                      reinterpret_cast<void*>(resize_buffers_hook));
    if (ret != kiero::Status::Success) {
        throw std::runtime_error("Failed to inject dx12 resize buffers hook: "
                                 + std::to_string(ret));
    }

    ret = kiero::bind(EXEC_CMD_QUEUE_FUNC_IDX, reinterpret_cast<void**>(&exec_cmd_queue_ptr),
                      reinterpret_cast<void*>(exec_cmd_queue_hook));
    if (ret != kiero::Status::Success) {
        throw std::runtime_error("Failed to inject dx12 exec cmd queue hook:  "
                                 + std::to_string(ret));
    }
}

}  // namespace dhf::gui::dx12
