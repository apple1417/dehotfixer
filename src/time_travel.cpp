#include "pch.h"

#include "memory.h"
#include "time_travel.h"

using namespace std::chrono_literals;
using namespace dhf::memory;

namespace dhf::time_travel {

namespace {

/*
Gearbox, in their infinite wisdom, created 4 basically different identical functions for the various
things which seed based on date.

All the functions start by subtracting 17 hours from utc, aproximately doing:
```
timespan = FTimespan::Assign(17 hours)
date = FDateTime::UtcNow()
date -= timespan
```

Due to how stuff got optimized, there are actually 6 locations this offset is created, hence the 6
injections later on.

We edit this offset when injecting custom times, so that they will continue ticking forwards. If you
inject to just before one of the times it ticks over, waiting should update all the events.

Unfortuantly, we can't easily edit the args to `FTimespan::Assign` without a detour. Instead, we
actually wipe out the entire call, and replace it with just setting the value on the stack to a
constant directly. We can then replace the constant to adjust what time we appear to be.
*/

const ue_timespan DEFAULT_OFFSET = 17h;
ue_timespan stored_offset;

// clang-format off
const uint8_t SHELLCODE[] = {
    // mov rcx,0000008E7E0AE800     (17 hours)
    0x48, 0xB9, 0x00, 0xE8, 0x0A, 0x7E, 0x8E, 0x00, 0x00, 0x00,
    // mov [rsp+000000FF],rcx       (the FF is replaced with a different offset based on signature)
    0x48, 0x89, 0x8C, 0x24, 0xFF, 0x00, 0x00, 0x00
};
// clang-format on

const auto SHELLCODE_OFFSET_OFFSET = 2;
const auto SHELLCODE_STACK_OFFSET = 14;

#pragma region Signatures

// UGameplayGlobals::GenerateCurrentWeekSeed
const Pattern GAMEPLAY_GLOBALS = {
    "\x40\x53\x48\x83\xEC\x60\x33\xDB\x48\x8D\x4C\x24\x00\x89\x5C\x24\x00",
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\xFF\xFF\xFF\x00", 0x20};
const auto GAMEPLAY_GLOBALS_SIZE = 27;
const auto GAMEPLAY_GLOBALS_STACK_OFFSET = 0x40;

// FOakPatchHelper::GenerateCurrentWeekSeed
const Pattern PATCH_HELPER = {"\x40\x53\x48\x83\xEC\x60\x33\xDB\x8B\xD1",
                              "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 0x22};
const auto PATCH_HELPER_SIZE = 30;
const auto PATCH_HELPER_STACK_OFFSET = 0x88;

// FVaultCardManager::GenerateCurrentDaySeed
const Pattern VAULT_CARD_DAY = {
    "\x83\xC0\xFD\x83\xF8\x08\x77\x00\x45\x33\xC9\x89\x5C\x24\x00\x33\xD2\x89\x5C\x24\x00\x48\x8D"
    "\x4C\x24\x00\x45\x8D\x41\x00\xE8\x00\x00\x00\x00\x48\x8B\x44\x24\x00\x48\x01\x84\x24\x00\x00"
    "\x00\x00\x89\x5C\x24\x00\x48\x8D\x4C\x24\x00\x41\xB9\x12\x00\x00\x00\x89\x5C\x24\x00\x89\x5C"
    "\x24\x00\xBA\xCF\x07\x00\x00\xC7\x44\x24\x00\x0C\x00\x00\x00",
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\xFF\xFF\xFF\xFF\xFF\xFF\x00\xFF\xFF\xFF\xFF\xFF\x00\xFF\xFF"
    "\xFF\xFF\x00\xFF\xFF\xFF\x00\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF\x00\xFF\xFF\xFF\xFF\x00\x00"
    "\x00\x00\xFF\xFF\xFF\x00\xFF\xFF\xFF\xFF\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\xFF\xFF"
    "\xFF\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\xFF\xFF\xFF\xFF",
    -0xA6};

const auto VAULT_CARD_DAY_P1_OFFSET = 0x26;
const auto VAULT_CARD_DAY_P1_SIZE = 27;
const auto VAULT_CARD_DAY_P1_STACK_OFFSET = 0x48;

const auto VAULT_CARD_DAY_P2_OFFSET = 0x5F;
const auto VAULT_CARD_DAY_P2_SIZE = 27;
const auto VAULT_CARD_DAY_P2_STACK_OFFSET = 0x48;

// FVaultCardManager::GenerateCurrentWeekSeed
const Pattern VAULT_CARD_WEEK = {
    "\x83\xC0\xFD\x83\xF8\x08\x77\x00\x45\x33\xC9\x89\x5C\x24\x00\x33\xD2\x89\x5C\x24\x00\x48\x8D"
    "\x4C\x24\x00\x45\x8D\x41\x00\xE8\x00\x00\x00\x00\x48\x8B\x44\x24\x00\x48\x01\x84\x24\x00\x00"
    "\x00\x00\x89\x5C\x24\x00\x48\x8D\x4C\x24\x00\x41\xB9\x12\x00\x00\x00\x89\x5C\x24\x00\x89\x5C"
    "\x24\x00\xBA\xCF\x07\x00\x00\x89\x5C\x24\x00",
    "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\xFF\xFF\xFF\xFF\xFF\xFF\x00\xFF\xFF\xFF\xFF\xFF\x00\xFF\xFF"
    "\xFF\xFF\x00\xFF\xFF\xFF\x00\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF\x00\xFF\xFF\xFF\xFF\x00\x00"
    "\x00\x00\xFF\xFF\xFF\x00\xFF\xFF\xFF\xFF\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\xFF\xFF"
    "\xFF\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00",
    -0xA6};

const auto VAULT_CARD_WEEK_P1_OFFSET = 0x26;
const auto VAULT_CARD_WEEK_P1_SIZE = 27;
const auto VAULT_CARD_WEEK_P1_STACK_OFFSET = 0x48;

const auto VAULT_CARD_WEEK_P2_OFFSET = 0x5F;
const auto VAULT_CARD_WEEK_P2_SIZE = 27;
const auto VAULT_CARD_WEEK_P2_STACK_OFFSET = 0x48;

#pragma endregion

const auto NOP = 0x90;

std::vector<int64_t*> injected_offsets{};

/**
 * @brief Injects the custom time offset shellcode into a function, and stores it's offset pointer
 *        in the global list.
 * @note Not thread safe.
 *
 * @param loc The location to inject to.
 * @param size The size of region we're injecting to. Excess will be nop'd.
 * @param stack_offset The offset in the stack the `FTimespan` is stored.
 */
void inject_shellcode(uintptr_t loc, size_t size, int32_t stack_offset) {
    auto loc_ptr = reinterpret_cast<uint8_t*>(loc);
    unlock_range(loc, size);

    // Doing this safely would be a bit of a pain
    // Since we know we're only injecting during startup, and these functions are only called once
    // we start loading into a map, we can be reasonably sure we won't run into issues though :)
    memcpy(loc_ptr, &SHELLCODE[0], sizeof(SHELLCODE));
    *reinterpret_cast<int32_t*>(loc_ptr + SHELLCODE_STACK_OFFSET) = stack_offset;

    memset(loc_ptr + sizeof(SHELLCODE), NOP, size - sizeof(SHELLCODE));

    injected_offsets.push_back(reinterpret_cast<int64_t*>(loc + SHELLCODE_OFFSET_OFFSET));
}

}  // namespace

const ue_timespan& time_offset = stored_offset;

void init(void) {
    auto gameplay_globals = sigscan(GAMEPLAY_GLOBALS);
    if (gameplay_globals == 0) {
        throw std::runtime_error(
            "Couldn't find signature for UGameplayGlobals::GenerateCurrentWeekSeed");
    }
    inject_shellcode(gameplay_globals, GAMEPLAY_GLOBALS_SIZE, GAMEPLAY_GLOBALS_STACK_OFFSET);

    auto patch_helper = sigscan(PATCH_HELPER);
    if (patch_helper == 0) {
        throw std::runtime_error(
            "Couldn't find signature for FOakPatchHelper::GenerateCurrentWeekSeed");
    }
    inject_shellcode(patch_helper, PATCH_HELPER_SIZE, PATCH_HELPER_STACK_OFFSET);

    auto vault_card_day = sigscan(VAULT_CARD_DAY);
    if (vault_card_day == 0) {
        throw std::runtime_error(
            "Couldn't find signature for FVaultCardManager::GenerateCurrentDaySeed");
    }
    inject_shellcode(vault_card_day + VAULT_CARD_DAY_P1_OFFSET, VAULT_CARD_DAY_P1_SIZE,
                     VAULT_CARD_DAY_P1_STACK_OFFSET);
    inject_shellcode(vault_card_day + VAULT_CARD_DAY_P2_OFFSET, VAULT_CARD_DAY_P2_SIZE,
                     VAULT_CARD_DAY_P2_STACK_OFFSET);

    auto vault_card_week = sigscan(VAULT_CARD_WEEK);
    if (vault_card_week == 0) {
        throw std::runtime_error(
            "Couldn't find signature for FVaultCardManager::GenerateCurrentWeekSeed");
    }
    inject_shellcode(vault_card_week + VAULT_CARD_WEEK_P1_OFFSET, VAULT_CARD_WEEK_P1_SIZE,
                     VAULT_CARD_WEEK_P1_STACK_OFFSET);
    inject_shellcode(vault_card_week + VAULT_CARD_WEEK_P2_OFFSET, VAULT_CARD_WEEK_P2_SIZE,
                     VAULT_CARD_WEEK_P2_STACK_OFFSET);
}

void set_time_offset(ue_timespan offset) {
    stored_offset = offset;

    // We need to negate the requested offset because it's subtracted from the current utc time
    auto ticks = ((-offset) + DEFAULT_OFFSET).count();

    for (const auto ptr : injected_offsets) {
        // Each indiviudal write here is thread safe, they should all be a single atomic instruction
        // The overall write to all offsets however is not
        // Chosing to ignore this cause dealing with it is a pain, it doesn't cause any issues
        // beyond just using an old value, and are users really going to run into it? :)
        *ptr = ticks;
    }
}

}  // namespace dhf::time_travel
