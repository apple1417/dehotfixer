#ifndef PCH_H
#define PCH_H

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <MinHook.h>

#ifdef __cplusplus

#include <chrono>
#include <cstdint>
#include <optional>
#include <ratio>
#include <tuple>
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

#endif /* PCH_H */
