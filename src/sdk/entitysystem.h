#pragma once
#include "../includes.h"

// -----------------------------------------------------------------------
// Offsets from cs2-dumper (2026-03-12)
// These are hardcoded — will break on game updates.
// -----------------------------------------------------------------------
namespace Offsets
{
    // client.dll globals (cs2-dumper 2026-03-12)
    constexpr uintptr_t dwLocalPlayerController = 0x22F3178;
    constexpr uintptr_t dwLocalPlayerPawn = 0x2068B60;
    constexpr uintptr_t dwEntityList = 0x24AE268;

    // CCSPlayerController
    constexpr uintptr_t m_hPlayerPawn = 0x90C;

    // C_BaseEntity
    constexpr uintptr_t m_fFlags = 0x400;
    constexpr uintptr_t m_iHealth = 0x354;
    constexpr uintptr_t m_lifeState = 0x35C;
    constexpr uintptr_t m_vecVelocity = 0x438;
    constexpr uintptr_t m_MoveType = 0x52D; // MoveType_t (uint8)

    // C_BasePlayerPawn
    constexpr uintptr_t m_pMovementServices = 0x1418;
}

// -----------------------------------------------------------------------
// Game constants and enums
// -----------------------------------------------------------------------
constexpr uint32_t FL_ONGROUND = (1 << 0);

enum MoveType_t : uint8_t
{
    MOVETYPE_NONE = 0,
    MOVETYPE_OBSOLETE = 1,
    MOVETYPE_WALK = 2,
    MOVETYPE_FLY = 3,
    MOVETYPE_FLYGRAVITY = 4,
    MOVETYPE_VPHYSICS = 5,
    MOVETYPE_PUSH = 6,
    MOVETYPE_NOCLIP = 7,
    MOVETYPE_LADDER = 8,
    MOVETYPE_OBSERVER = 9,
    MOVETYPE_CUSTOM = 10
};

// CS2 button flags for CInButtonState::nValue
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

// Button state container (CInButtonState from CS2 SDK)
struct CInButtonState
{
    uint8_t _pad0[0x8];     // 0x00 vtable pointer
    uint64_t nValue;        // 0x08 current pressed buttons
    uint64_t nValueChanged; // 0x10 buttons that changed this tick
    uint64_t nValueScroll;  // 0x18 scroll-related state
};
static_assert(sizeof(CInButtonState) == 0x20, "CInButtonState size mismatch");

// -----------------------------------------------------------------------
// Global button state offsets (cs2-dumper buttons.hpp, client.dll)
// buttons::jump + 0x00 is the ForceJump uint32 (no vtable at this address).
// Write 0x10001 to force jump, 0x100 to release.
// -----------------------------------------------------------------------
namespace ButtonOffsets
{
    constexpr uintptr_t jump = 0x2061E00;
}

// -----------------------------------------------------------------------
// Entity system helpers
// -----------------------------------------------------------------------
// Read a field at a given offset from an entity pointer
template <typename T> inline T GetField(void* pEntity, uintptr_t offset)
{
    return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(pEntity) + offset);
}

namespace EntitySystem
{
    uintptr_t GetClientBase();

    // Local player pointers
    void* GetLocalPlayerController();
    void* GetLocalPlayerPawn();

    // Local player field accessors — wraps offset reads so features stay clean
    uint32_t GetLocalPlayerFlags();
    uint8_t GetLocalMoveType();

    // ForceJump — resolves buttons::jump once, then caches.
    bool InitForceJump();
    void SetForceJump(bool bForce);
}
