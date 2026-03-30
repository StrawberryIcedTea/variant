#pragma once
#include <cstddef>
#include <cstdint>
#include "entity_handle.h"

// -----------------------------------------------------------------------
// Color_t -- RGBA byte color
// Used as per-draw-call tint in CMeshData::colValue
// -----------------------------------------------------------------------
struct Color_t
{
    uint8_t r, g, b, a;
};

// -----------------------------------------------------------------------
// CMaterial2 -- opaque CS2 material handle
// Passed by pointer to DrawObject; never dereferenced by us
// -----------------------------------------------------------------------
struct CMaterial2;

// -----------------------------------------------------------------------
// CSceneAnimatableObject -- CS2 scene object attached to an entity
// -----------------------------------------------------------------------
struct CSceneAnimatableObject
{
    char        _pad[0xC0];
    CBaseHandle hOwner;  // +0xC0 — player pawn handle (verified: hp=100, team=2/3)
                         // +0xB8 is a non-player entity (idx=320, team=0)
                         // +0x80 is CCSTeam entity (idx=1/3), shared by all team objects
};

// -----------------------------------------------------------------------
// CMeshData -- one primitive entry in CMeshPrimitiveOutputBuffer::m_out
//
// Verified layout (memory dump of entry[0] after GeneratePrimitives):
//   +0x00  vtable ptr (same value repeats at entry[1]+0x00, confirming stride=0x68)
//   +0x20  CMaterial2* pMaterial  (valid heap ptr in dump)
//   +0x28  sort key / hash        (0x837b... — NOT a pointer, do not write here)
//   +0x50  Color_t colValue       (0xffffffff = RGBA white in dump)
// -----------------------------------------------------------------------
struct CMeshData
{
    char        _pad0[0x20];    // +0x00 vtable + other fields
    CMaterial2* pMaterial;      // +0x20
    uint64_t    nSortKey;       // +0x28 sort key (hash of original material)
    char        _pad1[0x20];    // +0x30..+0x4F (+0x30 and +0x38 confirmed NULL in dump)
    Color_t     colValue;       // +0x50
    char        _pad2[0x14];    // +0x54..+0x67 — pad to verified stride
};
static_assert(offsetof(CMeshData, pMaterial) == 0x20);
static_assert(offsetof(CMeshData, nSortKey)  == 0x28);
static_assert(offsetof(CMeshData, colValue)  == 0x50);
static_assert(sizeof(CMeshData)              == 0x68);

// -----------------------------------------------------------------------
// CMeshPrimitiveOutputBuffer -- output buffer filled by GeneratePrimitives
// The hook calls original, which appends entries to m_out[m_count++].
// -----------------------------------------------------------------------
struct CMeshPrimitiveOutputBuffer
{
    CMeshData* m_out;           // +0x00: primitive array
    int32_t    m_maxPrimitives; // +0x08: capacity
    int32_t    m_count;         // +0x0C: current fill index (incremented by original)
};

// GeneratePrimitives function type (VCAnimatableSceneObjectDesc vtable)
using GeneratePrimitivesFn = void(__fastcall*)(void*, CSceneAnimatableObject*, void*, CMeshPrimitiveOutputBuffer*);

// -----------------------------------------------------------------------
// KV3 material creation types
//
// CKeyValues3 layout (asphyxia-cs2 analysis):
//   0x000 - 0x0FF  internal context (managed by engine)
//   0x100          uKey
//   0x108          pValue
//   0x110 - 0x117  pad
//
// fnSetTypeKV3  -- pattern-scanned from client.dll; initializes a raw KV3 block
// fnLoadKV3     -- exported from tier0.dll; loads KV3 text into initialized block
// fnCreateMaterial -- pattern-scanned from materialsystem2.dll; creates CMaterial2*
// -----------------------------------------------------------------------
// Axion layout: context (0x100 bytes) lives BEFORE this struct in the allocation.
// Allocate: new uint8_t[0x100 + sizeof(CKeyValues3)](), then use ptr+0x100 as CKeyValues3*.
struct CKeyValues3
{
    uint64_t uKey;   // +0x00
    void*    pValue; // +0x08
    char     _pad[0x8];
};

struct KV3ID_t
{
    const char* szName;
    uint64_t    unk0;
    uint64_t    unk1;
};

using LoadKV3Fn   = bool(__fastcall*)(CKeyValues3*, void*, const char*, const KV3ID_t*, const char*);
using CreateMatFn = int64_t(__fastcall*)(void*, CMaterial2**, const char*, CKeyValues3*, unsigned int, unsigned int);
