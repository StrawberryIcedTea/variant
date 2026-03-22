// Entity system access — local player

#include "entitysystem.h"
#include <Windows.h>

uintptr_t EntitySystem::GetClientBase()
{
    static uintptr_t base = 0;
    if (!base)
    {
        HMODULE hClient = GetModuleHandleA("client.dll");
        if (hClient)
            base = reinterpret_cast<uintptr_t>(hClient);
    }
    return base;
}

C_BaseEntity* EntitySystem::GetLocalPlayerPawn()
{
    uintptr_t base = GetClientBase();
    if (!base)
        return nullptr;

    return *reinterpret_cast<C_BaseEntity**>(base + Offsets::dwLocalPlayerPawn);
}
