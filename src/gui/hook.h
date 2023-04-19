#ifndef GUI_HOOK_H
#define GUI_HOOK_H

namespace dhf::gui {

/**
 * @brief Hooks the relevant graphics api to inject our gui.
 * @note Includes a short blocking delay, best done last.
 */
void hook(void);

/**
 * @brief Sets up the `WinProc` keyboard/mouse hooks.
 * @note Called by graphics api-specific initalization, not intended to be used externally.
 *
 * @param h_wnd Handle to the window to hook.
 * @return True if hooked successfully.
 */
bool hook_keys(HWND h_wnd);

}  // namespace dhf::gui

#endif /* GUI_HOOK_H */
