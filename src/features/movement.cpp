// Movement features — bunny hop
// Wired into CreateMove via createmove.cpp
//
// Uses ForceJump state machine (adapted from saura07) for reliable bhop.
// Queries EntitySystem for player state — no raw offsets in feature code.

#include "movement.h"
#include "../sdk/entitysystem.h"
#include "../core/variables.h"
#include "../utilities/inputhook.h"

void Movement::BunnyHop()
{
    if (!Vars.bBunnyHop)
        return;

    if (!EntitySystem::InitForceJump())
        return;

    bool bSpaceHeld = Input::bSpaceHeld.load(std::memory_order_relaxed);

    // State machine (adapted from saura07 CS:GO bhop)
    // bLastJumped: we forced jump last tick while on ground
    // bShouldFake: try forcing jump again to catch narrow landing windows
    static bool bLastJumped = false, bShouldFake = false;

    if (!bSpaceHeld)
    {
        bLastJumped = false;
        bShouldFake = false;
        return;
    }

    void* pPawn = EntitySystem::GetLocalPlayerPawn();
    if (!pPawn)
        return;

    uint8_t moveType = GetField<uint8_t>(pPawn, Offsets::m_MoveType);
    if (moveType == MOVETYPE_LADDER || moveType == MOVETYPE_NOCLIP || moveType == MOVETYPE_OBSERVER ||
        moveType == MOVETYPE_FLY)
        return;

    bool bOnGround = (GetField<uint32_t>(pPawn, Offsets::m_fFlags) & FL_ONGROUND) != 0;

    if (!bLastJumped && bShouldFake)
    {
        bShouldFake = false;
        EntitySystem::SetForceJump(true);
    }
    else if (bOnGround)
    {
        bLastJumped = true;
        bShouldFake = true;
        EntitySystem::SetForceJump(true);
    }
    else
    {
        EntitySystem::SetForceJump(false);
        bLastJumped = false;
    }
}
