#ifndef HFDAT_H
#define HFDAT_H

#include "pch.h"

namespace dhf::hfdat {

/// A list of all the loaded hotfix file names (including ordering chars).
extern const std::vector<std::string>& hotfix_names;

/// The name of the hfdat file hotfixes were loaded from.
extern const std::string& hfdat_name;

/// True if to use current hotfixes, rather than try overwriting
extern const bool& use_current_hotfixes;

/// The custom hotfixes to load
extern const std::vector<std::pair<std::wstring, std::wstring>>& hotfixes;

/// The name of the currently loaded hotfixes
extern const std::string& loaded_hotfixes_name;

/**
 * @brief Finds and loads the inital hotfix metadata.
 */
void init(void);

enum class LoadType {
    FILE,
    CURRENT,
    NONE,
};

/**
 * @brief Attempt to load a new hotfix file.
 *
 * @param name The full name of the hotfixes to load.
 * @param type What type of load to perform.
 */
void load_new_hotfixes(const std::string& name, LoadType type = LoadType::FILE);

}  // namespace dhf::hfdat

#endif /* HFDAT_H */
