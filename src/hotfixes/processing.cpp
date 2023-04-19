#include "pch.h"

#include "hotfixes/hooks.h"
#include "hotfixes/processing.h"
#include "hotfixes/unreal.h"
#include "settings.h"
#include "version.h"

namespace dhf::hotfixes {

namespace {

const constexpr auto BL3_NEWS_FRAME = L"oakasset.frame.patchNote";
const constexpr auto WL_NEWS_FRAME = L"asset.nexus.HotFix";

/**
 * @brief Struct holding all the vf tables we need to grab copies of.
 */
struct VFTables {
    bool found;
    void* json_value_string;
    void* json_value_array;
    void* json_value_object;
    void* shared_ptr_json_object;
    void* shared_ptr_json_value;
} vf_table = {};

// Haven't managed to fully reverse engineer json objects, but their extra data is always constant
//  for objects with the same amount of entries, so as a hack we can just hardcode it

// clang-format off
const uint32_t KNOWN_OBJECT_PATTERNS[3][16] = {{
    // Objects of size 1
    0x00000001, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000080,
    0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000000,
}, {
    // Objects of size 2
    0x00000003, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000002, 0x00000080,
    0xFFFFFFFF, 0x00000000, 0x00000001, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000000,
}, {
    // Objects of size 3
    0x00000007, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000003, 0x00000080,
    0xFFFFFFFF, 0x00000000, 0x00000002, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000000,
}};
// clang-format on

static_assert(sizeof(KNOWN_OBJECT_PATTERNS[0]) == sizeof(FJsonObject::pattern));

/**
 * @brief Gathers all required vf table pointers and fills in the vf table struct.
 *
 * @param discovery_json The json object received by the discovery hook.
 */
void gather_vf_tables(const FJsonObject* discovery_json) {
    if (vf_table.found) {
        return;
    }

    auto services = discovery_json->get<FJsonValueArray>(L"services");
    vf_table.json_value_array = services->vf_table;
    vf_table.shared_ptr_json_value = services->entries.data[0].ref_controller->vf_table;

    auto first_service = services->get<FJsonValueObject>(0);
    vf_table.json_value_object = first_service->vf_table;
    vf_table.shared_ptr_json_object = first_service->value.ref_controller->vf_table;

    auto group = first_service->to_obj()->get<FJsonValueString>(L"configuration_group");
    vf_table.json_value_string = group->vf_table;

    vf_table.found = true;
}

/**
 * @brief Adds a reference controller to a shared pointer.
 * @note The object must already be set before calling this.
 *
 * @tparam T The type of the shared pointer.
 * @param ptr The shared pointer to edit.
 * @param vf_table The vf table for the shared pointer's type.
 */
template <typename T>
void add_ref_controller(TSharedPtr<T>* ptr, void* vf_table) {
    ptr->ref_controller = u_malloc<FReferenceControllerBase>(sizeof(FReferenceControllerBase));
    ptr->ref_controller->vf_table = vf_table;
    ptr->ref_controller->ref_count = 1;
    ptr->ref_controller->weak_ref_count = 1;
    ptr->ref_controller->obj = ptr->obj;
}

/**
 * @brief Allocated memory to set an FString to a given value.
 * @note Also sets the string count.
 *
 * @param str The FString to fill.
 * @param value The value to set.
 */
void alloc_string(FString* str, const std::wstring& value) {
    str->count = (uint32_t)value.size() + 1;
    str->max = str->count;
    str->data = u_malloc<wchar_t>(str->count * sizeof(wchar_t));
    wcsncpy_s(str->data, str->count, value.c_str(), str->count - 1);
}

/**
 * @brief Creates a json string object.
 *
 * @param value The value of the string.
 * @return A pointer to the new object.
 */
FJsonValueString* create_json_string(const std::wstring& value) {
    auto obj = u_malloc<FJsonValueString>(sizeof(FJsonValueString));
    obj->vf_table = vf_table.json_value_string;
    obj->type = EJson::STRING;
    alloc_string(&obj->str, value);

    return obj;
}

/**
 * @brief Creates a json object.
 *
 * @tparam n The amount of entries in the object.
 * @param entries Key-value pairs of the object's entries.
 * @return A pointer to the new object.
 */
template <uint8_t n>
FJsonObject* create_json_object(
    const std::array<std::pair<std::wstring, FJsonValue*>, n>& entries) {
    static_assert(0 < n && n <= ARRAYSIZE(KNOWN_OBJECT_PATTERNS));

    auto obj = u_malloc<FJsonObject>(sizeof(FJsonObject));
    memcpy(&obj->pattern[0], &KNOWN_OBJECT_PATTERNS[n - 1][0], sizeof(obj->pattern));

    obj->entries.count = n;
    obj->entries.max = n;
    obj->entries.data = u_malloc<JSONObjectEntry>(n * sizeof(JSONObjectEntry));

    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
    for (auto i = 0; i < n; i++) {
        obj->entries.data[i].hash_next_id = ((int32_t)i) - 1;

        alloc_string(&obj->entries.data[i].key, entries[i].first);

        obj->entries.data[i].value.obj = entries[i].second;
        add_ref_controller(&obj->entries.data[i].value, vf_table.shared_ptr_json_value);
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)

    return obj;
}

/**
 * @brief Create a json array object.
 *
 * @param entries The entries in the array.
 * @return A pointer to the new object
 */
template <uint8_t n>
FJsonValueArray* create_json_array(const std::array<FJsonValue*, n>& entries) {
    auto obj = u_malloc<FJsonValueArray>(sizeof(FJsonValueArray));
    obj->vf_table = vf_table.json_value_array;
    obj->type = EJson::ARRAY;

    obj->entries.count = n;
    obj->entries.max = n;
    obj->entries.data = u_malloc<TSharedPtr<FJsonValue>>(n * sizeof(TSharedPtr<FJsonValue>));

    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
    for (auto i = 0; i < n; i++) {
        obj->entries.data[i].obj = entries[i];
        add_ref_controller(&obj->entries.data[i], vf_table.shared_ptr_json_value);
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)

    return obj;
}

/**
 * @brief Create a json value object from a raw object.
 *
 * @param obj The object to create a value object of.
 * @return A pointer to the new value object.
 */
FJsonValueObject* create_json_value_object(FJsonObject* obj) {
    auto val_obj = u_malloc<FJsonValueObject>(sizeof(FJsonValueObject));
    val_obj->vf_table = vf_table.json_value_object;
    val_obj->type = EJson::OBJECT;

    val_obj->value.obj = obj;
    add_ref_controller(&val_obj->value, vf_table.shared_ptr_json_object);

    return val_obj;
}

/**
 * @brief Gets the current time in an iso8601-formatted string.
 *
 * @return The current time string.
 */
std::wstring get_current_time_str(void) {
    auto now = std::chrono::system_clock::now();
    return std::format(L"{:%FT%TZ}", now);
}

}  // namespace

struct Hotfix {
    std::wstring key;
    std::wstring value;
};

void handle_discovery_from_json(FJsonObject** json) {
    gather_vf_tables(*json);

    auto services = (*json)->get<FJsonValueArray>(L"services");

    FJsonObject* micropatch = nullptr;
    for (uint32_t i = 0; i < services->count(); i++) {
        auto service = services->get<FJsonValueObject>(i)->to_obj();
        if (service->get<FJsonValueString>(L"service_name")->to_wstr() == L"Micropatch") {
            micropatch = service;
            break;
        }
    }

    // This happens during the first verify call so don't throw
    if (micropatch == nullptr) {
        return;
    }

    if (!vf_table.found) {
        throw std::runtime_error("Didn't find vf tables in time!");
    }

    std::vector<Hotfix> hotfixes{};

    auto params = micropatch->get<FJsonValueArray>(L"parameters");
    params->entries.count = (uint32_t)hotfixes.size();
    if (params->entries.count > params->entries.max) {
        params->entries.max = params->entries.count;
        params->entries.data = u_realloc<TSharedPtr<FJsonValue>>(
            params->entries.data, params->entries.max * sizeof(TSharedPtr<FJsonValue>));
    }

    for (size_t i = 0; i < hotfixes.size(); i++) {
        const auto& [key, value] = hotfixes[i];

        auto hotfix_entry = create_json_object<2>(
            {{{L"key", create_json_string(key)}, {L"value", create_json_string(value)}}});

        params->entries.data[i].obj = create_json_value_object(hotfix_entry);
        add_ref_controller(&params->entries.data[i], vf_table.shared_ptr_json_value);
    }
}

void handle_news_from_json(FJsonObject** json) {
    if (!vf_table.found) {
        throw std::runtime_error("Didn't find vf tables in time!");
    }

    auto news_data = (*json)->get<FJsonValueArray>(L"data");
    news_data->entries.count = 1;
    if (news_data->entries.count > news_data->entries.max) {
        news_data->entries.max = news_data->entries.count;
        news_data->entries.data = u_realloc<TSharedPtr<FJsonValue>>(
            news_data->entries.data, news_data->entries.max * sizeof(TSharedPtr<FJsonValue>));
    }

    auto contents_obj =
        create_json_object<1>({{{L"header", create_json_string(L"" FULL_PROJECT_NAME)}}});
    auto contents_arr = create_json_array<1>({{create_json_value_object(contents_obj)}});

    auto image_meta_tag_obj =
        create_json_object<1>({{{L"tag", create_json_string(L"disc_img_game_sm")}}});
    auto image_tags_obj = create_json_object<2>(
        {{{L"meta_tag", create_json_value_object(image_meta_tag_obj)},
          {L"value", create_json_string(settings::is_bl3() ? BL3_NEWS_FRAME : WL_NEWS_FRAME)}}});

    auto tags_arr = create_json_array<1>({{create_json_value_object(image_tags_obj)}});

    auto availablities_obj =
        create_json_object<1>({{{L"startTime", create_json_string(get_current_time_str())}}});
    auto availabilities_arr = create_json_array<1>({{create_json_value_object(availablities_obj)}});

    auto news_obj = create_json_object<3>({{{L"contents", contents_arr},
                                            {L"article_tags", tags_arr},
                                            {L"availabilities", availabilities_arr}}});

    news_data->entries.data[0].obj = create_json_value_object(news_obj);
    add_ref_controller(&news_data->entries.data[0], vf_table.shared_ptr_json_value);
}

}  // namespace dhf::hotfixes
