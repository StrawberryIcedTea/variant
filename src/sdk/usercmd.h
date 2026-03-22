#pragma once
#include <cstdint>
#include <cstddef>
#include "../utilities/memory.h"

// -----------------------------------------------------------------------
// Pointer validation
// -----------------------------------------------------------------------
inline bool IsValidPtr(const void* ptr)
{
    auto addr = reinterpret_cast<uintptr_t>(ptr);
    return addr > 0x10000 && addr < 0x7FFFFFFFFFFF;
}

// -----------------------------------------------------------------------
// CCSGOInput offset (struct not fully mapped — accessor below)
// -----------------------------------------------------------------------
namespace InputOffsets
{
    constexpr uintptr_t nButtonState = 0x248; // CInButtonState at CCSGOInput+0x248
    // nButtonState+0x08 = 0x250 = nValue  (confirmed via probing March 2026)
    // nButtonState+0x10 = 0x258 = nChanged
}

// -----------------------------------------------------------------------
// SDK structs — March 2026 build
// All offsets confirmed via vtable[22] probing
// -----------------------------------------------------------------------

// Protobuf RepeatedPtrField (0x18 bytes)
// Engine layout: { void* pArena, int nCurrentSize, int nTotalSize, Rep_t* pRep }
// Rep_t layout: { int allocatedSize(4), pad(4), void* elements[](at +8) }
struct RepeatedPtrField
{
    void* pArena;     // +0x00 — protobuf arena (always null on this build)
    int nCurrentSize; // +0x08 — current entry count
    int nTotalSize;   // +0x0C — total allocated
    void* pRep;       // +0x10 — Rep_t* (element pointer array)
};
static_assert(sizeof(RepeatedPtrField) == 0x18);

// CBasePB — protobuf base class (0x10 bytes)
// All protobuf message types (CSubtickMoveStep, CBaseUserCmdPB, CInButtonStatePB) inherit this
struct CBasePB
{
    MEM_PAD(0x08);        // +0x00: vtable
    uint32_t nHasBits;    // +0x08 — protobuf has-bits (marks which fields are set)
    uint32_t nCachedSize; // +0x0C — protobuf cached serialization size
};
static_assert(sizeof(CBasePB) == 0x10);
static_assert(offsetof(CBasePB, nHasBits) == 0x08);

// CInButtonState (0x20 bytes, inline in CUserCmd at +0x58 and CCSGOInput at +0x248)
// NOT a protobuf type — simple struct with vtable
struct CInButtonState
{
    MEM_PAD(0x08);     // +0x00: vtable
    uint64_t nValue;   // +0x08 — current pressed buttons (IN_JUMP etc)
    uint64_t nChanged; // +0x10 — buttons changed this tick
    uint64_t nScroll;  // +0x18 — scroll buttons
};
static_assert(sizeof(CInButtonState) == 0x20);
static_assert(offsetof(CInButtonState, nValue) == 0x08);

// CSubtickMoveStep : CBasePB (0x20 bytes, March 2026)
// Fields shifted -8 from asphyxia Nov 2024 layout
struct CSubtickMoveStep : CBasePB
{
    uint64_t nButton; // +0x10 — button enum (IN_JUMP etc)
    bool bPressed;    // +0x18 — press (true) or release (false)
    MEM_PAD(3);       // +0x19
    float flWhen;     // +0x1C — 0.0-1.0 normalized time within tick
};
static_assert(sizeof(CSubtickMoveStep) == 0x20);
static_assert(offsetof(CSubtickMoveStep, nButton) == 0x10);
static_assert(offsetof(CSubtickMoveStep, bPressed) == 0x18);
static_assert(offsetof(CSubtickMoveStep, flWhen) == 0x1C);

// CBaseUserCmdPB : CBasePB (protobuf, March 2026)
// Layout matches asphyxia Nov 2024 through +0x40, but flForwardMove shifted +8
struct CBaseUserCmdPB : CBasePB
{
    MEM_PAD(0x08);                 // +0x10: unknown field
    RepeatedPtrField subtickMoves; // +0x18 — subtick move steps (0x18 bytes)
    uint64_t nMoveCrc;             // +0x30 — CRC-like value
    void* pInButtonStatePB;        // +0x38 — CInButtonStatePB* (protobuf buttons)
    void* pViewAngles;             // +0x40 — CMsgQAngle*
    MEM_PAD(0x10);                 // +0x48 — nLegacyCommandNumber + nClientTick + new field
    float flForwardMove;           // +0x58 — forward/back movement
    float flSideMove;              // +0x5C — left/right movement
};
static_assert(offsetof(CBaseUserCmdPB, subtickMoves) == 0x18);
static_assert(offsetof(CBaseUserCmdPB, pInButtonStatePB) == 0x38);
static_assert(offsetof(CBaseUserCmdPB, flForwardMove) == 0x58);

