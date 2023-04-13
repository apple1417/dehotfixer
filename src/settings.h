#ifndef SETTINGS_H
#define SETTINGS_H

#include "pch.h"

namespace dhf::settings {

/**
 * @brief Initalizes the settings module.
 *
 * @param this_module Handle to this dll's module, used to grab it's path.
 */
void init(HMODULE this_module);

/**
 * @brief Checks if the game we're injected into is BL3.
 *
 * @return True if we're running in BL3.
 */
bool is_bl3(void);

}  // namespace dhf::settings

#endif /* SETTINGS_H */
