#ifndef HOTFIXES_HOOKS_H
#define HOTFIXES_HOOKS_H

namespace dhf::hotfixes {

/**
 * @brief Hooks all functions required to inject hotfixes.
 */
void init(void);

/**
 * @brief Calls unreal's malloc function.
 *
 * @tparam T The type to cast the return to.
 * @param len The amount of bytes to allocate.
 * @return A pointer to the allocated memory.
 */
[[nodiscard]] void* u_malloc(size_t len);
template <typename T>
[[nodiscard]] T* u_malloc(size_t len) {
    return reinterpret_cast<T*>(u_malloc(len));
}

/**
 * @brief Calls unreal's realloc function.
 *
 * @tparam T The type to cast the return to.
 * @param original The original memory to re-allocate.
 * @param len The amount of bytes to allocate.
 * @return A pointer to the re-allocated memory.
 */
[[nodiscard]] void* u_realloc(void* original, size_t len);
template <typename T>
[[nodiscard]] T* u_realloc(void* original, size_t len) {
    return reinterpret_cast<T*>(u_realloc(original, len));
}

/**
 * @brief Calls unreal's free function.
 *
 * @param data The memory to free.
 */
void u_free(void* data);

}  // namespace dhf::hotfixes

#endif /* HOTFIXES_HOOKS_H */
