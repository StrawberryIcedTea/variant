// Movement features — bunny hop via direct button manipulation
// Called from vtable[22]: clears jump on ground, optionally injects subtick entries

#include "movement.h"
#include "../sdk/entitysystem.h"
#include "../sdk/usercmd.h"
#include "../core/variables.h"
#include "../utilities/inputhook.h"

void Movement::BunnyHop(CUserCmd* pCmd, C_BaseEntity* pLocal)
{
    if (Vars.nBhopMode == Variables_t::BHOP_DISABLED || !pCmd)
        return;

    if (!Input::bSpaceHeld.load(std::memory_order_relaxed))
        return;

    if (pLocal->m_MoveType == MOVETYPE_LADDER || pLocal->m_MoveType == MOVETYPE_NOCLIP ||
        pLocal->m_MoveType == MOVETYPE_OBSERVER)
        return;

    if (!(pLocal->m_fFlags & FL_ONGROUND))
        return;

    constexpr uint64_t jump = static_cast<uint64_t>(IN_JUMP);

    // Clear jump to force a release — engine re-sets from space next frame
    pCmd->nButtons.nValue &= ~jump;

    // Subtick: inject press/release entries for timing
    if (Vars.nBhopMode == Variables_t::BHOP_SUBTICK && Subtick::fnCreateElement && IsValidPtr(pCmd->pBaseCmd))
        Subtick::Inject(pCmd->pBaseCmd, jump);
}
