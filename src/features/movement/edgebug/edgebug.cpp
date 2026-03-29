// EdgeBug — duck + zero movement when grazing a ledge edge to avoid landing

#include "../movement.h"
#include "../../../sdk/entity.h"
#include "../../../sdk/datatypes/usercmd.h"
#include "../../../sdk/trace.h"
#include "../../../core/variables.h"
#include "../../../core/hooks/prediction/prediction.h"

// Point trace straight down from (x, y, z) to check for ground at that XY position
static float TraceDownFrom(float x, float y, float z, float dist)
{
    if (!Trace::IsAvailable())
        return 1.f;

    Vec3 origin(x, y, z);
    Vec3 end(x, y, z - dist);

    Ray_t ray{};
    ray.m_vecStart = origin;
    ray.m_vecEnd = end;
    ray.m_nType = 0; // point trace

    TraceFilter_t filter{};
    filter.Init(0x1400B, 3);

    GameTrace_t trace{};
    if (!Trace::Shape(&ray, &origin, &end, &filter, &trace))
        return 1.f;

    return trace.m_flFraction;
}

// -----------------------------------------------------------------------
// EdgeBug
//
// Triggered when:
//   1. Center trace DOWN misses (no ground directly below center)
//   2. At least one bbox-edge offset trace HITS ground
//   3. Player will land this tick (GetLandingFraction < 1)
// -----------------------------------------------------------------------
bool Movement::EdgeBug(CUserCmd* pCmd, C_BaseEntity* pLocal)
{
    if (!Vars.bEdgeBug || !pCmd)
        return false;

    if (!IsValidPtr(pCmd->pBaseCmd))
        return false;

    if (pLocal->IsMovetypeBlocked())
        return false;

    uint32_t flags = Prediction::GetFlags(pLocal);
    if (flags & FL_ONGROUND)
        return false;

    const float* vel = pLocal->GetAbsVelocity();
    if (vel[2] >= 0.f)
        return false; // not falling

    float landFrac = GetLandingFraction(pLocal);
    if (landFrac < 0.f || landFrac >= 1.f)
        return false;

    const float* absOrigin = pLocal->GetAbsOrigin();
    auto* pCollision = pLocal->GetCollision();
    if (!absOrigin || !pCollision)
        return false;

    float ox = absOrigin[0];
    float oy = absOrigin[1];
    float oz = absOrigin[2];

    // Center must be over the void
    if (TraceDownFrom(ox, oy, oz, 8.f) < 1.f)
        return false;

    // At least one bbox edge must clip ground
    const float* colMaxs = pCollision->GetMaxs();
    float edgeX = colMaxs[0];
    float edgeY = colMaxs[1];

    static const float offsets[][2] = {
        {1.f, 0.f}, {-1.f, 0.f}, {0.f, 1.f}, {0.f, -1.f}, {1.f, 1.f}, {-1.f, 1.f}, {1.f, -1.f}, {-1.f, -1.f},
    };

    bool edgeFound = false;
    for (auto& off : offsets)
    {
        if (TraceDownFrom(ox + off[0] * edgeX, oy + off[1] * edgeY, oz, 8.f) < 1.f)
        {
            edgeFound = true;
            break;
        }
    }

    if (!edgeFound)
        return false;

    constexpr auto duck = static_cast<uint64_t>(IN_DUCK);
    constexpr auto jump = static_cast<uint64_t>(IN_JUMP);

    pCmd->nButtons.nValue |= duck;
    pCmd->nButtons.nValue &= ~jump;
    pCmd->pBaseCmd->flForwardMove = 0.f;
    pCmd->pBaseCmd->flSideMove = 0.f;

    return true;
}
