// PredictionSimulation hook — CCSGOInput vtable[16]
// Saves pre-prediction player state, calls original (engine predicts),
// then exposes predicted flags for movement features.

#include "prediction.h"
#include "../hooks.h"
#include "../../interfaces.h"
#include "../../../sdk/entity.h"
#include "../../../sdk/interfaces/iglobalvars.h"
#include "../../../utilities/debug.h"

void __fastcall Prediction::hkPredictionSimulation(void* pInput, int nSlot, CUserCmd* pCmd)
{
    auto oOriginal = DTR::Prediction.Original<decltype(&hkPredictionSimulation)>();

    auto* pLocal = EntitySystem::GetLocalPlayerPawn();
    if (!pLocal || pLocal->GetHealth() <= 0 || !I::pGlobalVars)
    {
        nPredictedFlags = 0;
        bPredictedFlagsValid = false;
        return oOriginal(pInput, nSlot, pCmd);
    }

    // Save pre-prediction flags (interpolated render state)
    nPredictedFlags = pLocal->GetFlags();

    // Save global timing state
    float flOldCurtime = I::pGlobalVars->flCurtime;
    float flOldFrameTime = I::pGlobalVars->flFrameTime;

    // Call original — engine runs prediction simulation
    oOriginal(pInput, nSlot, pCmd);

    // After prediction: read the predicted flags
    // The engine has now updated the entity's state to reflect what will happen this tick
    nPredictedFlags = pLocal->GetFlags();
    bPredictedFlagsValid = true;

    // Restore global timing state (prevent drift)
    I::pGlobalVars->flCurtime = flOldCurtime;
    I::pGlobalVars->flCurtime2 = flOldCurtime;
    I::pGlobalVars->flFrameTime = flOldFrameTime;
    I::pGlobalVars->flFrameTime2 = flOldFrameTime;
}
