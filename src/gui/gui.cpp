#include "pch.h"

#include "gui/gui.h"
#include "gui/hook.h"
#include "hfdat.h"
#include "hotfixes/processing.h"
#include "imgui.h"
#include "settings.h"
#include "time_travel.h"
#include "vault_cards.h"

using namespace std::chrono_literals;

namespace dhf::gui {

namespace {

bool settings_showing = true;

#pragma region Time Handling

// While vault cards existed before maurice vendors (2021-June-24), they're not really interesting
// Limiting to just the valid maurce date range helps avoid the 5mo where vendors would be invalid
const constexpr auto EARLIEST_MAURICE_DATE = 2021y / std::chrono::November / 18d;
const constexpr auto LAST_EVENT_DATE = 9999y / std::chrono::December / 31d;

const constexpr auto HOURS_IN_DAY = 24;
const constexpr auto MINUTES_IN_HOUR = 60;

struct EventTime {
    int32_t year;
    int32_t month;
    int32_t day;
    int32_t hour;
    int32_t minute;
} event_time;

/**
 * @brief Set the event time inputs to the current UTC time.
 */
void set_event_time_to_now(void) {
    auto now = std::chrono::system_clock::now();
    auto days = std::chrono::floor<std::chrono::days>(now);

    auto date =
        std::clamp(std::chrono::year_month_day{days}, EARLIEST_MAURICE_DATE, LAST_EVENT_DATE);

    std::chrono::hh_mm_ss time{std::chrono::floor<std::chrono::minutes>(now - days)};

    // It really annoys me how there are three different ways you get ints out of these things..
    event_time.year = (int)date.year();
    event_time.month = (int32_t)(unsigned)date.month();
    event_time.day = (int32_t)(unsigned)date.day();
    event_time.hour = time.hours().count();
    event_time.minute = time.minutes().count();
}

/**
 * @brief Clamps the event time inputs to valid values.
 */
void clamp_event_time(void) {
    // Firstly, clamp individual fields
    event_time.year =
        std::clamp(event_time.year, (int)EARLIEST_MAURICE_DATE.year(), (int)LAST_EVENT_DATE.year());
    event_time.month = std::clamp(event_time.month, (int32_t)(unsigned)std::chrono::January,
                                  (int32_t)(unsigned)std::chrono::December);

    auto yy_mm = std::chrono::year(event_time.year) / event_time.month;
    auto last_day = (yy_mm / std::chrono::last).day();
    event_time.day = std::clamp(event_time.day, 1, (int32_t)(unsigned)last_day);

    // Then clamp to within the actual date range
    auto clamped_date = std::clamp(yy_mm / event_time.day, EARLIEST_MAURICE_DATE, LAST_EVENT_DATE);
    event_time.year = (int)clamped_date.year();
    event_time.month = (int32_t)(unsigned)clamped_date.month();
    event_time.day = (int32_t)(unsigned)clamped_date.day();

    // Let time be whatever
    event_time.hour = std::clamp((int)event_time.hour, 0, HOURS_IN_DAY - 1);
    event_time.minute = std::clamp((int)event_time.minute, 0, MINUTES_IN_HOUR - 1);
}

/**
 * @brief Sets the time offset to the time currently in the event inputs.
 * @note Doesn't validate inputs, should be called after `clamp_event_time`.
 */
void apply_event_time(void) {
    auto wanted_time = (std::chrono::sys_days)(std::chrono::year(event_time.year) / event_time.month
                                               / event_time.day)
                       + std::chrono::hours(event_time.hour)
                       + std::chrono::minutes(event_time.minute);
    auto now = std::chrono::system_clock::now();

    time_travel::set_time_offset(wanted_time - now);
}

#pragma endregion

#pragma region Hotfix Handling

const constexpr auto NO_HOTFIXES = "No Hotfixes";
const constexpr auto CURRENT_HOTFIXES = "Current Hotfixes";

// Indexes > 0 refer to actual hotfixes
// Indexes < 0 count down through this default entries list - `storage_idx = -1 - default_idx`
const std::vector<std::string> DEFAULT_HOTFIX_LIST_ENTRIES{
    NO_HOTFIXES,
    CURRENT_HOTFIXES,
};
const constexpr auto NO_HOTFIXES_IDX = -1;
const constexpr auto CURRENT_HOTFIXES_IDX = -2;

int selected_hotfix_idx = CURRENT_HOTFIXES_IDX;

/**
 * @brief Get the display name of a hotfix.
 *
 * @param full_name The full hotfix name, including ordering chars.
 * @return A pointer to the display name. Only valid for the life of the full name.
 */
const char* get_hotfix_display_name(const std::string& full_name) {
    auto display_offset = full_name.find_first_of(';');
    if (display_offset == std::string::npos) {
        display_offset = 0;
    } else {
        display_offset++;
    }

    return full_name.c_str() + display_offset;
}

/**
 * @brief Updates the selected hotfix to the given index.
 *
 * @param idx The new index to select.
 */
void update_selected_hotfix(int idx) {
    selected_hotfix_idx = idx;

    auto name = idx < 0 ? DEFAULT_HOTFIX_LIST_ENTRIES[-1 - idx] : hfdat::hotfix_names[idx];
    auto type = idx == NO_HOTFIXES_IDX ? hfdat::LoadType::NONE
                                       : (idx == CURRENT_HOTFIXES_IDX ? hfdat::LoadType::CURRENT
                                                                      : hfdat::LoadType::FILE);

    hfdat::load_new_hotfixes(name, type);
}

#pragma endregion

#pragma region Status

/**
 * @brief Creates an ImGui colour vector from it's hex value.
 * @note Returns with full alpha.
 *
 * @param hex A 24-bit rgb value.
 * @return The corrosponding colour vector.
 */
consteval ImVec4 colour_from_hex(uint32_t hex) {
    // NOLINTBEGIN(readability-identifier-length, readability-magic-numbers)
    auto r = (hex >> 16) & 0xFF;
    auto g = (hex >> 8) & 0xFF;
    auto b = (hex >> 0) & 0xFF;
    return {static_cast<float>(r / 255.0), static_cast<float>(g / 255.0),
            static_cast<float>(b / 255.0), 1.0};
    // NOLINTEND(readability-identifier-length, readability-magic-numbers)
}

const constexpr ImVec4 ALL_COLOURS[] = {
    colour_from_hex(0xe6194b), colour_from_hex(0xf58231), colour_from_hex(0xffe119),
    colour_from_hex(0x3cb44b), colour_from_hex(0x42d4f4), colour_from_hex(0x4363d8),
    colour_from_hex(0xf032e6), colour_from_hex(0xffffff),
};

const constexpr auto B64_ALPHABET =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * @brief Base64 encodes a block of data.
 *
 * @param data The data to encode.
 * @param len The length of the data to encode.
 * @return The base64-encoded data
 */
std::string b64_encode(const uint8_t* data, size_t len) {
    std::stringstream encoded;
    /*
    01001101 01100001 01101110
    TTTTTTWW WWWWFFFF FFuuuuuu
    */
    size_t idx = 0;
    for (; idx < (len - 2); idx += 3) {
        // NOLINTBEGIN(readability-magic-numbers)
        // clang-format off
        encoded << B64_ALPHABET[                                ((data[idx + 0] & 0xFC) >> 2)];
        encoded << B64_ALPHABET[((data[idx + 0] & 0x03) << 4) | ((data[idx + 1] & 0xF0) >> 4)];
        encoded << B64_ALPHABET[((data[idx + 1] & 0x0F) << 2) | ((data[idx + 2] & 0xC0) >> 6)];
        encoded << B64_ALPHABET[((data[idx + 2] & 0x3F) << 0)                                ];
        // clang-format on
        // NOLINTEND(readability-magic-numbers)
    }

    if (idx == (len - 1)) {
        // NOLINTBEGIN(readability-magic-numbers)
        // clang-format off
        encoded << B64_ALPHABET[                                ((data[idx + 0] & 0xFC) >> 2)];
        encoded << B64_ALPHABET[((data[idx + 0] & 0x03) << 4)                                ];
        encoded << '=';
        encoded << '=';
        // clang-format on
        // NOLINTEND(readability-magic-numbers)
    } else if (idx == (len - 2)) {
        // NOLINTBEGIN(readability-magic-numbers)
        // clang-format off
        encoded << B64_ALPHABET[                                ((data[idx + 0] & 0xFC) >> 2)];
        encoded << B64_ALPHABET[((data[idx + 0] & 0x03) << 4) | ((data[idx + 1] & 0xF0) >> 4)];
        encoded << B64_ALPHABET[((data[idx + 1] & 0x0F) << 2)                                ];
        encoded << '=';
        // clang-format on
        // NOLINTEND(readability-magic-numbers)
    }

    return encoded.str();
}

/**
 * @brief Draws a window displaying the status of all our edits.
 */
void draw_status_window(void) {
    const constexpr auto background_alpha = 0.5;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {4, 4});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {2, 2});

