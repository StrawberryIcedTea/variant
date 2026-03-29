#pragma once
#include <cstdint>
#include <cstring>
#include "../utilities/memory.h"

// -----------------------------------------------------------------------
// CVValue_t — convar value union (Source 2)
// -----------------------------------------------------------------------
union CVValue_t
{
    bool b;
    int16_t i16;
    uint16_t u16;
    int32_t i32;
    uint32_t u32;
    int64_t i64;
    uint64_t u64;
    float fl;
    double db;
    const char* sz;
};

// -----------------------------------------------------------------------
// CConVar — Source 2 console variable (partial, read-only access)
// -----------------------------------------------------------------------
struct CConVar
{
    const char* szName;        // 0x00
    void* pNext;               // 0x08
    MEM_PAD(0x10);             // 0x10
    const char* szDescription; // 0x20
    uint32_t nType;            // 0x28
    uint32_t nRegistered;      // 0x2C
    uint32_t nFlags;           // 0x30
    MEM_PAD(0x24);             // 0x34 — padding to value at 0x58 (shifted from 0x40 in older builds)
    CVValue_t value;           // 0x58
};
static_assert(offsetof(CConVar, szName) == 0x00);
static_assert(offsetof(CConVar, nFlags) == 0x30);
static_assert(offsetof(CConVar, value) == 0x58);

// -----------------------------------------------------------------------
// Convar — cached convar pointers for movement features
//
// Pointers resolved in Convar::Setup() from IEngineCVar linked list.
// Read .value.fl / .value.b at runtime for live server values.
// -----------------------------------------------------------------------
namespace Convar
{
    // IEngineCVar interface pointer (tier0.dll "VEngineCvar007")
    inline void* pCvar = nullptr;

    // Cached convar pointers (null if not found — use defaults)
    inline CConVar* sv_airaccelerate = nullptr;     // default 12
    inline CConVar* sv_air_max_wishspeed = nullptr; // default 30
    inline CConVar* sv_gravity = nullptr;           // default 800
    inline CConVar* sv_autobunnyhopping = nullptr;  // default false
    inline CConVar* sv_maxspeed = nullptr;          // default 320
    inline CConVar* sv_standable_normal = nullptr;  // default 0.7

    // Find a convar by name — walks CUtlLinkedList<CConVar*, unsigned short> at IEngineCVar+0x48
    inline CConVar* Find(const char* szName)
    {
        if (!pCvar || !szName)
            return nullptr;

        // CUtlLinkedList layout at IEngineCVar+0x48 (shifted from +0x40 in older builds):
        //   +0x00: CUtlMemory::m_pMemory (pointer to node array)
        //   +0x08: uint16_t iHead, iTail, iFirstFree, nElementCount
        // Each node (16 bytes, padded): { CConVar* element (+0), uint16_t iPrev (+8), uint16_t iNext (+0xA) }
        auto base = reinterpret_cast<uintptr_t>(pCvar) + 0x48;
        auto* pNodes = *reinterpret_cast<uint8_t**>(base);
        uint16_t idx = *reinterpret_cast<uint16_t*>(base + 0x08);

        if (!pNodes)
            return nullptr;

        constexpr int kNodeSize = 16;
        constexpr uint16_t kInvalidIndex = 0xFFFF;

        for (int count = 0; idx != kInvalidIndex && count < 10000; ++count)
        {
            auto* pConVar = *reinterpret_cast<CConVar**>(pNodes + idx * kNodeSize);
            if (pConVar && pConVar->szName && strcmp(pConVar->szName, szName) == 0)
                return pConVar;
            idx = *reinterpret_cast<uint16_t*>(pNodes + idx * kNodeSize + 0x0A);
        }
        return nullptr;
    }

    // Setup — discover IEngineCVar and cache convar pointers
    bool Setup();

    // Safe float read with default fallback
    inline float GetFloat(CConVar* pConVar, float flDefault)
    {
        return pConVar ? pConVar->value.fl : flDefault;
    }

    inline bool GetBool(CConVar* pConVar, bool bDefault)
    {
        return pConVar ? pConVar->value.b : bDefault;
    }

    inline float GetGravity()
    {
        return GetFloat(sv_gravity, 800.f);
    }
    inline float GetStandableNormal()
    {
        return GetFloat(sv_standable_normal, 0.7f);
    }
}
