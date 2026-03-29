#pragma once
#include <cstdint>
#include <cstring>
#include "datatypes/vector.h"
#include "../utilities/memory.h"

// -----------------------------------------------------------------------
// Ray_t — hull/point trace ray (0x38 bytes)
// -----------------------------------------------------------------------
struct Ray_t
{
    Vec3 m_vecStart; // 0x00
    Vec3 m_vecEnd;   // 0x0C
    Vec3 m_vecMins;  // 0x18
    Vec3 m_vecMaxs;  // 0x24
    MEM_PAD(0x4);    // 0x30
    uint8_t m_nType; // 0x34 — 0=point, 1=box
};
static_assert(sizeof(Ray_t) == 0x38);

// -----------------------------------------------------------------------
// TraceFilter_t — trace collision filter (0x40 bytes)
// Manually initialized — no engine function needed
// -----------------------------------------------------------------------
struct TraceFilter_t
{
    MEM_PAD(0x8);                // 0x00
    int64_t m_uTraceMask;        // 0x08
    int64_t m_v1[2];             // 0x10
    int32_t m_arrSkipHandles[4]; // 0x20
    int16_t m_arrCollisions[2];  // 0x30
    int16_t m_v2;                // 0x34
    uint8_t m_v3;                // 0x36 — layer
    uint8_t m_v4;                // 0x37
    uint8_t m_v5;                // 0x38

    // Manual initialization (matches asphyxia approach)
    // uMask: 0x1400B = MASK_PLAYERSOLID (movement), 0x1C3003 = MASK_SHOT (visibility)
    // nLayer: 3 = movement, 4 = shot traces
    void Init(uint64_t uMask, uint8_t nLayer = 3)
    {
        std::memset(this, 0, sizeof(*this));
        m_uTraceMask = uMask;
        m_v2 = 7;
        m_v3 = nLayer;
        m_v4 = 0x49;
        m_v5 = 0;
    }

    // Init with entity skip (requires entity entry index, owner index, collision mask)
    void Init(uint64_t uMask, int32_t nEntityIndex, int32_t nOwnerIndex, int16_t nCollisionMask, uint8_t nLayer = 3)
    {
        Init(uMask, nLayer);
        m_arrSkipHandles[0] = nEntityIndex;
        m_arrSkipHandles[2] = nOwnerIndex;
        m_arrCollisions[0] = nCollisionMask;
    }
};
static_assert(sizeof(TraceFilter_t) == 0x40);

// -----------------------------------------------------------------------
// GameTrace_t — trace result (0x108 bytes)
// -----------------------------------------------------------------------
struct GameTrace_t
{
    void* m_pSurface;     // 0x00
    void* m_pHitEntity;   // 0x08
    void* m_pHitboxData;  // 0x10
    MEM_PAD(0x38);        // 0x18
    uint32_t m_uContents; // 0x50
    MEM_PAD(0x24);        // 0x54
    Vec3 m_vecStartPos;   // 0x78
    Vec3 m_vecEndPos;     // 0x84
    Vec3 m_vecNormal;     // 0x90
    Vec3 m_vecPosition;   // 0x9C
    MEM_PAD(0x4);         // 0xA8
    float m_flFraction;   // 0xAC — 0.0 = start solid, 1.0 = no hit
    MEM_PAD(0x6);         // 0xB0
    bool m_bAllSolid;     // 0xB6
    MEM_PAD(0x4D);        // 0xB7 — compiler pads to 0x108 for 8-byte alignment
};
static_assert(sizeof(GameTrace_t) == 0x108);
static_assert(offsetof(GameTrace_t, m_flFraction) == 0xAC);
static_assert(offsetof(GameTrace_t, m_vecNormal) == 0x90);

// -----------------------------------------------------------------------
// Trace — engine trace via CGameTraceManager vtable
// TraceShape = vtable[5] (0x28), confirmed from FF 50 28 in discovery pattern
// -----------------------------------------------------------------------
namespace Trace
{
    // CGameTraceManager global pointer — resolved via pattern scan
    inline void* pGameTraceManager = nullptr;

    // TraceShape calling convention (vtable[5])
    using TraceShapeFn = bool(__fastcall*)(void* pThis, Ray_t* pRay, Vec3* pStart, Vec3* pEnd, TraceFilter_t* pFilter,
                                           GameTrace_t* pTrace);

    constexpr int VTABLE_TRACESHAPE = 5; // 0x28 / 8

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    // Hull trace from start to end
    inline bool Shape(Ray_t* pRay, Vec3* pStart, Vec3* pEnd, TraceFilter_t* pFilter, GameTrace_t* pTrace)
    {
        if (!pGameTraceManager)
            return false;

        auto fn = M::GetVFunc<TraceShapeFn>(pGameTraceManager, VTABLE_TRACESHAPE);
        if (!fn)
            return false;

        return fn(pGameTraceManager, pRay, pStart, pEnd, pFilter, pTrace);
    }

    // Check if traces are available
    inline bool IsAvailable()
    {
        return pGameTraceManager != nullptr;
    }
}
