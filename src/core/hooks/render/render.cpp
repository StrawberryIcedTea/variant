// D3D11 render target + ImGui lifecycle + Present/ResizeBuffers hooks

#include "render.h"
#include "../../interfaces.h"
#include "../../menu/menu.h"
#include "../hooks.h"
#include "../cursor/cursor.h"
#include "../../../utilities/inputhook.h"

#include "../../../../dependencies/imgui/imgui.h"
#include "../../../../dependencies/imgui/imgui_impl_win32.h"
#include "../../../../dependencies/imgui/imgui_impl_dx11.h"

// ---------------------------------------------------------------------------
// Render target helpers
// ---------------------------------------------------------------------------

void Render::CreateRenderTarget()
{
    if (!I::pSwapChain || !I::pDevice)
        return;

    ID3D11Texture2D* pBackBuffer = nullptr;
    I::pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    if (pBackBuffer)
    {
        if (FAILED(I::pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTargetView)))
            pRenderTargetView = nullptr;
        pBackBuffer->Release();
    }
}

void Render::ReleaseRenderTarget()
{
    if (pRenderTargetView)
    {
        pRenderTargetView->Release();
        pRenderTargetView = nullptr;
    }
}

// ---------------------------------------------------------------------------
// ImGui lifecycle
// ---------------------------------------------------------------------------

bool Render::InitImGui(IDXGISwapChain* pSwapChain)
{
    if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&I::pDevice))))
        return false;

    I::pDevice->GetImmediateContext(&I::pContext);
    I::pSwapChain = pSwapChain;

    CreateRenderTarget();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    // Use the swap chain's output window - FindWindowA("SDL_app") may find wrong HWND
    DXGI_SWAP_CHAIN_DESC scDesc{};
    if (SUCCEEDED(pSwapChain->GetDesc(&scDesc)))
        I::hGameWindow = scDesc.OutputWindow;

    ImGui_ImplWin32_Init(I::hGameWindow);
    ImGui_ImplDX11_Init(I::pDevice, I::pContext);

    H::oWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrA(I::hGameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Render::hkWndProc)));

    return true;
}

void Render::ShutdownImGui()
{
    if (!bImGuiInitialized.exchange(false))
        return;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

// ---------------------------------------------------------------------------
// Hook handlers
// ---------------------------------------------------------------------------

HRESULT WINAPI Render::hkPresent(IDXGISwapChain* pSwapChain, UINT syncInterval, UINT flags)
{
    auto oPresent = DTR::Present.Original<decltype(&hkPresent)>();

    bool bInit = bImGuiInitialized.load();
    if (!bInit)
    {
        bInit = InitImGui(pSwapChain);
        bImGuiInitialized.store(bInit);
    }

    if (bInit)
    {
        static bool s_bWasOpen = Menu::bOpen.load();
        static POINT s_lastMenuCursor = {-1, -1};

        if (Input::bInsertPressed.exchange(false))
            Menu::bOpen = !Menu::bOpen.load();

        const bool bOpenNow = Menu::bOpen.load();
        const bool bJustOpened = bOpenNow && !s_bWasOpen;
        const bool bJustClosed = !bOpenNow && s_bWasOpen;
        const bool bRelative = Cursor::bGameRelativeMode.load();

        if (bJustClosed && bRelative)
            GetCursorPos(&s_lastMenuCursor);

        // Handle cursor mode on open/close transitions
        if (s_bWasOpen != bOpenNow)
            Cursor::SetMenuMode(bOpenNow);
        else if (bOpenNow)
            Cursor::SuppressSDLRelativeMode();

        // Restore the OS cursor shape after menu close (lobby) — our WM_SETCURSOR
        // suppression left it NULL, and Windows won't refresh it until mouse moves.
        if (bJustClosed && !bRelative && I::hGameWindow)
            SendMessageA(I::hGameWindow, WM_SETCURSOR, (WPARAM)I::hGameWindow, MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));

        if (bJustOpened && bRelative && I::hGameWindow)
        {
            // Match: restore last menu cursor (or center on first open)
            POINT warp;
            if (s_lastMenuCursor.x >= 0)
            {
                warp = s_lastMenuCursor;
            }
            else
            {
                RECT rc;
                if (GetClientRect(I::hGameWindow, &rc))
                {
                    warp = {(rc.right - rc.left) / 2, (rc.bottom - rc.top) / 2};
                    ClientToScreen(I::hGameWindow, &warp);
                }
                else
                    warp = {0, 0};
            }
            SetCursorPos(warp.x, warp.y);
        }

        // In a match use ImGui's software cursor so the OS cursor is never
        // visible — eliminates any DWM-composited glide on close.
        // In lobby the OS cursor is shared, so leave it as-is.
        {
            ImGuiIO& io = ImGui::GetIO();
            io.MouseDrawCursor = bOpenNow;
        }

        s_bWasOpen = bOpenNow;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        // On close: clear ImGui mouse position so the cursor doesn't flash at its last spot
        if (bJustClosed)
        {
            ImGuiIO& io = ImGui::GetIO();
            io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
        }

        // SDL3 steals all input — feed mouse state directly to ImGui
        if (bOpenNow)
        {
            ImGuiIO& io = ImGui::GetIO();

            POINT pt;
            if (GetCursorPos(&pt) && ScreenToClient(I::hGameWindow, &pt))
                io.AddMousePosEvent(static_cast<float>(pt.x), static_cast<float>(pt.y));

            io.AddMouseButtonEvent(0, Input::bMouseLeft.load());
            io.AddMouseButtonEvent(1, Input::bMouseRight.load());
            io.AddMouseButtonEvent(2, Input::bMouseMiddle.load());
        }

        ImGui::NewFrame();
        Menu::Render();
        ImGui::EndFrame();
        ImGui::Render();

        I::pContext->OMSetRenderTargets(1, &pRenderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    return oPresent(pSwapChain, syncInterval, flags);
}

HRESULT WINAPI Render::hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT bufferCount, UINT width, UINT height,
                                       DXGI_FORMAT newFormat, UINT flags)
{
    auto oResizeBuffers = DTR::ResizeBuffers.Original<decltype(&hkResizeBuffers)>();

    ReleaseRenderTarget();
    HRESULT hr = oResizeBuffers(pSwapChain, bufferCount, width, height, newFormat, flags);
    if (SUCCEEDED(hr))
        CreateRenderTarget();

    return hr;
}

// ---------------------------------------------------------------------------
// WndProc — forwards to ImGui, blocks game input when menu open
// ---------------------------------------------------------------------------

LRESULT CALLBACK Render::hkWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // When menu is open, block mouse input from reaching SDL/game.
    if (bImGuiInitialized.load() && Menu::bOpen)
    {
        switch (msg)
        {
        // Suppress OS cursor while menu is open in a match — we use ImGui's
        // software cursor instead to avoid DWM-composited glide on transitions.
        case WM_SETCURSOR:
            SetCursor(nullptr);
            return TRUE;
        case WM_INPUT:
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_XBUTTONDBLCLK:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
            return 0;
        default:
            break;
        }
    }

    return CallWindowProcA(H::oWndProc, hWnd, msg, wParam, lParam);
}
