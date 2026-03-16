// CreateMove hook — game input processing (bhop, etc.)

#include "createmove.h"
#include "../hooks.h"
#include "../../../features/movement.h"
#include "../../../utilities/debug.h"
#include <format>

bool __fastcall CreateMove::hkCreateMove(void* pInput, int nSlot, void* pCmd)
{
    auto oCreateMove = DTR::CreateMove.Original<decltype(&hkCreateMove)>();

    if (!pCmd)
        return oCreateMove(pInput, nSlot, pCmd);

    // Log once
    static bool s_loggedOnce = false;
    if (!s_loggedOnce)
    {
        C::Print(std::format("[createmove] hook firing | pInput: {:#x} | pCmd: {:#x}",
                             reinterpret_cast<uintptr_t>(pInput), reinterpret_cast<uintptr_t>(pCmd)));
        s_loggedOnce = true;
    }

    // Features — modify global state BEFORE oCreateMove reads it
    Movement::BunnyHop();

    return oCreateMove(pInput, nSlot, pCmd);
}
