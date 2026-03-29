#pragma once
#include "../../utilities/memory.h"

// -----------------------------------------------------------------------
// IEngineClient -- Source2EngineToClient001 from engine2.dll
// Provides local player index, in-game state, level info
// -----------------------------------------------------------------------
class IEngineClient
{
  public:
    // vtable[33]: GetLocalPlayer(int& nIndex, int nSplitScreenSlot)
    // Returns the local player's entity index (1-based after +1 adjustment)
    int GetLocalPlayer()
    {
        int nIndex = -1;
        // GetLocalPlayer is vtable[33], takes (int& nSlot, int nSplitScreenSlot)
        using Fn = void(__fastcall*)(void*, int&, int);
        auto fn = M::GetVFunc<Fn>(this, 33);
        fn(this, nIndex, 0);
        return nIndex + 1;
    }

    bool IsInGame() { return M::CallVFunc<bool>(this, 35); }

    bool IsConnected() { return M::CallVFunc<bool>(this, 36); }
};
