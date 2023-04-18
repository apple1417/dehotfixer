#ifndef TIME_TRAVEL_H
#define TIME_TRAVEL_H

#include "pch.h"

namespace dhf::time_travel {

// NOLINTNEXTLINE(readability-magic-numbers)
using ue_timespan = std::chrono::duration<int64_t, std::ratio<1, 10'000'000>>;

/// The current offset being added to all times.
extern const ue_timespan& time_offset;

/**
 * @brief Injects all code needed for time injection.
 * @note Not thread safe.
 */
void init(void);

/**
 * @brief Set the offset to add to all times.
 *
 * @param offset The offset to add.
 */
void set_time_offset(ue_timespan offset);

}  // namespace dhf::time_travel

#endif /* TIME_TRAVEL_H */
