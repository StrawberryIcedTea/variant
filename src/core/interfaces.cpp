// Interface discovery for CS2
// D3D11 device/swapchain via dummy window + Source 2 CreateInterface

#include "interfaces.h"
#include "../utilities/debug.h"
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

    D3D_FEATURE_LEVEL featureLevel;
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

    return true;
}

void** GetSwapChainVTable()
{
    return s_swapChainVTable;
}
