// Hook orchestrator — Setup/Restore

#include "hooks.h"
#include "render/render.h"
#include "cursor/cursor.h"
#include "createmove/createmove.h"
#include "../../utilities/debug.h"
#include "../../utilities/memory.h"
#include "../../utilities/inputhook.h"

bool H::Setup()
{
    if (MH_Initialize() != MH_OK)
    {
        C::Print("[hooks] MH_Initialize failed");
        return false;
    }

    auto** vtable = I::GetSwapChainVTable();
    if (!vtable)
    {
        C::Print("[hooks] swap chain vtable is null");
        return false;
    }

    if (!DTR::Present.Create(vtable[VTABLE::PRESENT], &Render::hkPresent))
        return false;
    if (!DTR::ResizeBuffers.Create(vtable[VTABLE::RESIZEBUFFERS], &Render::hkResizeBuffers))
        return false;

    if (I::pInputSystem)
    {
        if (!DTR::IsRelativeMouseMode.Create(M::GetVFunc(I::pInputSystem, VTABLE::ISRELATIVEMOUSEMODE),
                                             reinterpret_cast<void*>(&Cursor::hkIsRelativeMouseMode)))
            C::Print("[hooks] failed to hook IsRelativeMouseMode (non-fatal)");
    }

    HMODULE hSDL3 = GetModuleHandleA("SDL3.dll");
    if (hSDL3)
    {
        auto pSDLTarget = GetProcAddress(hSDL3, "SDL_SetWindowRelativeMouseMode");
        if (pSDLTarget)
        {
            if (!DTR::SDLSetRelMouseMode.Create(reinterpret_cast<void*>(pSDLTarget),
                                                reinterpret_cast<void*>(&Cursor::hkSDL_SetWindowRelativeMouseMode)))
                C::Print("[hooks] failed to hook SDL_SetWindowRelativeMouseMode (non-fatal)");
        }
    }

    // CCSGOInput vtable hooks
    if (I::pCSGOInput)
    {
        if (!DTR::CreateMove.Create(M::GetVFunc(I::pCSGOInput, VTABLE::CREATEMOVE),
                                    reinterpret_cast<void*>(&CreateMove::hkCreateMove)))
            C::Print("[hooks] failed to hook CreateMove (non-fatal)");

        if (!DTR::CreateMoveInner.Create(M::GetVFunc(I::pCSGOInput, VTABLE::CREATEMOVE_INNER),
                                         reinterpret_cast<void*>(&CreateMove::hkCreateMoveInner)))
            C::Print("[hooks] failed to hook CreateMoveInner (non-fatal)");
    }

    return true;
}

void H::Restore()
{
    Input::Stop();
    Render::ShutdownImGui();

    if (oWndProc && I::hGameWindow)
        SetWindowLongPtrA(I::hGameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(oWndProc));

    Render::ReleaseRenderTarget();

    DTR::CreateMoveInner.Remove();
    DTR::CreateMove.Remove();
    DTR::SDLSetRelMouseMode.Remove();
    DTR::IsRelativeMouseMode.Remove();
    DTR::Present.Remove();
    DTR::ResizeBuffers.Remove();

    if (I::pContext)
    {
        I::pContext->Release();
        I::pContext = nullptr;
    }
    if (I::pDevice)
    {
        I::pDevice->Release();
        I::pDevice = nullptr;
    }

    MH_Uninitialize();
}
