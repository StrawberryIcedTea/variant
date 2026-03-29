// ValidateUserCommand — sync movement buttons with move values + clamp view angles

#include "../movement.h"
#include "../../../sdk/datatypes/usercmd.h"
#include "../../../sdk/const.h"
#include <algorithm>
#include <cmath>

void Movement::ValidateUserCommand(CUserCmd* pCmd)
{
    if (!pCmd || !IsValidPtr(pCmd->pBaseCmd))
        return;

    auto* pBase = pCmd->pBaseCmd;
    auto& nValue = pCmd->nButtons.nValue;

    // Forward / Back
    nValue &= ~(static_cast<uint64_t>(IN_FORWARD) | static_cast<uint64_t>(IN_BACK));
    if (pBase->flForwardMove > 0.f)
        nValue |= static_cast<uint64_t>(IN_FORWARD);
    else if (pBase->flForwardMove < 0.f)
        nValue |= static_cast<uint64_t>(IN_BACK);

    // Left / Right (CS2: positive flSideMove = LEFT, negative = RIGHT)
    nValue &= ~(static_cast<uint64_t>(IN_MOVELEFT) | static_cast<uint64_t>(IN_MOVERIGHT));
    if (pBase->flSideMove > 0.f)
        nValue |= static_cast<uint64_t>(IN_MOVELEFT);
    else if (pBase->flSideMove < 0.f)
        nValue |= static_cast<uint64_t>(IN_MOVERIGHT);

    // Clamp view angles
    if (IsValidPtr(pBase->pViewAngles))
    {
        pBase->pViewAngles->x = std::clamp(pBase->pViewAngles->x, -89.f, 89.f);

        float& yaw = pBase->pViewAngles->y;
        yaw = std::fmod(yaw, 360.f);
        if (yaw > 180.f)
            yaw -= 360.f;
        else if (yaw < -180.f)
            yaw += 360.f;
    }
}
