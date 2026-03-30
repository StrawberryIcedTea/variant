#pragma once
#include "../../../sdk/material.h"

namespace Chams
{
    bool Setup();
    void OnGeneratePrimitives(GeneratePrimitivesFn oFn, void* pThis,
                               CSceneAnimatableObject* pObject, void* pUnk,
                               CMeshPrimitiveOutputBuffer* pBuf);
}
