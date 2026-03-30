#pragma once
#include "../includes.h"
#include "../sdk/interfaces/iglobalvars.h"
#include "../sdk/interfaces/iengineclient.h"
#include "../sdk/material.h"

// Game interfaces discovered at runtime
namespace I
{
    bool Setup();

    // D3D11 - found via DXGI swap chain
    inline IDXGISwapChain* pSwapChain = nullptr;
    inline ID3D11Device* pDevice = nullptr;
    inline ID3D11DeviceContext* pContext = nullptr;
    inline HWND hGameWindow = nullptr;

    // Swap chain vtable — captured from dummy device during Setup
    inline void** pSwapChainVTable = nullptr;

    // InputSystem - for cursor control (IsRelativeMouseMode vtable hook)
    inline void* pInputSystem = nullptr;

    // IEngineClient — Source2EngineToClient001 from engine2.dll
    inline IEngineClient* pEngine = nullptr;

    // Game entity access
    inline void* pGameResourceService = nullptr;  // IGameResourceService from engine2.dll
    inline void** ppEntitySystem      = nullptr;  // &CGameEntitySystem* global in client.dll
                                                   // Dereferenced each access — entity system
                                                   // is recreated on map load, direct ptr goes stale

    // CCSGOInput — hardcoded offset from cs2-dumper (pattern scans unreliable)
    // Used for vtable hooks: CreateMove[5], Prediction[16], CreateMoveInner[22]
    inline void* pCSGOInput = nullptr;

    // IEngineCVar — convar system (tier0.dll "VEngineCvar007")
    inline void* pCvar = nullptr;

    // IGlobalVars — engine timing state (curtime, frametime, tickcount)
    inline IGlobalVars* pGlobalVars = nullptr;

    // GetMatrixForView — pattern-scanned function, non-fatal (WorldToScreen disabled if null)
    inline void* pGetMatrixForView = nullptr;

    // CAnimatableSceneObject vtable (scenesystem.dll) — RTTI scan, stable across updates
    inline void** pAnimatableSceneObjectVTable = nullptr;

    // Material creation — required for chams; non-fatal if stale
    inline void*       pMaterialSystem  = nullptr;  // VMaterialSystem2_001
    inline LoadKV3Fn   fnLoadKV3        = nullptr;  // tier0.dll export
    inline CreateMatFn fnCreateMaterial = nullptr;  // materialsystem2.dll pattern

    inline float GetTickInterval()
    {
        return (pGlobalVars && pGlobalVars->flIntervalPerTick > 0.f) ? pGlobalVars->flIntervalPerTick : 1.f / 64.f;
    }
}
