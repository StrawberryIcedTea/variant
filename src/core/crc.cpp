// CRC rebuild — recompute pMoveCrc after post-original command modifications
//
// Uses:
//   - tier0.dll exports: CUtlBuffer ctor + EnsureCapacity
//   - client.dll patterns: SerializePartialToArray, WriteMessage, SetMessageData
//   - vtable[7] on CBaseUserCmdPB: CalculateCmdCRCSize
//
// WriteMessage has a generic prologue that matches many functions.
// All matches are collected, sorted by address (highest first), and
// tried via SEH until the correct one is found. Once found it stays
// locked for the session.

#include "crc.h"
#include "../sdk/datatypes/usercmd.h"
#include "../utilities/memory.h"
#include "../utilities/debug.h"
#include <Windows.h>
#include <format>

// -----------------------------------------------------------------------
// CUtlBuffer — opaque engine utility buffer (0x80 bytes)
// Initialized via exported tier0.dll constructor, not manually.
// -----------------------------------------------------------------------
struct CUtlBuffer
{
    char _data[0x80];
};
static_assert(sizeof(CUtlBuffer) == 0x80);

// -----------------------------------------------------------------------
// Function types
// -----------------------------------------------------------------------

// tier0.dll — exported by mangled name (stable across builds)
using UtlBufferInitFn = void(__fastcall*)(CUtlBuffer*, int, int, unsigned int);
using UtlBufferEnsureCapFn = void(__fastcall*)(CUtlBuffer*, int);

// client.dll — pattern-scanned (may break between updates)
using SerializePartialFn = bool(__fastcall*)(void*, CUtlBuffer, int);
using WriteMessageFn = void(__fastcall*)(uintptr_t*, CUtlBuffer, int);
using SetMessageDataFn = void*(__fastcall*)(void*, uintptr_t*, void*);

// CalculateCmdCRCSize — vtable[7] on CBaseUserCmdPB
constexpr int kCrcSizeVtableIndex = 7;

// -----------------------------------------------------------------------
// Resolved function pointers (set once in Setup)
// -----------------------------------------------------------------------
static UtlBufferInitFn s_fnBufInit = nullptr;
static UtlBufferEnsureCapFn s_fnBufEnsureCap = nullptr;
static SerializePartialFn s_fnSerialize = nullptr;
static WriteMessageFn s_fnWriteMsg = nullptr;
static SetMessageDataFn s_fnSetData = nullptr;

// -----------------------------------------------------------------------
// WriteMessage candidate cycling
//
// The generic WriteMessage prologue matches many functions in client.dll.
// We collect all matches, sort highest-offset-first (empirically correct),
// and cycle via SEH if a candidate crashes.
// -----------------------------------------------------------------------
static constexpr int kMaxCandidates = 16;
static uintptr_t s_candidates[kMaxCandidates] = {};
static int s_nCandidates = 0;
static int s_nCandidateIdx = 0;

static bool s_bSetupDone = false;
static bool s_bAvailable = false;

// -----------------------------------------------------------------------
// Pattern definitions
// -----------------------------------------------------------------------

// SerializePartialToArray — serializes CBaseUserCmdPB to byte buffer
static const char* s_serializePats[] = {
    "48 89 5C 24 18 55 56 57 48 81 EC 90",
    "48 89 5C 24 18 55 56 57 48 81 EC ? ? ? ? 48 8B",
};

// WriteMessage — encodes serialized protobuf into CRC message object
static const char* s_writeMsgPats[] = {
    "48 89 5C 24 10 48 89 6C 24 18 48 89 7C 24 20 41 56 48 83 EC 20",
};

// SetMessageData — writes CRC string into CBaseUserCmdPB::pMoveCrc
static const char* s_setDataPats[] = {
    "48 89 5C 24 20 55 56 57 48 83 EC 30 49",
    "48 89 5C 24 20 55 56 57 48 83 EC 30",
};

