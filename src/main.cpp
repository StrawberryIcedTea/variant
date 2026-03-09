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

#include "core/includes.h"
#include "core/interfaces.h"
#include "core/hooks.h"
#include "utilities/debug.h"

DWORD WINAPI DllInitialization(LPVOID lpModule)
{
    C::SetupConsole("variant debug");
    C::Print("[main] variant loaded");

    // Wait for CS2 to finish loading
    while (!GetModuleHandleA("client.dll"))
        Sleep(200);
    C::Print("[main] client.dll found");

    if (!I::Setup())
    {
        C::Print("[main] FATAL: interface setup failed");
        FreeLibraryAndExitThread(static_cast<HMODULE>(lpModule), 1);
        return 1;
    }
    C::Print("[main] interfaces OK");

    if (!H::Setup())
    {
        C::Print("[main] FATAL: hook setup failed");
        FreeLibraryAndExitThread(static_cast<HMODULE>(lpModule), 1);
        return 1;
    }
    C::Print("[main] hooks OK - variant ready");

    // Wait for unload key
    while (!GetAsyncKeyState(VK_END))
        Sleep(200);

    C::Print("[main] unloading...");
    H::Destroy();
    C::DestroyConsole();
    FreeLibraryAndExitThread(static_cast<HMODULE>(lpModule), 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        HANDLE hThread = CreateThread(nullptr, 0, DllInitialization, hModule, 0, nullptr);
        if (hThread)
            CloseHandle(hThread);
    }
    return TRUE;
}
