#ifndef HFDAT_H
#define HFDAT_H

#include "pch.h"

namespace dhf::hfdat {

/// A list of all the loaded hotfix file names (including ordering chars).
extern const std::vector<std::string>& hotfix_names;

/// The name of the hfdat file hotfixes were loaded from.
extern const std::string& hfdat_name;

/**
 * @brief Finds and loads the hotfix data file.
 */
void init(void);

}  // namespace dhf::hfdat

#endif /* HFDAT_H */
