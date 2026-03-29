// ViewMatrix -- captures world-to-projection matrix via GetMatrixForView hook

#include "viewmatrix.h"
#include "../hooks.h"
#include <imgui/imgui.h>

// Captured matrix -- updated every frame by the hook
static ViewMatrix_t s_cachedMatrix = {};
static bool s_bHookActive = false;

// -----------------------------------------------------------------------
// hkGetMatrixForView -- capture the world-to-projection matrix
// -----------------------------------------------------------------------
void* __fastcall ViewMatrix::hkGetMatrixForView(void* pRenderGameSystem, void* pViewRender,
                                                ViewMatrix_t* pOutWorldToView, ViewMatrix_t* pOutViewToProjection,
                                                ViewMatrix_t* pOutWorldToProjection, ViewMatrix_t* pOutWorldToPixels)
{
    auto oGetMatrixForView = DTR::GetMatrixForView.Original<decltype(&hkGetMatrixForView)>();
    void* result = oGetMatrixForView(pRenderGameSystem, pViewRender, pOutWorldToView, pOutViewToProjection,
                                     pOutWorldToProjection, pOutWorldToPixels);

    if (pOutWorldToProjection)
    {
        s_cachedMatrix = *pOutWorldToProjection;
        s_bHookActive = true;
    }

    return result;
}

// -----------------------------------------------------------------------
// Get -- returns hooked matrix, or nullptr if hook not yet active
// -----------------------------------------------------------------------
const ViewMatrix_t* ViewMatrix::Get()
{
    if (s_bHookActive)
        return &s_cachedMatrix;

    return nullptr;
}

// -----------------------------------------------------------------------
// WorldToScreen -- project 3D world position to 2D screen coordinates
// -----------------------------------------------------------------------
bool ViewMatrix::WorldToScreen(const float* vecWorld, float& outX, float& outY)
{
    const auto* m = Get();
    if (!m)
        return false;

    float w = (*m)[3][0] * vecWorld[0] + (*m)[3][1] * vecWorld[1] + (*m)[3][2] * vecWorld[2] + (*m)[3][3];
    if (w < 0.001f)
        return false;

    float invW = 1.f / w;
    float nx = ((*m)[0][0] * vecWorld[0] + (*m)[0][1] * vecWorld[1] + (*m)[0][2] * vecWorld[2] + (*m)[0][3]) * invW;
    float ny = ((*m)[1][0] * vecWorld[0] + (*m)[1][1] * vecWorld[1] + (*m)[1][2] * vecWorld[2] + (*m)[1][3]) * invW;

    const ImVec2 display = ImGui::GetIO().DisplaySize;
    outX = (display.x * 0.5f) + (nx * display.x * 0.5f);
    outY = (display.y * 0.5f) - (ny * display.y * 0.5f);

    return true;
}
