#pragma once
#include "../../../includes.h"
#include <dxgi.h>
#include <atomic>

// D3D11 render target + ImGui lifecycle + Present/ResizeBuffers hook handlers
namespace Render
{
    // Render target helpers
    void CreateRenderTarget();
    void ReleaseRenderTarget();

    // ImGui init/shutdown (called from Present hook on first frame)
    bool InitImGui(IDXGISwapChain* pSwapChain);
    void ShutdownImGui();

    // Hook handlers (wired into DTR:: by H::Setup)
    HRESULT WINAPI hkPresent(IDXGISwapChain* pSwapChain, UINT syncInterval, UINT flags);
    HRESULT WINAPI hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT bufferCount, UINT width, UINT height,
                                   DXGI_FORMAT newFormat, UINT flags);

    // WndProc subclass — forwards to ImGui, blocks game input when menu open
    LRESULT CALLBACK hkWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Render target used by Present
    inline ID3D11RenderTargetView* pRenderTargetView = nullptr;

    // True once ImGui has been initialized.
    // Atomic: read by Present thread, written by shutdown thread.
    inline std::atomic<bool> bImGuiInitialized = false;

}
