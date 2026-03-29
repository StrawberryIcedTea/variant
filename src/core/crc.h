// CRC rebuild — recompute pMoveCrc after modifying CBaseUserCmdPB fields
//
// The engine computes pMoveCrc over the serialized CBaseUserCmdPB during
// the original CreateMove call. Any post-original modifications to protobuf
// fields (flForwardMove, flSideMove, pViewAngles, etc.) invalidate the CRC.
//
// On secure servers this causes command rejection. CRC::Rebuild() re-serializes
// the modified command and updates the CRC to match.
//
// If patterns aren't found (stale build), Rebuild() returns false and the
// command goes out with a stale CRC — harmless in -insecure mode.
// SEH-protected: auto-disables on crash from stale pattern matches.

#pragma once
#include <cstdint>

struct CBaseUserCmdPB;

namespace CRC
{
    // Resolve CRC rebuild functions from engine.
    // Call once during init (after client.dll + tier0.dll are loaded).
    // Returns true if all functions found (CRC rebuild available).
    bool Setup();

    // Whether CRC rebuild is available (all patterns + exports resolved)
    bool IsAvailable();

    // Rebuild pMoveCrc after modifying CBaseUserCmdPB fields.
    // Call after ALL post-original command modifications are complete.
    // Returns true if CRC was successfully rebuilt.
    bool Rebuild(CBaseUserCmdPB* pBaseCmd);
}