// -----------------------------------------------------------------------
// Setup — resolve all functions from tier0.dll and client.dll
// -----------------------------------------------------------------------
bool CRC::Setup()
{
    if (s_bSetupDone)
        return s_bAvailable;
    s_bSetupDone = true;

    // --- tier0.dll: CUtlBuffer constructor + EnsureCapacity ---
    HMODULE hTier0 = GetModuleHandleA("tier0.dll");
    if (!hTier0)
    {
        C::Print("[crc] tier0.dll not loaded");
        return false;
    }

    s_fnBufInit =
        reinterpret_cast<UtlBufferInitFn>(GetProcAddress(hTier0, "??0CUtlBuffer@@QEAA@HHW4BufferFlags_t@0@@Z"));
    s_fnBufEnsureCap =
        reinterpret_cast<UtlBufferEnsureCapFn>(GetProcAddress(hTier0, "?EnsureCapacity@CUtlBuffer@@QEAAXH@Z"));

    if (!s_fnBufInit || !s_fnBufEnsureCap)
    {
        C::Print("[crc] CUtlBuffer exports not found in tier0.dll");
        return false;
    }

    // --- client.dll: pattern-scan Serialize + SetMessageData ---
    uintptr_t clientBase = 0;
    size_t clientSize = 0;
    M::GetModuleInfo("client.dll", clientBase, clientSize);

    auto serialize = M::FindPattern("client.dll", s_serializePats, std::size(s_serializePats));
    auto setData = M::FindPattern("client.dll", s_setDataPats, std::size(s_setDataPats));

    if (!serialize || !setData)
    {
        if (!serialize)
            C::Print("[crc] SerializePartialToArray not found");
        if (!setData)
            C::Print("[crc] SetMessageData not found");
        return false;
    }

    s_fnSerialize = reinterpret_cast<SerializePartialFn>(serialize);
    s_fnSetData = reinterpret_cast<SetMessageDataFn>(setData);

    // --- client.dll: collect all WriteMessage candidates ---
    s_nCandidates = 0;
    for (size_t p = 0; p < std::size(s_writeMsgPats) && s_nCandidates < kMaxCandidates; ++p)
    {
        uintptr_t addr = M::FindPattern("client.dll", s_writeMsgPats[p]);
        while (addr && s_nCandidates < kMaxCandidates)
        {
            s_candidates[s_nCandidates++] = addr;
            addr = M::FindNextPattern("client.dll", s_writeMsgPats[p], addr);
        }
    }

    if (s_nCandidates == 0)
    {
        C::Print("[crc] WriteMessage pattern not found");
        return false;
    }

    // Sort highest-offset-first — the correct WriteMessage is consistently
    // at the highest address, so this minimizes SEH crash attempts.
    for (int i = 0; i < s_nCandidates / 2; ++i)
    {
        int j = s_nCandidates - 1 - i;
        auto tmp = s_candidates[i];
        s_candidates[i] = s_candidates[j];
        s_candidates[j] = tmp;
    }

    s_nCandidateIdx = 0;
    s_fnWriteMsg = reinterpret_cast<WriteMessageFn>(s_candidates[0]);

    s_bAvailable = true;
    C::Print(std::format("[crc] ready ({} WriteMessage candidates)", s_nCandidates));
    return true;
}

bool CRC::IsAvailable()
{
    return s_bAvailable;
}

// -----------------------------------------------------------------------
// RebuildInternal — CRC recomputation logic
//
// Separated from SEH wrapper because MSVC /EHsc forbids __try in
// functions containing C++ objects that require unwinding.
//
// WriteMessage (the crash-prone call) runs BEFORE modifying pBaseCmd
// so that SEH recovery from a wrong candidate leaves no dirty state.
// -----------------------------------------------------------------------
static bool RebuildInternal(CBaseUserCmdPB* pBaseCmd)
{
    int nSize = M::CallVFunc<int>(pBaseCmd, kCrcSizeVtableIndex);
    if (nSize <= 0 || nSize > 4096)
        return false;

    CUtlBuffer buf = {};
    s_fnBufInit(&buf, 0, 0, 0);
    s_fnBufEnsureCap(&buf, nSize + 1);

    if (!s_fnSerialize(pBaseCmd, buf, nSize))
        return false;

    uintptr_t msgBuf[3] = {};

    // WriteMessage is the crash-prone call (wrong candidate).
    // Execute before touching pBaseCmd so SEH leaves clean state.
    s_fnWriteMsg(msgBuf, buf, nSize);

    // All risky calls done — safe to modify game state
    pBaseCmd->nCachedSize |= 1;

    uint32_t nBits = pBaseCmd->nHasBits & ~3u;
    pBaseCmd->pMoveCrc = s_fnSetData(static_cast<void*>(&pBaseCmd->pMoveCrc), msgBuf, &nBits);

    return true;
}

// -----------------------------------------------------------------------
// RebuildSEH — pure C SEH wrapper (no C++ objects with dtors)
// -----------------------------------------------------------------------
static bool RebuildSEH(CBaseUserCmdPB* pBaseCmd)
{
    __try
    {
        return RebuildInternal(pBaseCmd);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        s_bAvailable = false;
        return false;
    }
}

// -----------------------------------------------------------------------
// CRC::Rebuild — public entry point with candidate cycling
// -----------------------------------------------------------------------
bool CRC::Rebuild(CBaseUserCmdPB* pBaseCmd)
{
    if (!s_bAvailable || !IsValidPtr(pBaseCmd))
        return false;

    bool ok = RebuildSEH(pBaseCmd);

    // SEH caught a crash — advance to next WriteMessage candidate
    if (!ok && !s_bAvailable)
    {
        ++s_nCandidateIdx;
        if (s_nCandidateIdx < s_nCandidates)
        {
            s_fnWriteMsg = reinterpret_cast<WriteMessageFn>(s_candidates[s_nCandidateIdx]);
            s_bAvailable = true;

            uintptr_t clientBase = 0;
            size_t clientSize = 0;
            M::GetModuleInfo("client.dll", clientBase, clientSize);
            C::Print(std::format("[crc] candidate {} crashed, trying {}: client+{:#x}", s_nCandidateIdx - 1,
                                 s_nCandidateIdx, s_candidates[s_nCandidateIdx] - clientBase));
        }
        else
        {
            C::Print("[crc] all WriteMessage candidates exhausted — CRC disabled");
        }
    }

    return ok;
}
