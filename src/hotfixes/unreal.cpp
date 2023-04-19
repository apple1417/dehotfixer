#include "pch.h"

#include "hotfixes/unreal.h"

namespace dhf::hotfixes {

#pragma region Casting

template <typename T>
struct JTypeMapping {
    using type = T;
    static inline const EJson ENUM_TYPE{};
};

template <>
struct JTypeMapping<FJsonValueString> {
    using type = FJsonValueString;
    static inline const EJson ENUM_TYPE = EJson::STRING;
};

template <>
struct JTypeMapping<FJsonValueArray> {
    using type = FJsonValueArray;
    static inline const EJson ENUM_TYPE = EJson::ARRAY;
};

template <>
struct JTypeMapping<FJsonValueObject> {
    using type = FJsonValueObject;
    static inline const EJson ENUM_TYPE = EJson::OBJECT;
};

template <typename T>
T* FJsonValue::cast(void) {
    if (this->type != JTypeMapping<T>::ENUM_TYPE) {
        throw std::runtime_error("JSON object was of unexpected type "
                                 + std::to_string((uint32_t)this->type));
    }
    return reinterpret_cast<T*>(this);
}

#pragma endregion

#pragma region Accessors

std::wstring FString::to_wstr(void) const {
    return {this->data};
}

std::wstring FJsonValueString::to_wstr(void) const {
    return this->str.to_wstr();
}

uint32_t FJsonValueArray::count() const {
    return this->entries.count;
}

template <typename T>
T* FJsonValueArray::get(uint32_t idx) const {
    if (idx > this->count()) {
        throw std::out_of_range("Array index out of range");
    }
    return this->entries.data[idx].obj->cast<T>();
}

template <typename T>
T* FJsonObject::get(const std::wstring& key) const {
    for (uint32_t i = 0; i < this->entries.count; i++) {
        auto entry = this->entries.data[i];
        if (entry.key.to_wstr() == key) {
            return entry.value.obj->cast<T>();
        }
    }
    throw std::runtime_error("Couldn't find key!");
}

FJsonObject* FJsonValueObject::to_obj(void) const {
    return this->value.obj;
}

#pragma endregion

#pragma region Explict Template Instantiation

template FJsonValueString* FJsonValue::cast(void);
template FJsonValueArray* FJsonValue::cast(void);
template FJsonValueObject* FJsonValue::cast(void);

template FJsonValueString* FJsonValueArray::get(uint32_t) const;
template FJsonValueArray* FJsonValueArray::get(uint32_t) const;
template FJsonValueObject* FJsonValueArray::get(uint32_t) const;

template FJsonValueString* FJsonObject::get(const std::wstring& key) const;
template FJsonValueArray* FJsonObject::get(const std::wstring& key) const;
template FJsonValueObject* FJsonObject::get(const std::wstring& key) const;

#pragma endregion

}  // namespace dhf::hotfixes
