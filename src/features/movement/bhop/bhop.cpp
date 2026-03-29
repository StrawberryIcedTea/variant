// BunnyHop — trace-based landing detection for precise subtick jump timing
// Also owns the per-tick landing fraction cache used by jumpbug and edgebug.

#include "../movement.h"
#include "../../../sdk/entity.h"
#include "../../../sdk/datatypes/usercmd.h"
#include "../../../sdk/trace.h"
#include "../../../core/convars.h"
#include "../../../core/variables.h"
#include "../../../core/interfaces.h"
#include "../../../core/hooks/prediction/prediction.h"
#include "../../../utilities/input.h"
#include <algorithm>

static bool IsValidGround(const Vec3& normal)
{
    return normal.z >= Convar::GetStandableNormal();
}

// -----------------------------------------------------------------------
// Landing fraction cache — computed once per tick, reset each CreateMoveInner
// -----------------------------------------------------------------------
static float s_cachedLandFrac = -1.f;
static bool s_landFracValid = false;

static float PredictLandingFraction(C_BaseEntity* pLocal)
{
    auto* pCollision = pLocal->GetCollision();
    const float* absOrigin = pLocal->GetAbsOrigin();
    if (!Trace::IsAvailable() || !pCollision || !absOrigin)
        return -1.f;

    float tickInterval = I::GetTickInterval();
    float gravity = Convar::GetGravity();
    const float* vel = pLocal->GetAbsVelocity();
    float vz = vel[2];

    float vzEnd = vz - gravity * tickInterval;
    if (vz > 0.f && vzEnd > 0.f)
        return -1.f; // still ascending

    const float* colMins = pCollision->GetMins();
    const float* colMaxs = pCollision->GetMaxs();
    Vec3 origin(absOrigin[0], absOrigin[1], absOrigin[2]);
    Vec3 mins(colMins[0], colMins[1], colMins[2]);
    Vec3 maxs(colMaxs[0], colMaxs[1], colMaxs[2]);

    float vx = vel[0], vy = vel[1];
    float dz = vz * tickInterval - 0.5f * gravity * tickInterval * tickInterval;
    Vec3 end(origin.x + vx * tickInterval, origin.y + vy * tickInterval, origin.z + dz);

    Ray_t ray{};
    ray.m_vecStart = origin;
    ray.m_vecEnd = end;
    ray.m_vecMins = mins;
    ray.m_vecMaxs = maxs;
    ray.m_nType = 1;

    TraceFilter_t filter{};
    filter.Init(0x1400B, 3);

    GameTrace_t trace{};
    if (!Trace::Shape(&ray, &origin, &end, &filter, &trace))
        return -1.f;

    if (trace.m_flFraction >= 1.f)
        return -1.f;

    if (!IsValidGround(trace.m_vecNormal))
        return -1.f;

    return trace.m_flFraction;
}

float Movement::GetLandingFraction(C_BaseEntity* pLocal)
{
    if (s_landFracValid)
        return s_cachedLandFrac;

    s_cachedLandFrac = PredictLandingFraction(pLocal);
    s_landFracValid = true;
    return s_cachedLandFrac;
}

void Movement::InvalidateLandingCache()
{
    s_landFracValid = false;
}

// -----------------------------------------------------------------------
// BunnyHop
// -----------------------------------------------------------------------
void Movement::BunnyHop(CUserCmd* pCmd, C_BaseEntity* pLocal)
{
    if (Vars.nBhopMode == Variables_t::BHOP_DISABLED || !pCmd)
        return;

    if (Convar::GetBool(Convar::sv_autobunnyhopping, false))
        return;

    if (!Input::bSpaceHeld.load(std::memory_order_relaxed))
        return;

    if (pLocal->IsMovetypeBlocked())
        return;

    constexpr auto jump = static_cast<uint64_t>(IN_JUMP);

    uint32_t flags = Prediction::GetFlags(pLocal);
    bool bGrounded = (flags & FL_ONGROUND) != 0;

    // Predicted flags lag by one tick — if we predict landing this tick, treat as grounded.
    if (!bGrounded)
    {
        float landFrac = GetLandingFraction(pLocal);
        if (landFrac >= 0.f && landFrac < 1.f)
            bGrounded = true;
    }

    if (!bGrounded)
        return; // airborne, no landing predicted this tick

    // Grounded (or predicted landing): clear button so engine sees a clean 0→1 transition.
    pCmd->nButtons.nValue &= ~jump;

    // BHOP_SUBTICK: pin the press to the predicted landing fraction, or early in tick if already grounded.
    if (Vars.nBhopMode == Variables_t::BHOP_SUBTICK && IsValidPtr(pCmd->pBaseCmd))
    {
        float landFrac = GetLandingFraction(pLocal); // cached — free second call
        float pressFrac = (landFrac >= 0.f && landFrac < 1.f) ? std::clamp(landFrac + 0.01f, 0.f, 0.99f) : 0.01f;
        Subtick::AddButton(pCmd, jump, 0.f, false);      // release
        Subtick::AddButton(pCmd, jump, pressFrac, true); // press at landing or early
    }
}
