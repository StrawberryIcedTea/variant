#pragma once
#include "includes.h"

// Global shared state — accessible from any module
namespace G
{
    // DLL module handle (set in DllMain)
    inline HMODULE hDll = nullptr;

    // Local player pointer (updated per-frame in hooks)
    inline void* pLocal = nullptr;

    // True while DLL should stay loaded
    inline bool bRunning = true;
}
