#pragma once
#include "../../../includes.h"
#include "../../../sdk/entity.h"

struct CUserCmd;

// PredictionSimulation hook on CCSGOInput vtable[16]
// Fires after CreateMove — engine runs one tick of movement prediction.
// We save pre-prediction flags and expose predicted state to features.
namespace Prediction
{
    void __fastcall hkPredictionSimulation(void* pInput, int nSlot, CUserCmd* pCmd);

    // Predicted player flags — read after engine prediction runs.
    // bPredictedFlagsValid distinguishes "flags = 0 (airborne)" from "not yet set".
    inline uint32_t nPredictedFlags = 0;
    inline bool bPredictedFlagsValid = false;

    inline uint32_t GetFlags(C_BaseEntity* pLocal)
    {
        return bPredictedFlagsValid ? nPredictedFlags : pLocal->GetFlags();
    }
}
