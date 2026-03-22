#pragma once
#include <cstdint>
#include <cstddef>
#include "../utilities/memory.h"

// -----------------------------------------------------------------------
// Offsets from cs2-dumper — 2026-03-19
// -----------------------------------------------------------------------
namespace Offsets
{
    // client.dll globals
    constexpr uintptr_t dwLocalPlayerController = 0x22F4188;
    constexpr uintptr_t dwLocalPlayerPawn = 0x2069B50;
    constexpr uintptr_t dwEntityList = 0x24AF268;

    // CCSGOInput object (inline global in client.dll .data)
    constexpr uintptr_t dwCSGOInput = 0x2319FC0;

    // IGameResourceService
    constexpr uintptr_t pEntitySystem = 0x58; // CGameEntitySystem* at IGameResourceService+0x58
}

// -----------------------------------------------------------------------
// Game constants and enums
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

// -----------------------------------------------------------------------
// C_BaseEntity — partial struct (cs2-dumper offsets, March 2026)
// -----------------------------------------------------------------------
// cppcheck-suppress noConstructor
struct C_BaseEntity
{
    MEM_PAD(0x354);        // +0x000
    int32_t m_iHealth;     // +0x354
    MEM_PAD(0x04);         // +0x358
    uint8_t m_lifeState;   // +0x35C
    MEM_PAD(0xA3);         // +0x35D
    uint32_t m_fFlags;     // +0x400
    MEM_PAD(0x129);        // +0x404
    MoveType_t m_MoveType; // +0x52D
};
static_assert(offsetof(C_BaseEntity, m_iHealth) == 0x354);
static_assert(offsetof(C_BaseEntity, m_lifeState) == 0x35C);
static_assert(offsetof(C_BaseEntity, m_fFlags) == 0x400);
static_assert(offsetof(C_BaseEntity, m_MoveType) == 0x52D);

// -----------------------------------------------------------------------
// Entity system helpers
// -----------------------------------------------------------------------
namespace EntitySystem
{
    uintptr_t GetClientBase();
    C_BaseEntity* GetLocalPlayerPawn();
}
