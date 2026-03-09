// Hook implementations
// D3D11 Present + ResizeBuffers + WndProc for ImGui overlay

#include "hooks.h"
#include "interfaces.h"
#include "menu.h"
#include "../utilities/debug.h"
#include "../utilities/memory.h"
#include "../utilities/inputhook.h"
#include <atomic>

#include "../../dependencies/imgui/imgui.h"
#include "../../dependencies/imgui/imgui_impl_win32.h"
#include "../../dependencies/imgui/imgui_impl_dx11.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ---------------------------------------------------------------------------
// Cursor control - two layers:
// 1. InputSystem::IsRelativeMouseMode (vtable 76) - game-level control
// 2. SDL_SetWindowRelativeMouseMode - SDL-level control (backup)
// Both force relative mouse mode OFF when menu is open.
// ---------------------------------------------------------------------------

// --- Layer 1: InputSystem vtable hook ---
using fnIsRelativeMouseMode = void*(__fastcall*)(void*, bool);
static bool s_bGameRelativeMode = true;

static void* __fastcall hkIsRelativeMouseMode(void* pThisptr, bool bActive)
{
    auto oFunc = DTR::IsRelativeMouseMode.Original<fnIsRelativeMouseMode>();
    s_bGameRelativeMode = bActive;
    if (Menu::bOpen)
        return oFunc(pThisptr, false);
    return oFunc(pThisptr, bActive);
}

// --- Layer 2: SDL3 hook ---
using fnSDL_SetWindowRelativeMouseMode = int (*)(void*, int);
using fnSDL_GetKeyboardFocus = void* (*)();
static fnSDL_GetKeyboardFocus pSDL_GetKeyboardFocus = nullptr;

static int hkSDL_SetWindowRelativeMouseMode(void* pWindow, int bEnabled)
{
    auto oFunc = DTR::SDLSetRelMouseMode.Original<fnSDL_SetWindowRelativeMouseMode>();
    if (Menu::bOpen)
        return oFunc(pWindow, 0);
    return oFunc(pWindow, bEnabled);
}

static void ForceRelativeMouseMode(bool bRelative)
{
    auto oFunc = DTR::SDLSetRelMouseMode.Original<fnSDL_SetWindowRelativeMouseMode>();
    if (!oFunc || !pSDL_GetKeyboardFocus)
        return;
    void* pWindow = pSDL_GetKeyboardFocus();
    if (!pWindow)
        return;
    oFunc(pWindow, bRelative ? 1 : 0);
}

// ---------------------------------------------------------------------------
// Render target helpers (shared by Present + ResizeBuffers)
// ---------------------------------------------------------------------------
static ID3D11RenderTargetView* s_pRenderTargetView = nullptr;

static void CreateRenderTarget()
{
    if (!I::pSwapChain || !I::pDevice)
        return;

    ID3D11Texture2D* pBackBuffer = nullptr;
    I::pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    if (pBackBuffer)
    {
        if (FAILED(I::pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &s_pRenderTargetView)))
            s_pRenderTargetView = nullptr;
        pBackBuffer->Release();
    }
}

static void ReleaseRenderTarget()
{
    if (s_pRenderTargetView)
    {
        s_pRenderTargetView->Release();
        s_pRenderTargetView = nullptr;
    }
}

// ---------------------------------------------------------------------------
// ImGui lifecycle (init once on first Present, shutdown on Destroy)
// ---------------------------------------------------------------------------
static std::atomic<bool> s_bImGuiInitialized = false;

static bool InitImGui(IDXGISwapChain* pSwapChain)
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
        SetWindowLongPtrA(I::hGameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&H::hkWndProc)));

    Input::Start();

    return true;
}

static void ShutdownImGui()
{
    if (!s_bImGuiInitialized)
        return;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    s_bImGuiInitialized = false;
}

// ---------------------------------------------------------------------------
// Hook handlers
// ---------------------------------------------------------------------------

