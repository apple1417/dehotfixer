#include "pch.h"

#include "hotfixes/processing.h"
#include "hotfixes/unreal.h"
#include "memory.h"

using namespace dhf::memory;

namespace dhf::hotfixes {

namespace {

const constexpr auto MALLOC_ALIGNMENT = 8;

const Pattern MALLOC_PATTERN{
    "\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xF9\x8B\xDA\x48\x8B\x0D\x00\x00\x00\x00\x48"
    "\x85\xC9",
    "\xFF\xFF\xFF\xFF\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\x00\x00\x00\xFF"
    "\xFF\xFF"};

using malloc_func = void* (*)(size_t, uint32_t);
malloc_func malloc_ptr;

}  // namespace

void* u_malloc(size_t count) {
    auto ret = malloc_ptr(count, MALLOC_ALIGNMENT);
    if (ret == nullptr) {
        throw std::runtime_error("Failed to allocate memory!");
    }
    memset(ret, 0, count);
    return ret;
}

namespace {

const Pattern REALLOC_PATTERN{
    "\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xF1\x41\x8B\xD8\x48\x8B"
    "\x0D\x00\x00\x00\x00\x48\x8B\xFA",
    "\xFF\xFF\xFF\xFF\x00\xFF\xFF\xFF\xFF\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
    "\xFF\x00\x00\x00\x00\xFF\xFF\xFF"};

using realloc_func = void* (*)(void*, size_t, uint32_t);
realloc_func realloc_ptr;

}  // namespace

void* u_realloc(void* original, size_t count) {
    auto ret = realloc_ptr(original, count, MALLOC_ALIGNMENT);
    if (ret == nullptr) {
        throw std::runtime_error("Failed to re-allocate memory!");
    }
    return ret;
}

namespace {

const Pattern FREE_PATTERN{
    "\x48\x85\xC9\x74\x00\x53\x48\x83\xEC\x20\x48\x8B\xD9\x48\x8B\x0D\x00\x00\x00\x00",
    "\xFF\xFF\xFF\xFF\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\x00\x00\x00"};

using free_func = void (*)(void*);
free_func free_ptr;

}  // namespace

void u_free(void* data) {
    free_ptr(data);
}

namespace {

const Pattern DISCOVERY_PATTERN{
    "\x40\x55\x53\x57\x48\x8D\x6C\x24\x00\x48\x81\xEC\x90\x00\x00\x00\x48\x83\x3A\x00\x48\x8B\xDA"
    "\x48\x8B\xF9\x75\x00\x32\xC0\x48\x81\xC4\x90\x00\x00\x00\x5F\x5B\x5D\xC3\x4C\x89\xBC\x24\x00"
    "\x00\x00\x00",
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
    "\xFF\xFF\xFF\xFF\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00"
    "\x00\x00\x00"};

using discovery_from_json_func = bool (*)(void*, FJsonObject**);
discovery_from_json_func original_discovery_from_json_ptr = nullptr;

/**
 * @brief Detour function for `GbxSparkSdk::Discovery::Services::FromJson`.
 *
 * @param this_service The service object this was called on.
 * @param json Unreal json objects containing the received data.
 * @return ¯\_(ツ)_/¯
 */
bool discovery_from_json_hook(void* this_service, FJsonObject** json) {
    try {
        handle_discovery_from_json(json);
    } catch (std::exception& ex) {
        std::cerr << "[dhf] Exception occured in discovery hook: " << ex.what() << "\n";
    }

    return original_discovery_from_json_ptr(this_service, json);
}

const Pattern NEWS_PATTERN{
    "\x40\x55\x53\x57\x48\x8D\x6C\x24\x00\x48\x81\xEC\x90\x00\x00\x00\x48\x83\x3A\x00\x48\x8B\xDA"
    "\x48\x8B\xF9\x75\x00\x32\xC0\x48\x81\xC4\x90\x00\x00\x00\x5F\x5B\x5D\xC3\x48\x89\xB4\x24\x00"
    "\x00\x00\x00",
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
    "\xFF\xFF\xFF\xFF\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00"
    "\x00\x00\x00"};

using news_from_json_func = bool (*)(void*, FJsonObject**);
news_from_json_func original_news_from_json_ptr = nullptr;

/**
 * @brief Detour function for `GbxSparkSdk::News::NewsResponse::FromJson`.
 *
 * @param this_service The service object this was called on.
 * @param json Unreal json objects containing the received data.
 * @return ¯\_(ツ)_/¯
 */
bool news_from_json_hook(void* this_service, FJsonObject** json) {
    try {
        handle_news_from_json(json);
    } catch (std::exception& ex) {
        std::cerr << "[dhf] Exception occured in discovery hook: " << ex.what() << "\n";
    }

    return original_news_from_json_ptr(this_service, json);
}

}  // namespace

void init(void) {
    malloc_ptr = sigscan<malloc_func>(MALLOC_PATTERN);
    realloc_ptr = sigscan<realloc_func>(REALLOC_PATTERN);
    free_ptr = sigscan<free_func>(FREE_PATTERN);

    auto discovery = sigscan(DISCOVERY_PATTERN);
    auto ret = MH_CreateHook(reinterpret_cast<LPVOID>(discovery),
                             reinterpret_cast<LPVOID>(&discovery_from_json_hook),
                             reinterpret_cast<LPVOID*>(&original_discovery_from_json_ptr));
    if (ret != MH_OK) {
        throw std::runtime_error("MH_CreateHook failed " + std::to_string(ret));
    }
    ret = MH_EnableHook(reinterpret_cast<LPVOID>(discovery));
    if (ret != MH_OK) {
        throw std::runtime_error("MH_EnableHook failed " + std::to_string(ret));
    }

    auto news = sigscan(NEWS_PATTERN);
    ret = MH_CreateHook(reinterpret_cast<LPVOID>(news),
                        reinterpret_cast<LPVOID>(&news_from_json_hook),
                        reinterpret_cast<LPVOID*>(&original_news_from_json_ptr));
    if (ret != MH_OK) {
        throw std::runtime_error("MH_CreateHook failed " + std::to_string(ret));
    }
    ret = MH_EnableHook(reinterpret_cast<LPVOID>(news));
    if (ret != MH_OK) {
        throw std::runtime_error("MH_EnableHook failed " + std::to_string(ret));
    }
}

}  // namespace dhf::hotfixes
