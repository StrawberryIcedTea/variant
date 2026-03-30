#include "generateprimitives.h"
#include "../hooks.h"
#include "../../../features/visuals/chams/chams.h"

void __fastcall GeneratePrimitives::hkGeneratePrimitives(void* pThis, CSceneAnimatableObject* pObject,
                                                          void* pUnk, CMeshPrimitiveOutputBuffer* pBuf)
{
    Chams::OnGeneratePrimitives(
        DTR::GeneratePrimitives.Original<GeneratePrimitivesFn>(),
        pThis, pObject, pUnk, pBuf);
}
