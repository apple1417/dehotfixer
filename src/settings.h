#ifndef SETTINGS_H
#define SETTINGS_H

#include "pch.h"

namespace dhf::settings {

/// The path to this dll.
extern const std::filesystem::path& dll_path;
/// True if to create a debug console window
extern const bool& debug_console;
/// True if the game we're injected into is BL3.
extern const bool& is_bl3;
/// True if we believe we were loaded via a plugin loader, rather than by the game directly.
extern const bool& loaded_via_plugin_loader;

/**
 * @brief Initalizes the settings module.
 *
 * @param this_module Handle to this dll's module, used to grab it's path.
 */
void init(HMODULE this_module);

}  // namespace dhf::settings

#endif /* SETTINGS_H */