    ImGui::SetNextWindowSize({0, 0});
    ImGui::SetNextWindowBgAlpha(background_alpha);

    ImGui::Begin("##status", nullptr,
                 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs
                     | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
                     | ImGuiWindowFlags_NoFocusOnAppearing
                     | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // Cache the last hotfix hash, primarily so that we can have it display as n/a until the game's
    // loaded them for the first time
    // The performance benefit is a nice side effect
    static std::string hotfix_hash = "n/a";
    static uint64_t last_hotfix_hash = 0;
    if (hotfixes::running_hotfix_hash != last_hotfix_hash) {
        hotfix_hash = b64_encode(reinterpret_cast<const uint8_t*>(&hotfixes::running_hotfix_hash),
                                 sizeof(hotfixes::running_hotfix_hash));
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    auto& hotfix_colour = ALL_COLOURS[hotfixes::running_hotfix_hash % IM_ARRAYSIZE(ALL_COLOURS)];

    ImGui::TextColored(hotfix_colour, "%s", hotfix_hash.c_str());
    ImGui::TextDisabled("%s", get_hotfix_display_name(hotfixes::running_hotfix_name));

    if (settings::is_bl3()) {
        auto injected_time = std::chrono::system_clock::now() + time_travel::time_offset;
        ImGui::TextDisabled("%s", std::format("{:%F %R}", injected_time).c_str());
    }

    ImGui::SetWindowPos({0, ImGui::GetMainViewport()->Size.y - ImGui::GetWindowHeight()});

    ImGui::End();
    ImGui::PopStyleVar(2);
}

#pragma endregion

/**
 * @brief Draws a section of a window holding all the settings to do with vault cards.
 */
void draw_vault_card_section(void) {
    ImGui::SeparatorText("Vault Cards");
    ImGui::Checkbox("Enabled", &vault_cards::enable);
}

/**
 * @brief Draws a section of a window holding all the settings to do with time travel.
 */
void draw_time_travel_section(void) {
    const constexpr auto event_time_input_width = 15;

    ImGui::SeparatorText("Event Time Travel");
    ImGui::Text("Time (UTC 24h)");

    ImGui::SetNextItemWidth(ImGui::GetFontSize() * event_time_input_width);
    ImGui::InputScalarN("##event time scalar", ImGuiDataType_S32,
                        reinterpret_cast<int32_t*>(&event_time),
                        sizeof(event_time) / sizeof(event_time.year), nullptr, nullptr, "%02d");
    static_assert(std::is_same_v<decltype(event_time.year), int32_t>);

    clamp_event_time();

    ImGui::SameLine();
    if (ImGui::Button("Apply")) {
        apply_event_time();
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        set_event_time_to_now();
        time_travel::set_time_offset(time_travel::ue_timespan::zero());
    }
}

/**
 * @brief Draws a section of a window holding all the settings to do with hotfixes.
 */
void draw_hotfix_section(void) {
    ImGui::SeparatorText("Hotfixes");

    static int highlighted_hotfix_idx = selected_hotfix_idx;

    if (ImGui::Button("Update Selection")) {
        update_selected_hotfix(highlighted_hotfix_idx);
    }

    // Right align
    ImGui::SameLine(ImGui::GetWindowSize().x - ImGui::CalcTextSize(hfdat::hfdat_name.c_str()).x
                    - ImGui::GetStyle().ItemSpacing.x);
    ImGui::TextDisabled("%s", hfdat::hfdat_name.c_str());

    ImGui::Text("%s", get_hotfix_display_name(hfdat::loaded_hotfixes_name));

    static ImGuiTextFilter filter;
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Filter");
    ImGui::SameLine();
    filter.Draw("##filter", -FLT_MIN);

    if (ImGui::BeginListBox("##hotfixlist", {-FLT_MIN, -FLT_MIN})) {
        bool double_click = false;

        for (auto i = 0; (size_t)i < DEFAULT_HOTFIX_LIST_ENTRIES.size(); i++) {
            auto name = get_hotfix_display_name(DEFAULT_HOTFIX_LIST_ENTRIES[i]);
            auto storage_idx = -1 - i;
            if (filter.PassFilter(name)) {
                if (ImGui::Selectable(name, highlighted_hotfix_idx == storage_idx,
                                      ImGuiSelectableFlags_AllowDoubleClick)) {
                    highlighted_hotfix_idx = storage_idx;
                    double_click = ImGui::IsMouseDoubleClicked(0);
                }
            }
        }

        for (auto i = 0; (size_t)i < hfdat::hotfix_names.size(); i++) {
            auto name = get_hotfix_display_name(hfdat::hotfix_names[i]);
            if (filter.PassFilter(name)) {
                if (ImGui::Selectable(name, highlighted_hotfix_idx == i,
                                      ImGuiSelectableFlags_AllowDoubleClick)) {
                    highlighted_hotfix_idx = i;
                    double_click = ImGui::IsMouseDoubleClicked(0);
                }
            }
        }

        if (double_click) {
            update_selected_hotfix(highlighted_hotfix_idx);
        }

        ImGui::EndListBox();
    }
}

}  // namespace

void render(void) {
    const constexpr auto default_settings_window_height_lines = 30;
    const constexpr auto default_settings_window_width = 320;

    draw_status_window();

    if (!settings_showing) {
        return;
    }

    ImGui::SetNextWindowSize(
        {default_settings_window_width,
         default_settings_window_height_lines * ImGui::GetTextLineHeightWithSpacing()},
        ImGuiCond_FirstUseEver);

    ImGui::Begin("dehotfixer (Ctrl+Shift+Ins)", &settings_showing, ImGuiWindowFlags_NoCollapse);
    if (settings::is_bl3()) {
        draw_vault_card_section();
        draw_time_travel_section();
    }

    draw_hotfix_section();

    ImGui::End();
}

void init(void) {
    hook();
    set_event_time_to_now();
    update_selected_hotfix(CURRENT_HOTFIXES_IDX);
}

bool is_showing(void) {
    return settings_showing;
}

void toggle_showing(void) {
    settings_showing = !settings_showing;

    if (settings_showing && time_travel::time_offset == time_travel::ue_timespan::zero()) {
        set_event_time_to_now();
    }
}

}  // namespace dhf::gui
