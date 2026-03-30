// Interface discovery for CS2
// D3D11 device/swapchain via dummy window + Source 2 CreateInterface

#include "interfaces.h"
#include "../sdk/entity.h"
#include "../sdk/datatypes/usercmd.h"
#include "../sdk/trace.h"
#include "convars.h"
#include "schema.h"
#include "crc.h"
#include "../utilities/debug.h"
#include "../utilities/memory.h"
#include <format>

// Create a temporary D3D11 device + swapchain to grab the vtable pointers.
// Standard kiero approach: create dummy device, read vtable, destroy.
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

    I::pSwapChainVTable = *reinterpret_cast<void***>(pSc);

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
    auto pCreateInterface = reinterpret_cast<CreateInterfaceFn>(M::GetExport(moduleName, "CreateInterface"));
    if (!pCreateInterface)
        return nullptr;

    return pCreateInterface(interfaceName, nullptr);
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

    // IEngineClient — local player index, in-game state
    pEngine = static_cast<IEngineClient*>(CaptureInterface("engine2.dll", "Source2EngineToClient001"));
    if (!pEngine)
    {
        C::Print("[interfaces] FAILED: Source2EngineToClient001 not found");
        return false;
    }

    // GameResourceService - needed for entity access (bhop, ESP, etc.)
    pGameResourceService = CaptureInterface("engine2.dll", "GameResourceServiceClientV001");
    if (!pGameResourceService)
    {
        C::Print("[interfaces] FAILED: GameResourceServiceClientV001 not found");
        return false;
    }

    // CGameEntitySystem — pattern-scanned from client.dll (survives offset shifts)
    {
        static const char* entityListPats[] = {
            "48 8B 0D ? ? ? ? 48 89 7C 24 ? 8B FA C1 EB",
            "48 8B 0D ? ? ? ? 48 89 7C 24 ? 8B FA",
        };
        uintptr_t addr = M::FindPattern("client.dll", entityListPats, std::size(entityListPats));
        if (addr)
        {
            ppEntitySystem = reinterpret_cast<void**>(M::ResolveRelative(addr, 0x3, 0x7));
        }
        else
        {
            C::Print("[interfaces] CGameEntitySystem pattern not found (entity features disabled)");
            return false;
        }
    }

    // CCSGOInput — hardcoded offset (pattern scans match wrong mov rcx instructions)
    // The object is behind a global pointer in client.dll's .data section
    // Update from cs2-dumper when game updates break this offset
    {
        constexpr uintptr_t dwCSGOInput = 0x231B2E0;
        auto clientBase = reinterpret_cast<uintptr_t>(GetModuleHandleA("client.dll"));
        pCSGOInput = reinterpret_cast<void*>(clientBase + dwCSGOInput);
    }

    // Subtick: resolve fnCreateElement + fnAddToField from inlined add_subtick_moves code
    // Pattern: E8 [createElement] 48 8B D0 48 8D 4F 18 E8 [addToField] 48 8B D0
    // These are called by the engine's own subtick creation logic (inlined, not a standalone fn)
    {
        static const char* subtickPats[] = {
            "E8 ? ? ? ? 48 8B D0 48 8D 4F 18 E8 ? ? ? ? 48 8B D0", // asphyxia — full call pair
            "E8 ? ? ? ? 48 8B D0 48 8D 4F 18 E8",                  // shorter — just up to second call
        };
        uintptr_t addr = M::FindPattern("client.dll", subtickPats, std::size(subtickPats));
        if (addr)
        {
            Subtick::fnCreateElement = reinterpret_cast<Subtick::CreateElementFn>(M::ResolveRelative(addr, 1, 5));
            Subtick::fnAddToField = reinterpret_cast<Subtick::AddToFieldFn>(M::ResolveRelative(addr + 12, 1, 5));
        }
        else
        {
            C::Print("[interfaces] subtick pattern not found (subtick disabled)");
        }
    }

    // CGameTraceManager — global pointer for hull/shape traces
    // TraceShape is vtable[5] (FF 50 28 in discovery pattern), no separate pattern needed
    {
        static const char* tracePats[] = {
            "48 8B 0D ? ? ? ? 48 8B 01 FF 50 28 48 8D 8D E0 01", // asphyxia — full xref
            "48 8B 0D ? ? ? ? 48 8B 01 FF 50 28 48 8D 8D",       // shorter suffix
            "48 8B 0D ? ? ? ? 48 8B 01 FF 50 28",                // minimal — mov rcx,[rip+?] + vtable call
        };
        uintptr_t addr = M::FindPattern("client.dll", tracePats, std::size(tracePats));
        if (addr)
        {
            Trace::pGameTraceManager = *reinterpret_cast<void**>(M::ResolveRelative(addr, 0x3, 0x7));
        }
        else
        {
            C::Print("[interfaces] CGameTraceManager pattern not found (traces disabled)");
        }
    }

    // TraceFilter_t is manually initialized — no pattern needed

    // IGlobalVars — engine timing state (curtime, frametime, tickcount)
    // Multiple patterns: asphyxia (load, longer) and Axion (store, shorter)
    {
        static const char* globalVarsPats[] = {
            "48 8B 0D ? ? ? ? 4C 8D 05 ? ? ? ? 48 89 74 24 30 BA", // asphyxia — STR: "gpGlobals->rendertime()"
            "48 89 0D ? ? ? ? 48 89 41",                           // Axion — STR: "OnSequenceCycleChanged"
        };
        uintptr_t addr = M::FindPattern("client.dll", globalVarsPats, std::size(globalVarsPats));
        if (addr)
        {
            pGlobalVars = *reinterpret_cast<IGlobalVars**>(M::ResolveRelative(addr, 0x3, 0x7));
        }
        else
        {
            C::Print("[interfaces] IGlobalVars pattern not found (prediction disabled)");
        }
    }

    // Schema system — runtime offset resolution (survives game updates)
    if (!Schema::Setup("client.dll"))
        C::Print("[interfaces] schema setup failed (will use fallback offsets)");

    // IEngineCVar — convar system from tier0.dll
    pCvar = CaptureInterface("tier0.dll", "VEngineCvar007");
    if (pCvar)
        Convar::pCvar = pCvar;
    else
        C::Print("[interfaces] IEngineCVar not found (convars will use defaults)");

    // Cache frequently-used convars
    if (!Convar::Setup())
        C::Print("[interfaces] convar setup failed (will use defaults)");

    // CRC rebuild — pattern scan for protobuf serialization functions
    // If patterns are stale (game update), CRC rebuild is unavailable
    // but everything still works in -insecure mode
    CRC::Setup();

    // GetMatrixForView — IDA: CViewRender::OnRenderStart -> GetMatricesForView
    // Non-fatal: WorldToScreen is disabled if the pattern is stale
    {
        static const char* matrixPats[] = {
            "48 89 5C 24 ? 48 89 74 24 ? 57 48 81 EC ? ? ? ? 49", // save rbx+rsi, push rdi, sub rsp
            "40 53 48 81 EC ? ? ? ? 49 8B C1",                    // pre-March 2026: push rbx, sub rsp,imm32
        };
        uintptr_t addr = M::FindPattern("client.dll", matrixPats, std::size(matrixPats));
        if (addr)
            pGetMatrixForView = reinterpret_cast<void*>(addr);
        else
            C::Print("[interfaces] GetMatrixForView pattern not found (WorldToScreen disabled)");
    }

    // CAnimatableSceneObject vtable — RTTI scan (no singleton at startup; class name stable across updates)
    {
        pAnimatableSceneObjectVTable = M::FindVTable("scenesystem.dll", "CAnimatableSceneObjectDesc");
        if (!pAnimatableSceneObjectVTable)
            C::Print("[interfaces] CAnimatableSceneObjectDesc vtable not found (chams disabled)");
    }

    // IMaterialSystem2 — needed as 'this' for CreateMaterial
    I::pMaterialSystem = CaptureInterface("materialsystem2.dll", "VMaterialSystem2_001");
    if (!I::pMaterialSystem)
        C::Print("[interfaces] VMaterialSystem2_001 not found (chams material creation disabled)");

    // LoadKV3 — tier0.dll export; loads KV3 text into a CKeyValues3 block
    {
        HMODULE hTier0 = GetModuleHandleA("tier0.dll");
        if (hTier0)
        {
            // Try both known mangled names (signature changed between CS2 builds)
            auto* p = M::GetExport("tier0.dll", "?LoadKV3@@YA_NPEAVKeyValues3@@PEAVCUtlString@@PEBDAEBUKV3ID_t@@2@Z");
            if (!p)
                p = M::GetExport("tier0.dll", "?LoadKV3@@YA_NPEAVKeyValues3@@PEAVCUtlString@@PEBDAEBUKV3ID_t@@2I@Z");
            if (p)
                fnLoadKV3 = reinterpret_cast<LoadKV3Fn>(p);
            else
                C::Print("[interfaces] LoadKV3 export not found in tier0.dll (chams material creation disabled)");
        }
    }

    // CreateMaterial — materialsystem2.dll; creates CMaterial2* from KV3 data
    {
        static const char* createMatPats[] = {
            "48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 48 81 EC ? ? ? ? 48 8D 0D",
            "48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 48 81 EC ? ? ? ? 48 8B 05",
        };
        uintptr_t addr = M::FindPattern("materialsystem2.dll", createMatPats, std::size(createMatPats));
        if (addr)
            fnCreateMaterial = reinterpret_cast<CreateMatFn>(addr);
        else
            C::Print("[interfaces] CreateMaterial pattern not found (chams material creation disabled)");
    }

    return true;
}
