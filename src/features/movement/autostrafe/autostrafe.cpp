// AutoStrafe — optimal air acceleration via subtick analog deltas or direct move values

#include "../movement.h"
#include "../../../sdk/entity.h"
#include "../../../sdk/datatypes/usercmd.h"
#include "../../../core/convars.h"
#include "../../../core/variables.h"
#include "../../../core/interfaces.h"
#include "../../../core/hooks/prediction/prediction.h"
#include <algorithm>
#include <cmath>

static CCSPlayer_MovementServices* GetMovementServices(C_BaseEntity* pLocal)
{
    auto* pSvc = reinterpret_cast<C_BasePlayerPawn*>(pLocal)->GetMovementServices();
    return IsValidPtr(pSvc) ? static_cast<CCSPlayer_MovementServices*>(pSvc) : nullptr;
}

static void GetMovementParams(C_BaseEntity* pLocal, float& outFriction, float& outStamina)
{
    auto* pSvc = GetMovementServices(pLocal);
    if (!pSvc)
    {
        outFriction = 1.f;
        outStamina = 0.f;
        return;
    }

    float f = pSvc->GetSurfaceFriction();
    outFriction = (f > 0.f && f <= 2.f) ? f : 1.f;

    float s = pSvc->GetStamina();
    outStamina = (s >= 0.f && s <= 100.f) ? s : 0.f;
}

constexpr float kPi = 3.14159265358979323846f;
constexpr float kDegToRad = kPi / 180.f;

static float Speed2D(float vx, float vy)
{
    return std::sqrtf(vx * vx + vy * vy);
}

static void AirAccelerate(float& vx, float& vy, float wishX, float wishY, float dt, float surfaceFriction,
                          float wishspeed, float accelrate)
{
    float currentspeed = vx * wishX + vy * wishY;
    float addspeed = wishspeed - currentspeed;
    if (addspeed <= 0.f)
        return;

    float accelspeed = accelrate * dt * wishspeed * surfaceFriction;
    if (accelspeed > addspeed)
        accelspeed = addspeed;

    vx += accelspeed * wishX;
    vy += accelspeed * wishY;
}

static float OptimalStrafeAngle(float speed2d, float airMaxWish)
{
    if (speed2d <= airMaxWish)
        return kPi / 2.f;

    float cosTheta = airMaxWish / speed2d;
    return std::acosf(std::clamp(cosTheta, -1.f, 1.f));
}

static void CalcOptimalStrafe(float vx, float vy, float yawRad, int direction, float airMaxWish, float& outFwd,
                              float& outSide, float& outWishX, float& outWishY)
{
    float speed = Speed2D(vx, vy);
    float cosYaw = std::cosf(yawRad);
    float sinYaw = std::sinf(yawRad);

    if (speed < 1.f)
    {
        outFwd = 0.f;
        outSide = static_cast<float>(direction);
        outWishX = outFwd * cosYaw - outSide * sinYaw;
        outWishY = outFwd * sinYaw + outSide * cosYaw;
        return;
    }

    float optAngle = OptimalStrafeAngle(speed, airMaxWish);
    float velAngle = std::atan2f(vy, vx);
    float targetAngle = velAngle + static_cast<float>(direction) * optAngle;

    float wishX = std::cosf(targetAngle);
    float wishY = std::sinf(targetAngle);

    float fwd = wishX * cosYaw + wishY * sinYaw;
    float side = -wishX * sinYaw + wishY * cosYaw;

    float len = Speed2D(fwd, side);
    if (len > 0.001f)
    {
        fwd /= len;
        side /= len;
    }

    outFwd = fwd;
    outSide = side;
    outWishX = wishX;
    outWishY = wishY;
}

// -----------------------------------------------------------------------
// AutoStrafe
// -----------------------------------------------------------------------
void Movement::AutoStrafe(CUserCmd* pCmd, C_BaseEntity* pLocal)
{
    if (Vars.nAutoStrafeMode == Variables_t::STRAFE_DISABLED || !pCmd)
        return;

    if (!IsValidPtr(pCmd->pBaseCmd) || !IsValidPtr(pCmd->pBaseCmd->pViewAngles))
        return;

    if (pLocal->IsMovetypeBlocked())
        return;

    uint32_t flags = Prediction::GetFlags(pLocal);
    if (flags & FL_ONGROUND)
        return;

    static float flPrevYaw = 0.f;
    float flYaw = pCmd->pBaseCmd->pViewAngles->y;
    float flDelta = flYaw - flPrevYaw;
    flPrevYaw = flYaw;

    if (flDelta > 180.f)
        flDelta -= 360.f;
    else if (flDelta < -180.f)
        flDelta += 360.f;

    if (std::fabs(flDelta) < 0.1f)
        return;

    int direction = (flDelta > 0.f) ? 1 : -1;

    const float* vel = pLocal->GetAbsVelocity();
    float vx = vel[0], vy = vel[1];
    float yawRad = flYaw * kDegToRad;

    float friction = 1.f, stamina = 0.f;
    GetMovementParams(pLocal, friction, stamina);

    if (stamina > 0.f)
    {
        float speedPenalty = std::max(0.f, 1.f - stamina / 100.f);
        vx *= speedPenalty;
        vy *= speedPenalty;
    }

    float airMaxWish = Convar::GetFloat(Convar::sv_air_max_wishspeed, 30.f);
    float accelrate = Convar::GetFloat(Convar::sv_airaccelerate, 12.f);
    auto* pBase = pCmd->pBaseCmd;

    if (Vars.nAutoStrafeMode == Variables_t::STRAFE_NORMAL)
    {
        float fwd = 0.f, side = 0.f, wishX = 0.f, wishY = 0.f;
        CalcOptimalStrafe(vx, vy, yawRad, direction, airMaxWish, fwd, side, wishX, wishY);
        pBase->flForwardMove = fwd;
        pBase->flSideMove = side;
        return;
    }

    int nSteps = std::clamp(Subtick::kMaxEntries - Subtick::pool.nUsed, 0, 12);
    if (nSteps <= 0)
        return;

    float tickInterval = I::GetTickInterval();
    float frametime = tickInterval / static_cast<float>(nSteps);

    float flMoveFwd = 0.f;
    float flMoveSide = 0.f;

    for (int i = 0; i < nSteps; ++i)
    {
        float fwd = 0.f, side = 0.f, wishX = 0.f, wishY = 0.f;
        CalcOptimalStrafe(vx, vy, yawRad, direction, airMaxWish, fwd, side, wishX, wishY);

        float flDeltaFwd = fwd - flMoveFwd;
        float flDeltaSide = side - flMoveSide;

        if (auto* pSubtick = pBase->CreateSubtickMove())
        {
            pSubtick->nHasBits |= MOVESTEP_BITS_WHEN | MOVESTEP_BITS_ANALOG_FWD | MOVESTEP_BITS_ANALOG_LEFT;
            pSubtick->flWhen = static_cast<float>(i) / static_cast<float>(nSteps);
            pSubtick->flAnalogForwardDelta = flDeltaFwd;
            pSubtick->flAnalogLeftDelta = flDeltaSide;
        }

        flMoveFwd += flDeltaFwd;
        flMoveSide += flDeltaSide;

        AirAccelerate(vx, vy, wishX, wishY, frametime, friction, airMaxWish, accelrate);
    }

    pBase->flForwardMove = flMoveFwd;
    pBase->flSideMove = flMoveSide;
}
