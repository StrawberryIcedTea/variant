#pragma once
#include "includes.h"

// Game interfaces discovered at runtime
namespace I
{
bool Setup();

// D3D11 - found via DXGI swap chain
inline IDXGISwapChain* pSwapChain = nullptr;
inline ID3D11Device* pDevice = nullptr;
inline ID3D11DeviceContext* pContext = nullptr;
inline HWND hGameWindow = nullptr;

// InputSystem - for cursor control (IsRelativeMouseMode vtable hook)
inline void* pInputSystem = nullptr;
}
