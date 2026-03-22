// Interface discovery for CS2
// D3D11 device/swapchain via dummy window + Source 2 CreateInterface

#include "interfaces.h"
#include "../sdk/entitysystem.h"
#include "../sdk/usercmd.h"
#include "../utilities/debug.h"
#include "../utilities/memory.h"
#include <format>

// Create a temporary D3D11 device + swapchain to grab the vtable pointers.
// Standard kiero approach: create dummy device, read vtable, destroy.
static void** s_swapChainVTable = nullptr;

static bool GetD3D11VTables()
{
    WNDCLASSEX wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "VariantDummy";
    RegisterClassEx(&wc);

    HWND hWnd = CreateWindowEx(0, wc.lpszClassName, "", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr,
                               wc.hInstance, nullptr);

    if (!hWnd)
    {
        C::Print("[interfaces] failed to create dummy window");
        return false;
    }

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.Width = 2;
    sd.BufferDesc.Height = 2;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    IDXGISwapChain* pSc = nullptr;
    ID3D11Device* pDev = nullptr;
    ID3D11DeviceContext* pCtx = nullptr;

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
                                               D3D11_SDK_VERSION, &sd, &pSc, &pDev, &featureLevel, &pCtx);

    if (FAILED(hr))
    {
        C::Print(std::format("[interfaces] D3D11CreateDeviceAndSwapChain failed: {:#x}", static_cast<unsigned>(hr)));
        DestroyWindow(hWnd);
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return false;
    }

    s_swapChainVTable = *reinterpret_cast<void***>(pSc);

    pSc->Release();
    pDev->Release();
    pCtx->Release();
    DestroyWindow(hWnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return true;
}

// Source 2 CreateInterface export
using CreateInterfaceFn = void* (*)(const char* pName, int* pReturnCode);

static void* CaptureInterface(const char* moduleName, const char* interfaceName)
{
    HMODULE hModule = GetModuleHandleA(moduleName);
    if (!hModule)
        return nullptr;

    auto pCreateInterface = reinterpret_cast<CreateInterfaceFn>(GetProcAddress(hModule, "CreateInterface"));
    if (!pCreateInterface)
        return nullptr;

    int returnCode = 0;
    void* pInterface = pCreateInterface(interfaceName, &returnCode);
    return pInterface;
}

bool I::Setup()
{
    if (!GetD3D11VTables())
        return false;

    // Early window handle - FindWindowA("SDL_app") may match a helper window.
    // InitImGui() overwrites this with the correct HWND from the swap chain.
    hGameWindow = FindWindowA("SDL_app", nullptr);
    if (!hGameWindow)
    {
        C::Print("[interfaces] game window not found (SDL_app)");
        return false;
    }

    // InputSystem - for cursor control
    pInputSystem = CaptureInterface("inputsystem.dll", "InputSystemVersion001");
    // non-fatal: overlay works without cursor hooks

    // GameResourceService - needed for entity access (bhop, ESP, etc.)
    pGameResourceService = CaptureInterface("engine2.dll", "GameResourceServiceClientV001");
    if (!pGameResourceService)
    {
        C::Print("[interfaces] FAILED: GameResourceServiceClientV001 not found");
        return false;
    }
    C::Print(
        std::format("[interfaces] IGameResourceService: {:#x}", reinterpret_cast<uintptr_t>(pGameResourceService)));

    // CGameEntitySystem at IGameResourceService+0x58
    pEntitySystem =
        *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(pGameResourceService) + Offsets::pEntitySystem);
    if (!pEntitySystem)
    {
        C::Print("[interfaces] WARNING: CGameEntitySystem is null (may not be initialized yet)");
    }
    else
    {
        C::Print(std::format("[interfaces] CGameEntitySystem: {:#x}", reinterpret_cast<uintptr_t>(pEntitySystem)));
    }

    // CCSGOInput — inline global in client.dll at dwCSGOInput offset
    {
        auto clientBase = reinterpret_cast<uintptr_t>(GetModuleHandleA("client.dll"));
        pCSGOInput = reinterpret_cast<void*>(clientBase + Offsets::dwCSGOInput);
        C::Print(std::format("[interfaces] CCSGOInput @ {:#x} (client+{:#x})", reinterpret_cast<uintptr_t>(pCSGOInput),
                             Offsets::dwCSGOInput));
    }

    // Subtick: resolve fnCreateElement + fnAddToField from inlined add_subtick_moves code
    // Pattern: E8 [createElement] 48 8B D0 48 8D 4F 18 E8 [addToField] 48 8B D0
    // These are called by the engine's own subtick creation logic (inlined, not a standalone fn)
    {
        uintptr_t addr = M::FindPattern("client.dll", "E8 ? ? ? ? 48 8B D0 48 8D 4F 18 E8 ? ? ? ? 48 8B D0");
        if (addr)
        {
            Subtick::fnCreateElement = reinterpret_cast<Subtick::CreateElementFn>(M::ResolveRelative(addr, 1, 5));
            Subtick::fnAddToField = reinterpret_cast<Subtick::AddToFieldFn>(M::ResolveRelative(addr + 12, 1, 5));
            C::Print(std::format("[interfaces] subtick: createElement={:#x} addToField={:#x}",
                                 reinterpret_cast<uintptr_t>(Subtick::fnCreateElement),
                                 reinterpret_cast<uintptr_t>(Subtick::fnAddToField)));
        }
        else
        {
            C::Print("[interfaces] subtick pattern not found (subtick disabled)");
        }
    }

    return true;
}

void** I::GetSwapChainVTable()
{
    return s_swapChainVTable;
}
