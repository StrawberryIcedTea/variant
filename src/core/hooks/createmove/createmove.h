#pragma once
#include "../../../includes.h"

// CreateMove hook handler
namespace CreateMove
{
    bool __fastcall hkCreateMove(void* pInput, int nSlot, void* pCmd);
}
