#pragma once

struct CUserCmd;
struct C_BaseEntity;

// Movement features (bunny hop, autostrafe, jumpbug, edgebug)
// Each feature lives in its own subfolder; this header is the shared public API.
namespace Movement
{
    // AutoStrafe — single call per tick (before original):
    //   compute turn direction, inject subtick analog entries, set movement values
    void AutoStrafe(CUserCmd* pCmd, C_BaseEntity* pLocal);

    // Called from vtable[22] — subtick jump on landing + clear/re-press on ground
    void BunnyHop(CUserCmd* pCmd, C_BaseEntity* pLocal);

    // JumpBug — duck before landing, unduck+jump at landing fraction
    // Returns true if jumpbug fired (bhop should skip predictive landing)
    bool JumpBug(CUserCmd* pCmd, C_BaseEntity* pLocal);

    // EdgeBug — duck + zero movement when grazing an edge to avoid landing
    // Returns true if edgebug is active this tick (other features should yield)
    bool EdgeBug(CUserCmd* pCmd, C_BaseEntity* pLocal);

    // Sync IN_FORWARD/BACK/MOVELEFT/MOVERIGHT buttons with flForwardMove/flSideMove.
    // Call at the END of all feature processing to prevent server rejection.
    void ValidateUserCommand(CUserCmd* pCmd);

    // Reset per-tick landing fraction cache — call at start of each CreateMoveInner
    void InvalidateLandingCache();

    // Landing fraction shared between bhop/jumpbug/edgebug (cached, one trace per tick)
    // Returns [0,1] fraction of tick where player contacts ground, -1 if not landing this tick
    float GetLandingFraction(C_BaseEntity* pLocal);
}
