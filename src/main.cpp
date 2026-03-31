// variant - CS2 utility DLL
// Entry point: DllMain -> DllInitialization thread

//    ██▒   █▓ ▄▄▄       ██▀███   ██▓ ▄▄▄       ███▄    █ ▄▄▄█████▓
//   ▓██░   █▒▒████▄    ▓██ ▒ ██▒▓██▒▒████▄     ██ ▀█   █ ▓  ██▒ ▓▒
//    ▓██  █▒░▒██  ▀█▄  ▓██ ░▄█ ▒▒██▒▒██  ▀█▄  ▓██  ▀█ ██▒▒ ▓██░ ▒░
//     ▒██ █░░░██▄▄▄▄██ ▒██▀▀█▄  ░██░░██▄▄▄▄██ ▓██▒  ▐▌██▒░ ▓██▓ ░
//      ▒▀█░   ▓█   ▓██▒░██▓ ▒██▒░██░ ▓█   ▓██▒▒██░   ▓██░  ▒██▒ ░
//      ░ ▐░   ▒▒   ▓▒█░░ ▒▓ ░▒▓░░▓   ▒▒   ▓▒█░░ ▒░   ▒ ▒   ▒ ░░
//      ░ ░░    ▒   ▒▒ ░  ░▒ ░ ▒░ ▒ ░  ▒   ▒▒ ░░ ░░   ░ ▒░    ░
//        ░░    ░   ▒     ░░   ░  ▒ ░  ░   ▒      ░   ░ ░   ░
//         ░        ░  ░   ░      ░        ░  ░         ░
//        ░

#include "includes.h"
#include "global.h"
#include "core/interfaces.h"
#include "core/hooks/hooks.h"
#include "utilities/debug.h"
#include "utilities/input.h"

DWORD WINAPI DllInitialization(LPVOID lpModule)
{
    C::SetupConsole("variant debug");
    C::Print("[main] variant loaded");

    // Start input hooks early so the unload key works during setup
    Input::Start();

    // Wait for CS2 to finish loading
    while (!GetModuleHandleA("client.dll"))
        Sleep(200);

    if (!I::Setup())
    {
        C::Print("[main] FATAL: interface setup failed");
        Input::Stop();
        C::DestroyConsole();
        FreeLibraryAndExitThread(static_cast<HMODULE>(lpModule), 1);
        return 1;
    }
    C::Print("[main] interfaces OK");

    if (!H::Setup())
    {
        C::Print("[main] FATAL: hook setup failed");
        H::Restore(); // Clean up any hooks that were partially created
        C::DestroyConsole();
        FreeLibraryAndExitThread(static_cast<HMODULE>(lpModule), 1);
        return 1;
    }
    C::Print("[main] hooks OK - variant ready");

    // Wait for unload key (VK_END via low-level hook — GetAsyncKeyState broken by SDL3)
    while (!Input::bEndPressed.load())
        Sleep(200);

    C::Print("[main] unloading...");
    H::Restore();

    // Grace period: let any in-flight Present/ResizeBuffers calls return
    // before the DLL is unloaded from under them
    Sleep(500);

    C::DestroyConsole();
    FreeLibraryAndExitThread(static_cast<HMODULE>(lpModule), 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        G::hDll = hModule;
        DisableThreadLibraryCalls(hModule);
        HANDLE hThread = CreateThread(nullptr, 0, DllInitialization, hModule, 0, nullptr);
        if (hThread)
            CloseHandle(hThread);
    }
    return TRUE;
}
