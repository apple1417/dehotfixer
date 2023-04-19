#include "pch.h"

#include "gui/gui.h"
#include "gui/hook.h"
#include "hfdat.h"
#include "imgui.h"
#include "settings.h"
#include "time_travel.h"
#include "vault_cards.h"

using namespace std::chrono_literals;

namespace dhf::gui {

namespace {

bool settings_showing = true;

const constexpr auto NO_HOTFIXES = "No Hotfixes";
const constexpr auto CURRENT_HOTFIXES = "Current Hotfixes";
const constexpr auto NO_HOTFIXES_IDX = -2;
const constexpr auto CURRENT_HOTFIXES_IDX = -1;

int selected_hotfix_idx = CURRENT_HOTFIXES_IDX;

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

    ImGui::TextColored(ImVec4(1.0, 1.0, 0.0, 1.0), "hash");
    ImGui::TextDisabled("hotfix name");

    if (settings::is_bl3()) {
        auto injected_time = std::chrono::system_clock::now() + time_travel::time_offset;
        ImGui::TextDisabled("%s", std::format("{:%F %R}", injected_time).c_str());
    }

    ImGui::SetWindowPos({0, ImGui::GetMainViewport()->Size.y - ImGui::GetWindowHeight()});

    ImGui::End();
    ImGui::PopStyleVar(2);
}

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
    if (ImGui::Button("Apply##time travel")) {
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

    ImGui::Button("Apply##hotfix");  // TODO

    // Right align
    ImGui::SameLine(ImGui::GetWindowSize().x - ImGui::CalcTextSize(hfdat::hfdat_name.c_str()).x
                    - ImGui::GetStyle().ItemSpacing.x);
    ImGui::TextDisabled("%s", hfdat::hfdat_name.c_str());

    if (ImGui::BeginListBox("##hotfixlist", ImVec2(-FLT_MIN, -FLT_MIN))) {
        if (ImGui::Selectable(NO_HOTFIXES, selected_hotfix_idx == NO_HOTFIXES_IDX)) {
            selected_hotfix_idx = NO_HOTFIXES_IDX;
        }
        if (ImGui::Selectable(CURRENT_HOTFIXES, selected_hotfix_idx == CURRENT_HOTFIXES_IDX)) {
            selected_hotfix_idx = CURRENT_HOTFIXES_IDX;
        }

        for (auto i = 0; (size_t)i < hfdat::hotfix_names.size(); i++) {
            auto& hotfix = hfdat::hotfix_names[i];
            auto display_offset = hotfix.find_first_of(';');
            if (display_offset == std::string::npos) {
                display_offset = 0;
            } else {
                display_offset++;
            }

            if (ImGui::Selectable(hotfix.c_str() + display_offset, selected_hotfix_idx == i)) {
                selected_hotfix_idx = i;
            }
        }

        ImGui::EndListBox();
    }
}

}  // namespace

void render(void) {
    const constexpr auto default_settings_window_height = 30;

    ImGui::ShowDemoWindow();

    draw_status_window();

    if (!settings_showing) {
        return;
    }

    ImGui::SetNextWindowSize(
        {0, default_settings_window_height * ImGui::GetTextLineHeightWithSpacing()},
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