HRESULT WINAPI H::hkPresent(IDXGISwapChain* pSwapChain, UINT syncInterval, UINT flags)
{
    auto oPresent = DTR::Present.Original<decltype(&hkPresent)>();

    if (!s_bImGuiInitialized)
        s_bImGuiInitialized = InitImGui(pSwapChain);

    if (s_bImGuiInitialized)
    {
        static bool s_bWasOpen = Menu::bOpen.load();

        if (Input::bInsertPressed.exchange(false))
            Menu::bOpen = !Menu::bOpen.load();

        const bool bOpenNow = Menu::bOpen.load();

        // Detect any open→closed transition (INSERT key or ImGui X button)
        if (s_bWasOpen && !bOpenNow)
        {
            // Layer 1: InputSystem - restore game's relative mouse mode
            auto oIsRMM = DTR::IsRelativeMouseMode.Original<fnIsRelativeMouseMode>();
            if (oIsRMM && I::pInputSystem)
                oIsRMM(I::pInputSystem, s_bGameRelativeMode);

            // Layer 2: SDL direct - re-enable relative mouse
            ForceRelativeMouseMode(true);
        }

        // Detect closed→open transition
        if (!s_bWasOpen && bOpenNow)
        {
            auto oIsRMM = DTR::IsRelativeMouseMode.Original<fnIsRelativeMouseMode>();
            if (oIsRMM && I::pInputSystem)
                oIsRMM(I::pInputSystem, false);

            ForceRelativeMouseMode(false);
        }

        s_bWasOpen = bOpenNow;

        // Keep cursor free every frame while menu open (game may re-enable)
        if (bOpenNow)
            ForceRelativeMouseMode(false);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        // SDL3 steals all input - WndProc never receives WM_LBUTTONDOWN etc.
        // Feed mouse position (GetCursorPos) and button state (WH_MOUSE_LL)
        // directly to ImGui via the event API before NewFrame processes them.
        if (Menu::bOpen)
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

        I::pContext->OMSetRenderTargets(1, &s_pRenderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    return oPresent(pSwapChain, syncInterval, flags);
}

HRESULT WINAPI H::hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT bufferCount, UINT width, UINT height,
                                  DXGI_FORMAT newFormat, UINT flags)
{
    auto oResizeBuffers = DTR::ResizeBuffers.Original<decltype(&hkResizeBuffers)>();

    ReleaseRenderTarget();
    HRESULT hr = oResizeBuffers(pSwapChain, bufferCount, width, height, newFormat, flags);
    if (SUCCEEDED(hr))
        CreateRenderTarget();

    return hr;
}

LRESULT CALLBACK H::hkWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (s_bImGuiInitialized)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam) || Menu::bOpen)
            return 1;
    }

    return CallWindowProcA(oWndProc, hWnd, msg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// Setup / Destroy
// ---------------------------------------------------------------------------

bool H::Setup()
{
    if (MH_Initialize() != MH_OK)
    {
        C::Print("[hooks] MH_Initialize failed");
        return false;
    }

    auto** vtable = GetSwapChainVTable();
    if (!vtable)
    {
        C::Print("[hooks] swap chain vtable is null");
        return false;
    }

    // Present = slot 8, ResizeBuffers = slot 13
    DTR::Present.pTarget = vtable[8];
    DTR::ResizeBuffers.pTarget = vtable[13];

    if (!DTR::Present.Create(&hkPresent))
    {
        C::Print("[hooks] failed to hook Present");
        return false;
    }
    if (!DTR::ResizeBuffers.Create(&hkResizeBuffers))
    {
        C::Print("[hooks] failed to hook ResizeBuffers");
        return false;
    }

    // Cursor control: hook InputSystem vtable + SDL3
    if (I::pInputSystem)
    {
        DTR::IsRelativeMouseMode.pTarget = M::GetVFunc(I::pInputSystem, 76);
        DTR::IsRelativeMouseMode.Create(reinterpret_cast<void*>(&hkIsRelativeMouseMode));
    }

    HMODULE hSDL3 = GetModuleHandleA("SDL3.dll");
    if (hSDL3)
    {
        pSDL_GetKeyboardFocus = reinterpret_cast<fnSDL_GetKeyboardFocus>(GetProcAddress(hSDL3, "SDL_GetKeyboardFocus"));

        auto pSDLTarget = GetProcAddress(hSDL3, "SDL_SetWindowRelativeMouseMode");
        if (pSDLTarget)
        {
            DTR::SDLSetRelMouseMode.pTarget = reinterpret_cast<void*>(pSDLTarget);
            DTR::SDLSetRelMouseMode.Create(reinterpret_cast<void*>(&hkSDL_SetWindowRelativeMouseMode));
        }
    }

    return true;
}

void H::Destroy()
{
    if (oWndProc && I::hGameWindow)
        SetWindowLongPtrA(I::hGameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(oWndProc));

    Input::Stop();
    ShutdownImGui();
    ReleaseRenderTarget();

    DTR::SDLSetRelMouseMode.Destroy();
    DTR::IsRelativeMouseMode.Destroy();
    DTR::Present.Destroy();
    DTR::ResizeBuffers.Destroy();
    MH_Uninitialize();
}
