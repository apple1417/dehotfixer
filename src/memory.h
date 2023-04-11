#ifndef MEMORY_H
#define MEMORY_H

#include "pch.h"

namespace dhf::memory {

/**
 * @brief Struct holding information about a sigscan pattern.
 */
struct Pattern {
   public:
    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
    const uint8_t* bytes;
    const uint8_t* mask;
    const ptrdiff_t offset;
    const size_t size;
    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

    /**
     * @brief Construct a pattern from strings.
     *
     * @tparam n The length of the strings (should be picked up automatically).
     * @param bytes The bytes to match.
     * @param mask The mask over the bytes to match.
     * @param offset The constant offset to add to the found address.
     * @return A sigscan pattern.
     */
    template <size_t n>
    Pattern(const char (&bytes)[n], const char (&mask)[n], ptrdiff_t offset = 0)
        : bytes(reinterpret_cast<const uint8_t*>(bytes)),
          mask(reinterpret_cast<const uint8_t*>(mask)),
          offset(offset),
          size(n - 1) {}

    static_assert(sizeof(uint8_t) == sizeof(char), "uint8_t is different size to char");
};

/**
 * @brief Performs a sigscan.
 *
 * @tparam T The type to cast the result to.
 * @param pattern The pattern to search for.
 * @return The found location, or nullptr.
 */
uintptr_t sigscan(const Pattern& pattern);
template <typename T>
T sigscan(const Pattern& pattern) {
    return reinterpret_cast<T>(sigscan(pattern));
}

/**
 * @brief Unlocks a region of memory for full read/write access. Intended for hex edits.
 *
 * @param start The start of the range to unlock.
 * @param size The size of the range to unlock.
 */
void unlock_range(uintptr_t start, size_t size);
inline void unlock_range(uint8_t* start, size_t size) {
    unlock_range(reinterpret_cast<uintptr_t>(start), size);
}

}  // namespace dhf::memory

#endif /* MEMORY_H */
