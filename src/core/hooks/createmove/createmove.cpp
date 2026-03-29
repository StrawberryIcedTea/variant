// CreateMove hooks — CCSGOInput vtable[5] + vtable[22]
// vtable[5]: passthrough (hooked for future use)
// vtable[22]: bhop + autostrafe + subtick inject/restore + CRC rebuild

#include "createmove.h"
#include "../hooks.h"
#include "../../interfaces.h"
#include "../../../sdk/entity.h"
#include "../../../sdk/datatypes/usercmd.h"
#include "../../crc.h"
#include "../../../features/movement/movement.h"
#include "../../../utilities/debug.h"

// -----------------------------------------------------------------------
// vtable[5] — outer wrapper
// -----------------------------------------------------------------------
bool __fastcall CreateMove::hkCreateMove(void* pInput, int nSlot, bool bActive)
{
    auto oCreateMove = DTR::CreateMove.Original<decltype(&hkCreateMove)>();
    return oCreateMove(pInput, nSlot, bActive);
}

// -----------------------------------------------------------------------
// vtable[22] — inner CreateMove (CUserCmd* access + subtick)
//
// Flow:
//   1. BEFORE original: BunnyHop + JumpBug (buttons + subtick inject)
//   2. BEFORE original: AutoStrafe (subtick analog deltas + movement values)
//   3. Original runs → engine processes subtick entries + computes nMoveCrc
//   4. Restore subtick pool (undo pRep replacement)
//   5. AFTER original: EdgeBug (zeros movement if active)
//   6. AFTER original: ValidateUserCommand (sync buttons + clamp angles)
//   7. AFTER original: CRC::Rebuild (fix CRC for post-original modifications)
// -----------------------------------------------------------------------
bool __fastcall CreateMove::hkCreateMoveInner(void* pInput, int nSlot, void* pCmd)
{
    auto oCreateMoveInner = DTR::CreateMoveInner.Original<decltype(&hkCreateMoveInner)>();

    auto* cmd = static_cast<CUserCmd*>(pCmd);
    if (!cmd)
        return oCreateMoveInner(pInput, nSlot, pCmd);

    Movement::InvalidateLandingCache();

    // BEFORE original: button manipulation + subtick injection + movement values
    auto* pLocal = EntitySystem::GetLocalPlayerPawn();
    if (pLocal && pLocal->GetHealth() > 0)
    {
        if (!Movement::JumpBug(cmd, pLocal))
            Movement::BunnyHop(cmd, pLocal);

        Movement::AutoStrafe(cmd, pLocal);
    }

    bool result = oCreateMoveInner(pInput, nSlot, pCmd);

    // Restore subtick pool (undo pRep replacement before engine sees it next tick)
    Subtick::Restore();

    // AFTER original: EdgeBug overrides movement if on an edge
    if (pLocal && pLocal->GetHealth() > 0)
        Movement::EdgeBug(cmd, pLocal);

    Movement::ValidateUserCommand(cmd);

    // CRC rebuild — recompute pMoveCrc after post-original modifications.
    // SEH-protected: auto-disables if pattern match is stale.
    if (CRC::IsAvailable() && IsValidPtr(cmd->pBaseCmd))
        CRC::Rebuild(cmd->pBaseCmd);

    return result;
}
