#pragma once
#include "includes.h"
#include "../utilities/hookmanager.h"
#include <dxgi.h>

// Accessor for swap chain vtable (defined in interfaces.cpp)
void** GetSwapChainVTable();

// Detour instances - one HookManager per hooked function
namespace DTR
{
inline HookManager Present;
inline HookManager ResizeBuffers;
inline HookManager IsRelativeMouseMode;
inline HookManager SDLSetRelMouseMode;
}

namespace H
{
bool Setup();
void Destroy();

// Original WndProc (subclassed, not detoured)
inline WNDPROC oWndProc = nullptr;

// Hook handlers
HRESULT WINAPI hkPresent(IDXGISwapChain* pSwapChain, UINT syncInterval, UINT flags);
HRESULT WINAPI hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT bufferCount, UINT width, UINT height,
                               DXGI_FORMAT newFormat, UINT flags);
LRESULT CALLBACK hkWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
}
