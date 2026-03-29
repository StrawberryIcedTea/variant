#pragma once
#include <cstdint>

// -----------------------------------------------------------------------
// ViewMatrix_t -- 4x4 projection matrix (world -> screen transform)
// -----------------------------------------------------------------------
struct ViewMatrix_t
{
    float arrData[4][4] = {};

    float* operator[](int nIndex) { return arrData[nIndex]; }
    const float* operator[](int nIndex) const { return arrData[nIndex]; }
};

// -----------------------------------------------------------------------
// ViewMatrix -- captured from GetMatrixForView hook (no hardcoded offsets)
// -----------------------------------------------------------------------
namespace ViewMatrix
{
    // GetMatrixForView hook -- captures view matrix every frame
    void* __fastcall hkGetMatrixForView(void* pRenderGameSystem, void* pViewRender, ViewMatrix_t* pOutWorldToView,
                                        ViewMatrix_t* pOutViewToProjection, ViewMatrix_t* pOutWorldToProjection,
                                        ViewMatrix_t* pOutWorldToPixels);

    // Get the current view matrix (captured from last hook call)
    const ViewMatrix_t* Get();

    // Project a 3D world position to 2D screen coordinates.
    // Returns false if the point is behind the camera.
    bool WorldToScreen(const float* vecWorld, float& outX, float& outY);
}
