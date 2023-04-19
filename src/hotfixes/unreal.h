#ifndef HOTFIXES_UNREAL_H
#define HOTFIXES_UNREAL_H

#include "pch.h"

namespace dhf::hotfixes {

template <typename T>
struct TArray {
    T* data;
    uint32_t count;
    uint32_t max;
};

struct FString : TArray<wchar_t> {
    /**
     * @brief Converts this string to a stl string.
     *
     * @return An stl string.
     */
    [[nodiscard]] std::wstring to_wstr(void) const;
};

struct FReferenceControllerBase {
    void* vf_table;
    int32_t ref_count;
    int32_t weak_ref_count;
    void* obj;
};

template <typename T>
struct TSharedPtr {
    T* obj;
    FReferenceControllerBase* ref_controller;
};

enum class EJson {
    NONE = 0,
    NULL_ = 1,  // NOLINT(readability-identifier-naming)
    STRING = 2,
    NUMBER = 3,
    BOOLEAN = 4,
    ARRAY = 5,
    OBJECT = 6
};

struct FJsonValue {
    void* vf_table;
    enum EJson type;

    /**
     * @brief Casts this object to a specific type.
     * @note Throws a runtime error if the type doesn't line up.
     *
     * @tparam T The type to cast to.
     * @return A pointer to this object casted to the relevant type.
     */
    template <typename T>
    T* cast(void);
};

template <typename K, typename V>
struct KeyValuePair {
    K key;
    V value;
    int32_t hash_next_id;
    int32_t hash_idx;
};

using JSONObjectEntry = KeyValuePair<FString, TSharedPtr<FJsonValue>>;

struct FJsonObject {
    static const constexpr auto PATTERN_SIZE = 16;

    TArray<JSONObjectEntry> entries;

    // Unknown what this data represents.
    uint32_t pattern[PATTERN_SIZE];

    /**
     * @brief Gets a value on this object given it's key.
     * @note Throws a runtime error if the key is not found.
     *
     * @tparam T The type to cast the value to.
     * @param key The key to look up.
     * @return A pointer to the value object.
     */
    template <typename T>
    T* get(const std::wstring& key) const;
};

struct FJsonValueString : FJsonValue {
    FString str;

    /**
     * @brief Converts this string to a stl string.
     *
     * @return An stl string.
     */
    [[nodiscard]] std::wstring to_wstr(void) const;
};

struct FJsonValueArray : FJsonValue {
    TArray<TSharedPtr<FJsonValue>> entries;

    /**
     * @brief Gets the amount of items in this array.
     *
     * @return The amount of items.
     */
    [[nodiscard]] uint32_t count(void) const;

    /**
     * @brief Gets an entry in the array given it's index.
     * @note May throw an out of range error.
     *
     * @tparam T The type to cast the value to.
     * @param idx The index to get.
     * @return A pointer to the value object.
     */
    template <typename T>
    T* get(uint32_t idx) const;
};

struct FJsonValueObject : FJsonValue {
    TSharedPtr<FJsonObject> value;

    /**
     * @brief Gets the object internal to this value.
     *
     * @return A pointer to the object.
     */
    [[nodiscard]] FJsonObject* to_obj(void) const;
};

}  // namespace dhf::hotfixes

#endif /* HOTFIXES_UNREAL_H */
