#pragma once
#include "../../includes.h"
#include "../../utilities/hookmanager.h"
#include "../interfaces.h"
#include <dxgi.h>

namespace VTABLE
{
    enum
    {
        /* IDXGISwapChain */
        PRESENT = 8,
        RESIZEBUFFERS = 13,

        /* InputSystem */
        ISRELATIVEMOUSEMODE = 76,

        /* CCSGOInput */
        CREATEMOVE = 5,
        CREATEMOVE_INNER = 22
    };
}

namespace DTR
{
    inline HookManager Present;
    inline HookManager ResizeBuffers;
    inline HookManager IsRelativeMouseMode;
    inline HookManager SDLSetRelMouseMode;
    inline HookManager CreateMove;
    inline HookManager CreateMoveInner;
}

namespace H
{
    bool Setup();
    void Restore();

    inline WNDPROC oWndProc = nullptr;
}
