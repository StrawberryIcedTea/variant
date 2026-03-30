// Hook orchestrator — Setup/Restore

#include "hooks.h"
#include "render/render.h"
#include "cursor/cursor.h"
#include "createmove/createmove.h"
#include "prediction/prediction.h"
#include "viewmatrix/viewmatrix.h"
#include "generateprimitives/generateprimitives.h"
#include "../../utilities/debug.h"
#include "../../utilities/memory.h"
#include "../../utilities/input.h"

bool H::Setup()
{
    if (MH_Initialize() != MH_OK)
        return false;

    if (!DTR::Present.Create(I::pSwapChainVTable[VTABLE::PRESENT], &Render::hkPresent))
        return false;

    if (!DTR::ResizeBuffers.Create(I::pSwapChainVTable[VTABLE::RESIZEBUFFERS], &Render::hkResizeBuffers))
        return false;

    if (!DTR::IsRelativeMouseMode.Create(M::GetVFunc(I::pInputSystem, VTABLE::ISRELATIVEMOUSEMODE),
                                         &Cursor::hkIsRelativeMouseMode))
        return false;

    if (!DTR::SDLSetRelMouseMode.Create(M::GetExport("SDL3.dll", "SDL_SetWindowRelativeMouseMode"),
                                        &Cursor::hkSDL_SetWindowRelativeMouseMode))
        return false;

    if (!DTR::CreateMove.Create(M::GetVFunc(I::pCSGOInput, VTABLE::CREATEMOVE), &CreateMove::hkCreateMove))
        return false;

    if (!DTR::Prediction.Create(M::GetVFunc(I::pCSGOInput, VTABLE::PREDICTION), &Prediction::hkPredictionSimulation))
        return false;

    if (!DTR::CreateMoveInner.Create(M::GetVFunc(I::pCSGOInput, VTABLE::CREATEMOVE_INNER),
                                     &CreateMove::hkCreateMoveInner))
        return false;

    if (!DTR::GetMatrixForView.Create(I::pGetMatrixForView, &ViewMatrix::hkGetMatrixForView))
        return false;

    if (!DTR::GeneratePrimitives.Create(I::pAnimatableSceneObjectVTable[VTABLE::GENERATE_PRIMITIVES],
                                        &GeneratePrimitives::hkGeneratePrimitives))
        return false;

    return true;
}

void H::Restore()
{
    Input::Stop();
    Render::ShutdownImGui();

    if (oWndProc && I::hGameWindow)
        SetWindowLongPtrA(I::hGameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(oWndProc));

    Render::ReleaseRenderTarget();

    DTR::GeneratePrimitives.Remove();
    DTR::GetMatrixForView.Remove();
    DTR::CreateMoveInner.Remove();
    DTR::Prediction.Remove();
    DTR::CreateMove.Remove();
    DTR::SDLSetRelMouseMode.Remove();
    DTR::IsRelativeMouseMode.Remove();
    DTR::Present.Remove();
    DTR::ResizeBuffers.Remove();

    I::pContext->Release();
    I::pContext = nullptr;
    I::pDevice->Release();
    I::pDevice = nullptr;

    MH_Uninitialize();
}
