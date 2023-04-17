#include "pch.h"

#include "gui/gui.h"
#include "hfdat.h"
#include "imgui.h"
#include "settings.h"
#include "vault_cards.h"

namespace dhf::gui {

namespace {

const constexpr auto NO_HOTFIXES = "No Hotfixes";
const constexpr auto CURRENT_HOTFIXES = "Current Hotfixes";
const constexpr auto NO_HOTFIXES_IDX = -2;
const constexpr auto CURRENT_HOTFIXES_IDX = -1;

int selected_hotfix_idx = CURRENT_HOTFIXES_IDX;

struct EventTime {
    int year;
    int month;
    int day;
    int hour;
    int minute;
} event_time;

void set_event_time_to_now(void) {
    std::time_t now{};
    std::time(&now);
    std::tm time{};
    gmtime_s(&time, &now);

    // NOLINTNEXTLINE(readability-magic-numbers)
    event_time.year = time.tm_year + 1900;
    event_time.month = time.tm_mon;
    event_time.day = time.tm_mday;
    event_time.hour = time.tm_hour;
    event_time.minute = time.tm_min;
}

void validate_event_time(void) {
    // TODO: instead on open check if ever applied
    static bool inital = true;
    if (inital) {
        inital = false;
        set_event_time_to_now();
    }

    // TODO: try out better c++20 conversions?
    // NOLINTBEGIN(readability-magic-numbers)
    event_time.year = std::clamp(event_time.year, 0, 9999);
    event_time.month = std::clamp(event_time.month, 1, 12);
    event_time.day = std::clamp(event_time.day, 1, 31);  // TODO
    event_time.hour = std::clamp(event_time.hour, 0, 23);
    event_time.minute = std::clamp(event_time.minute, 0, 59);
    // NOLINTEND(readability-magic-numbers)
}

}  // namespace

void render(void) {
    ImGui::ShowDemoWindow();

    // NOLINTNEXTLINE(readability-magic-numbers)
    ImGui::SetNextWindowSize({0, 30 * ImGui::GetTextLineHeightWithSpacing()},
                             ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("dehotfixer")) {
        ImGui::End();
        return;
    }

    if (settings::is_bl3()) {
        ImGui::SeparatorText("Vault Cards");
        ImGui::Checkbox("Enabled", &vault_cards::enable);

        ImGui::SeparatorText("Event Time Travel");
        ImGui::Text("Time (UTC 24h)");

        // NOLINTNEXTLINE(readability-magic-numbers)
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 15);
        ImGui::InputScalarN("##event time scalar", ImGuiDataType_S32,
                            reinterpret_cast<int*>(&event_time), sizeof(event_time) / sizeof(int),
                            nullptr, nullptr, "%d");

        ImGui::SameLine();
        if (ImGui::Button("Now")) {
            set_event_time_to_now();
        }
        ImGui::SameLine();
        ImGui::Button("Apply##time");  // TODO

        validate_event_time();
    }

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

    ImGui::End();
}

}  // namespace dhf::gui
