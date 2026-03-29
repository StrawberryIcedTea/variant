#pragma once
#include <cstdint>
#include "../../utilities/memory.h"

// -----------------------------------------------------------------------
// IGlobalVars — engine global timing state
// Resolved from: "48 89 0D ? ? ? ? 48 89 41" (client.dll)
// -----------------------------------------------------------------------
struct IGlobalVars
{
    float flRealTime;         // 0x00
    int32_t nFrameCount;      // 0x04
    float flFrameTime;        // 0x08
    float flFrameTime2;       // 0x0C
    int32_t nMaxClients;      // 0x10
    float flIntervalPerTick;  // 0x14
    MEM_PAD(0x08);            // 0x18
    void* pUnkFunc;           // 0x20
    float flUnk;              // 0x28
    float flCurtime;          // 0x2C
    float flCurtime2;         // 0x30
    MEM_PAD(0x0C);            // 0x34
    int32_t nTickCount;       // 0x40
    float flIntervalPerTick2; // 0x44
    void* pCurrentNetChannel; // 0x48
    MEM_PAD(0x130);           // 0x50
    char* szCurrentMap;       // 0x180
    char* szCurrentMapName;   // 0x188
};
static_assert(sizeof(IGlobalVars) == 0x190);
