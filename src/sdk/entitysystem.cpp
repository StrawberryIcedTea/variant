// Entity system access — local player, button state

#include "entitysystem.h"
#include "../utilities/debug.h"
#include <format>

// -----------------------------------------------------------------------
// Module base
// -----------------------------------------------------------------------

uintptr_t EntitySystem::GetClientBase()
{
    static uintptr_t base = 0;
    if (!base)
    {
        HMODULE hClient = GetModuleHandleA("client.dll");
        if (hClient)
            base = reinterpret_cast<uintptr_t>(hClient);
    }
    return base;
}

// -----------------------------------------------------------------------
// Local player
// -----------------------------------------------------------------------

void* EntitySystem::GetLocalPlayerController()
{
    uintptr_t base = GetClientBase();
    if (!base)
        return nullptr;

    return *reinterpret_cast<void**>(base + Offsets::dwLocalPlayerController);
}

void* EntitySystem::GetLocalPlayerPawn()
{
    uintptr_t base = GetClientBase();
    if (!base)
        return nullptr;

    return *reinterpret_cast<void**>(base + Offsets::dwLocalPlayerPawn);
}

// -----------------------------------------------------------------------
// Local player field accessors
// -----------------------------------------------------------------------

uint32_t EntitySystem::GetLocalPlayerFlags()
{
    void* pPawn = GetLocalPlayerPawn();
    if (!pPawn)
        return 0;

    return GetField<uint32_t>(pPawn, Offsets::m_fFlags);
}

uint8_t EntitySystem::GetLocalMoveType()
{
    void* pPawn = GetLocalPlayerPawn();
    if (!pPawn)
        return MOVETYPE_NONE;

    return GetField<uint8_t>(pPawn, Offsets::m_MoveType);
}

// -----------------------------------------------------------------------
// ForceJump — button force state at buttons::jump + 0x00
// -----------------------------------------------------------------------

static constexpr uint32_t FORCE_JUMP_ON = 65537; // 0x10001
static constexpr uint32_t FORCE_JUMP_OFF = 256;  // 0x100

static uint32_t* s_pForceJump = nullptr;
static bool s_forceJumpInitDone = false;

bool EntitySystem::InitForceJump()
{
    if (s_forceJumpInitDone)
        return s_pForceJump != nullptr;
    s_forceJumpInitDone = true;

    uintptr_t clientBase = GetClientBase();
    if (!clientBase)
    {
        C::Print("[entitysystem] ForceJump init failed — client.dll base not found");
        return false;
    }

    s_pForceJump = reinterpret_cast<uint32_t*>(clientBase + ButtonOffsets::jump);

    C::Print(std::format("[entitysystem] ForceJump @ {:#x}, current value: {:#x}",
                         reinterpret_cast<uintptr_t>(s_pForceJump), *s_pForceJump));
    return true;
}

void EntitySystem::SetForceJump(bool bForce)
{
    if (s_pForceJump)
        *s_pForceJump = bForce ? FORCE_JUMP_ON : FORCE_JUMP_OFF;
}
