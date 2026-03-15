#pragma once
#include "../../includes.h"
#include "../../utilities/hookmanager.h"
#include <dxgi.h>

// Accessor for swap chain vtable (defined in interfaces.cpp)
void** GetSwapChainVTable();

// Virtual table indexes — single source of truth for all vtable offsets
namespace VTABLE
{
    enum
    {
        /* IDXGISwapChain */
        PRESENT = 8,
        RESIZEBUFFERS = 13,

        /* InputSystem */
        ISRELATIVEMOUSEMODE = 76
    };
}

// Detour instances - one HookManager per hooked function
namespace DTR
{
    inline HookManager Present;
    inline HookManager ResizeBuffers;
    inline HookManager IsRelativeMouseMode;
    inline HookManager SDLSetRelMouseMode;
}

namespace H
{
    bool Setup();
    void Restore();

    // Original WndProc (subclassed, not detoured) — stored here because Restore() unsubclasses
    inline WNDPROC oWndProc = nullptr;
}
