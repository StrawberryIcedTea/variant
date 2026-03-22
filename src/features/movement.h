#pragma once

struct CInButtonState;
struct CUserCmd;
struct C_BaseEntity;

// Movement features (bunny hop, etc.)
// CS2 subtick engine handles press/release internally — we only need to
// clear IN_JUMP on ground to force a release, engine re-presses from held space.
namespace Movement
{
    // Called from vtable[22] — clears jump on ground + optional subtick entries
    void BunnyHop(CUserCmd* pCmd, const C_BaseEntity* pLocal);
}
