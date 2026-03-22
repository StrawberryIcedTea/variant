#pragma once
#include "../includes.h"

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
    void** GetSwapChainVTable();

    // InputSystem - for cursor control (IsRelativeMouseMode vtable hook)
    inline void* pInputSystem = nullptr;

    // Game entity access
    inline void* pGameResourceService = nullptr; // IGameResourceService from engine2.dll
    inline void* pEntitySystem = nullptr;        // CGameEntitySystem* (offset 0x58 from resource service)

    // CCSGOInput object pointer — resolved via dwCSGOInput offset, used for vtable[5] hooking
    inline void* pCSGOInput = nullptr;
}
