#include "pch.h"

#include "memory.h"
#include "vault_cards.h"

using namespace dhf::memory;

namespace dhf::vault_cards {

bool enable = false;

namespace {

const Pattern REFRESH_CHALLENGE_LIST_SIG{
    "\x48\x8B\xC4\x48\x89\x48\x00\x55\x48\x8D\x68\x00\x48\x81\xEC\x00\x01\x00\x00",
    "\xFF\xFF\xFF\xFF\xFF\xFF\x00\xFF\xFF\xFF\xFF\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF"};

const constexpr auto CLEAR_DAILY_CHALLENGES_OFFSET = 0x129;
const constexpr auto CLEAR_WEEKLY_CHALLENGES_OFFSET = 0x147;

using refresh_challenge_list_func = void (*)(void* self);
using clear_daily_challenges_func = void (*)(void* self);
using clear_weekly_challenges_func = void (*)(void* self);

refresh_challenge_list_func original_refresh_challenge_list_ptr;

clear_daily_challenges_func clear_daily_challenges_ptr;
clear_weekly_challenges_func clear_weekly_challenges_ptr;

void refresh_challenge_list_hook(void* self) {
    if (enable) {
        original_refresh_challenge_list_ptr(self);
    } else {
        clear_daily_challenges_ptr(self);
        clear_weekly_challenges_ptr(self);
    }
}

}  // namespace

void init(void) {
    auto refresh = sigscan(REFRESH_CHALLENGE_LIST_SIG);

    clear_daily_challenges_ptr =
        read_offset<clear_daily_challenges_func>(refresh + CLEAR_DAILY_CHALLENGES_OFFSET);
    clear_weekly_challenges_ptr =
        read_offset<clear_weekly_challenges_func>(refresh + CLEAR_WEEKLY_CHALLENGES_OFFSET);

    auto ret = MH_CreateHook(reinterpret_cast<LPVOID>(refresh),
                             reinterpret_cast<LPVOID>(&refresh_challenge_list_hook),
                             reinterpret_cast<LPVOID*>(&original_refresh_challenge_list_ptr));
    if (ret != MH_OK) {
        // throw std::runtime_error("MH_CreateHook failed " + std::to_string(ret));
    }
    ret = MH_EnableHook(reinterpret_cast<LPVOID>(refresh));
    if (ret != MH_OK) {
        // throw std::runtime_error("MH_EnableHook failed " + std::to_string(ret));
    }
}

}  // namespace dhf::vault_cards