// CUserCmd (0x98 bytes)
struct CUserCmd
{
    MEM_PAD(0x08);            // +0x00: vtable
    uint32_t nCmdNumber;      // +0x08 — command sequence number
    MEM_PAD(0x34);            // +0x0C
    CBaseUserCmdPB* pBaseCmd; // +0x40 — protobuf base command
    MEM_PAD(0x10);            // +0x48
    CInButtonState nButtons;  // +0x58 — inline button state (NOT protobuf)
};
static_assert(offsetof(CUserCmd, nCmdNumber) == 0x08);
static_assert(offsetof(CUserCmd, pBaseCmd) == 0x40);
static_assert(offsetof(CUserCmd, nButtons) == 0x58);

// -----------------------------------------------------------------------
// Subtick dirty bits
// -----------------------------------------------------------------------
constexpr uint32_t MOVESTEP_BITS_BUTTON = 0x1;
constexpr uint32_t MOVESTEP_BITS_PRESSED = 0x2;
constexpr uint32_t MOVESTEP_BITS_WHEN = 0x4;
constexpr uint32_t MOVESTEP_BITS_ALL = MOVESTEP_BITS_BUTTON | MOVESTEP_BITS_PRESSED | MOVESTEP_BITS_WHEN;

// CInButtonStatePB has-bits
constexpr uint32_t BTNPB_BITS_VALUE = 0x1;
constexpr uint32_t BTNPB_BITS_CHANGED = 0x2;
constexpr uint32_t BTNPB_BITS_SCROLL = 0x4;

// -----------------------------------------------------------------------
// CCSGOInput accessor (returns CInButtonState* for live button state)
// -----------------------------------------------------------------------
namespace CInput
{
    inline CInButtonState* GetButtonState(void* pInput)
    {
        return reinterpret_cast<CInButtonState*>(reinterpret_cast<uintptr_t>(pInput) + InputOffsets::nButtonState);
    }
}

// -----------------------------------------------------------------------
// Subtick — CBaseUserCmdPB subtick entry manipulation
//
// General-purpose: any feature can inject subtick steps into a command.
// Uses static objects (allocated once, never freed by engine) with an
// inject/restore pattern around the original hook call.
//
// Usage from a hook:
//   BEFORE original: Subtick::Inject(pCmd->pBaseCmd, button, press)
//   call original
//   AFTER original:  Subtick::Restore(pCmd->pBaseCmd)
// -----------------------------------------------------------------------
namespace Subtick
{
    // Function pointers resolved from engine pattern in interfaces.cpp
    // Pattern: E8 ? ? ? ? 48 8B D0 48 8D 4F 18 E8 ? ? ? ?
    using CreateElementFn = void*(__fastcall*)(void* pArena);
    using AddToFieldFn = void*(__fastcall*)(void* pRepeatedField, void* pElement);
    inline CreateElementFn fnCreateElement = nullptr;
    inline AddToFieldFn fnAddToField = nullptr;

    // Write fields into a CSubtickMoveStep
    inline void WriteStep(CSubtickMoveStep* pStep, uint64_t button, bool pressed, float when)
    {
        if (!pStep)
            return;

        pStep->nHasBits |= MOVESTEP_BITS_ALL;
        pStep->nButton = button;
        pStep->bPressed = pressed;
        pStep->flWhen = when;
    }

    // Inject two subtick entries (press + release) into CBaseUserCmdPB.
    // Press at start of tick, release mid-tick — tight scroll-like window.
    // Saves original field state for Restore(). Call BEFORE the original hook.
    void Inject(CBaseUserCmdPB* pBase, uint64_t button, float flWhenPress = 0.015625f, float flWhenRelease = 0.65f);

    // Restore original subtick field state. Call AFTER the original hook returns.
    // Prevents engine cleanup from freeing our static objects.
    void Restore(CBaseUserCmdPB* pBase);
}
