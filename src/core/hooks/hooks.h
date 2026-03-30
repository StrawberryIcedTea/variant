#pragma once
#include "../../includes.h"
#include "../../utilities/hookmanager.h"
#include "../interfaces.h"

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
        PREDICTION = 16,
        CREATEMOVE_INNER = 22,

        /* CAnimatableSceneObject (scenesystem.dll) */
        GENERATE_PRIMITIVES = 4,
    };
}

namespace DTR
{
    inline HookManager Present;
    inline HookManager ResizeBuffers;
    inline HookManager IsRelativeMouseMode;
    inline HookManager SDLSetRelMouseMode;
    inline HookManager CreateMove;
    inline HookManager Prediction;
    inline HookManager CreateMoveInner;
    inline HookManager GetMatrixForView;
    inline HookManager GeneratePrimitives;
}

namespace H
{
    bool Setup();
    void Restore();

    inline WNDPROC oWndProc = nullptr;
}
