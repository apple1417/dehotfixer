#ifndef PCH_H
#define PCH_H

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <TlHelp32.h>

#include <MinHook.h>

#include <archive.h>
#include <archive_entry.h>

#ifdef __cplusplus

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

// NOLINTNEXTLINE(readability-identifier-naming)
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND h_wnd,
                                                             UINT u_msg,
                                                             WPARAM w_param,
                                                             LPARAM l_param);

#include <kiero.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <ratio>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <vector>

using std::int16_t;
using std::int32_t;
using std::int64_t;
using std::int8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::uint8_t;

#endif

#if defined(_MSC_VER)
#define DLL_EXPORT extern "C" __declspec(dllexport)
#elif defined(__clang__)
#define DLL_EXPORT extern "C" [[gnu::dllexport]]
#else
#error Unknown DLL export attribute
#endif

#endif /* PCH_H */
