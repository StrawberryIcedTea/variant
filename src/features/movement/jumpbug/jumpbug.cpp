// JumpBug — duck before landing, unduck+jump at exact landing fraction

#include "../movement.h"
#include "../../../sdk/entity.h"
#include "../../../sdk/datatypes/usercmd.h"
#include "../../../core/variables.h"
#include "../../../core/interfaces.h"
#include "../../../core/hooks/prediction/prediction.h"
#include "../../../utilities/input.h"
#include <algorithm>

// -----------------------------------------------------------------------
// JumpBug
//
// Subtick layout (4 entries):
//   1. Duck press at 0.0 (start of tick — shrink bbox)
//   2. Duck release at landing fraction (restore bbox at contact)
//   3. Jump release at landing fraction (clear any held jump)
//   4. Jump press at landing fraction (jump off ground)
// -----------------------------------------------------------------------
bool Movement::JumpBug(CUserCmd* pCmd, C_BaseEntity* pLocal)
{
    if (!Vars.bJumpBug || !pCmd)
        return false;

    if (!IsValidPtr(pCmd->pBaseCmd))
        return false;

    if (!Input::bSpaceHeld.load(std::memory_order_relaxed))
        return false;

    if (pLocal->IsMovetypeBlocked())
        return false;

    uint32_t flags = Prediction::GetFlags(pLocal);
    if (flags & FL_ONGROUND)
        return false;

    float landFrac = GetLandingFraction(pLocal);
    if (landFrac < 0.f || landFrac >= 1.f)
        return false;

    constexpr auto duck = static_cast<uint64_t>(IN_DUCK);
    constexpr auto jump = static_cast<uint64_t>(IN_JUMP);

    pCmd->nButtons.nValue &= ~(duck | jump);

    float frac = std::clamp(landFrac, 0.02f, 0.98f);

    Subtick::AddButton(pCmd, duck, 0.f, true);   // duck at start (shrink bbox)
    Subtick::AddButton(pCmd, duck, frac, false); // unduck at landing (restore bbox)
    Subtick::AddButton(pCmd, jump, frac, false); // release jump (clear held)
    Subtick::AddButton(pCmd, jump, frac, true);  // press jump (jump off ground)

    return true;
}
