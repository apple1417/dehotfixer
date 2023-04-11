#include "pch.h"

#include "memory.h"

namespace dhf::memory {

namespace {

/**
 * @brief Gets the address range covered by the exe's module.
 *
 * @return A tuple of the exe start address and it's length.
 */
std::tuple<uintptr_t, size_t> get_exe_range(void) {
    static std::optional<std::tuple<uintptr_t, size_t>> range = std::nullopt;
    if (range) {
        return *range;
    }

    HMODULE exe_module = GetModuleHandleA(nullptr);

    MEMORY_BASIC_INFORMATION mem;
    if (VirtualQuery(static_cast<LPCVOID>(exe_module), &mem, sizeof(mem)) == 0) {
        throw std::runtime_error("VirtualQuery failed!");
    }

    auto allocation_base = mem.AllocationBase;
    if (allocation_base == nullptr) {
        throw std::runtime_error("AllocationBase was NULL!");
    }

    auto dos_header = reinterpret_cast<IMAGE_DOS_HEADER*>(allocation_base);
    auto nt_header = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<uint8_t*>(allocation_base)
                                                         + dos_header->e_lfanew);
    auto module_length = nt_header->OptionalHeader.SizeOfImage;

    range = {reinterpret_cast<uintptr_t>(allocation_base), module_length};
    return *range;
}

}  // namespace

uintptr_t sigscan(const Pattern& pattern) {
    auto [start, size] = get_exe_range();
    auto start_ptr = reinterpret_cast<uint8_t*>(start);

    // The naive O(nm) search works well enough, even repeating it for each different pattern
    for (size_t i = 0; i < (size - pattern.size); i++) {
        bool found = true;
        for (size_t j = 0; j < pattern.size; j++) {
            auto val = start_ptr[i + j];
            if ((val & pattern.mask[j]) != pattern.bytes[j]) {
                found = false;
                break;
            }
        }
        if (found) {
            return reinterpret_cast<uintptr_t>(&start_ptr[i + pattern.offset]);
        }
    }

    return 0;
}

void unlock_range(uintptr_t start, size_t size) {
    DWORD old_protect = 0;
    if (VirtualProtect(reinterpret_cast<LPVOID>(start), size, PAGE_EXECUTE_READWRITE, &old_protect)
        == 0) {
        throw std::runtime_error("VirtualProtect failed!");
    }
}

}  // namespace dhf::memory
