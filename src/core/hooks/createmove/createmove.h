#pragma once
#include "../../../includes.h"

// CreateMove hooks on CCSGOInput
namespace CreateMove
{
    // vtable[5] — outer wrapper (button manipulation)
    bool __fastcall hkCreateMove(void* pInput, int nSlot, bool bActive);

    // vtable[22] — inner CreateMove (CUserCmd* access for subtick)
    bool __fastcall hkCreateMoveInner(void* pInput, int nSlot, void* pCmd);
}
