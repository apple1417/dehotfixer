#ifndef GUI_GUI_H
#define GUI_GUI_H

namespace dhf::gui {

/**
 * @brief Initalizes the gui.
 * @note Includes a short blocking delay (from hooking), best done last.
 */
void init(void);

/**
 * @brief Renders our gui.
 */
void render(void);

/**
 * @brief Gets if we're currently showing the settings gui.
 * @note The status window in the corner is always shown.
 *
 * @return True if the settings gui is showing.
 */
[[nodiscard]] bool is_showing(void);

/**
 * @brief Toggles if the settings gui is showing.
 */
void toggle_showing(void);

}  // namespace dhf::gui

#endif /* GUI_GUI_H */
