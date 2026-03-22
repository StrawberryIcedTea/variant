// CreateMove hooks — CCSGOInput vtable[5] + vtable[22]
// vtable[5]: passthrough (reserved for future features)
// vtable[22]: bhop + subtick inject/restore

#include "createmove.h"
#include "../hooks.h"
#include "../../interfaces.h"
#include "../../../sdk/entitysystem.h"
#include "../../../sdk/usercmd.h"
#include "../../../features/movement.h"

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
// -----------------------------------------------------------------------
bool __fastcall CreateMove::hkCreateMoveInner(void* pInput, int nSlot, void* pCmd)
{
    auto oCreateMoveInner = DTR::CreateMoveInner.Original<decltype(&hkCreateMoveInner)>();

    auto* cmd = static_cast<CUserCmd*>(pCmd);

    // Feature processing BEFORE original — engine processes our modifications
    if (cmd && pInput == I::pCSGOInput)
    {
        auto* pLocal = EntitySystem::GetLocalPlayerPawn();
        if (pLocal && pLocal->m_iHealth > 0)
            Movement::BunnyHop(cmd, pLocal);
    }

    bool result = oCreateMoveInner(pInput, nSlot, pCmd);

    // Restore subtick field after original (prevent engine cleanup of our static objects)
    if (cmd && pInput == I::pCSGOInput && IsValidPtr(cmd->pBaseCmd))
        Subtick::Restore(cmd->pBaseCmd);

    return result;
}
