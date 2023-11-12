#ifndef SETTINGS_H
#define SETTINGS_H

#include "pch.h"

namespace dhf::settings {

/// The path to this dll.
extern const std::filesystem::path& dll_path;
/// True if the game we're injected into is BL3.
extern const bool& is_bl3;

/**
 * @brief Initalizes the settings module.
 *
 * @param this_module Handle to this dll's module, used to grab it's path.
 */
void init(HMODULE this_module);

}  // namespace dhf::settings

#endif /* SETTINGS_H */
