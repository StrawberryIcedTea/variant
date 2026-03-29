#pragma once
#include <cstdint>

// -----------------------------------------------------------------------
// Game constants and enums
// All globals (local player, entity list, CCSGOInput, view matrix) are
// resolved via pattern scans or engine interfaces -- no hardcoded offsets.
// -----------------------------------------------------------------------
constexpr uint32_t FL_ONGROUND = (1 << 0);

enum MoveType_t : uint8_t
{
    MOVETYPE_NONE = 0,
    MOVETYPE_WALK = 2,
    MOVETYPE_FLY = 3,
    MOVETYPE_NOCLIP = 7,
    MOVETYPE_LADDER = 8,
    MOVETYPE_OBSERVER = 9
};

// CS2 button flags
enum ECommandButtons : uint64_t
{
    IN_ATTACK = 1ULL << 0,
    IN_JUMP = 1ULL << 1,
    IN_DUCK = 1ULL << 2,
    IN_FORWARD = 1ULL << 3,
    IN_BACK = 1ULL << 4,
    IN_USE = 1ULL << 5,
    IN_MOVELEFT = 1ULL << 9,
    IN_MOVERIGHT = 1ULL << 10,
    IN_RELOAD = 1ULL << 13,
    IN_SPRINT = 1ULL << 16
};
