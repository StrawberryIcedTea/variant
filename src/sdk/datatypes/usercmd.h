#pragma once
#include <cstdint>
#include <cstddef>
#include "../../utilities/memory.h"

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
// Offsets confirmed via raw dump probing
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

// CBasePB — protobuf base class (0x18 bytes)
// All protobuf message types inherit this.
// Confirmed via raw dump: has_bits at +0x10, not +0x08.
struct CBasePB
{
    MEM_PAD(0x08);        // +0x00: vtable
    MEM_PAD(0x08);        // +0x08: protobuf internal metadata
    uint32_t nHasBits;    // +0x10 — protobuf has-bits (marks which fields are set)
    uint32_t nCachedSize; // +0x14 — protobuf cached serialization size
};
static_assert(sizeof(CBasePB) == 0x18);
static_assert(offsetof(CBasePB, nHasBits) == 0x10);

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

// -----------------------------------------------------------------------
// Subtick dirty bits — mapped by proto declaration order (not field number)
// Proto fields: button=1, pressed=2, when=3, analog_forward_delta=4,
//               analog_left_delta=5, pitch_delta=8, yaw_delta=9
// Has-bit index = declaration order (0-based), so bits are 0x01..0x40
// -----------------------------------------------------------------------
constexpr uint32_t MOVESTEP_BITS_BUTTON = 0x01;      // field 1
constexpr uint32_t MOVESTEP_BITS_PRESSED = 0x02;     // field 2
constexpr uint32_t MOVESTEP_BITS_WHEN = 0x04;        // field 3
constexpr uint32_t MOVESTEP_BITS_ANALOG_FWD = 0x08;  // field 4 (was 0x20 — wrong)
constexpr uint32_t MOVESTEP_BITS_ANALOG_LEFT = 0x10; // field 5 (was 0x40 — wrong)
constexpr uint32_t MOVESTEP_BITS_PITCH_DELTA = 0x20; // field 8
constexpr uint32_t MOVESTEP_BITS_YAW_DELTA = 0x40;   // field 9
constexpr uint32_t MOVESTEP_BITS_ALL = MOVESTEP_BITS_BUTTON | MOVESTEP_BITS_PRESSED | MOVESTEP_BITS_WHEN;

// CSubtickMoveStep : CBasePB (0x38 bytes, March 2026)
// Offsets confirmed via raw dump of engine-created entry
// pitch_delta / yaw_delta added per usercmd.proto (fields 8, 9)
struct CSubtickMoveStep : CBasePB
{
    uint64_t nButton;           // +0x18 — button enum (IN_JUMP etc)
    bool bPressed;              // +0x20 — press (true) or release (false)
    MEM_PAD(3);                 // +0x21
    float flWhen;               // +0x24 — 0.0-1.0 normalized time within tick
    float flAnalogForwardDelta; // +0x28 — forward move delta per subtick
    float flAnalogLeftDelta;    // +0x2C — left move delta per subtick
    float flPitchDelta;         // +0x30 — pitch delta (proto field 8)
    float flYawDelta;           // +0x34 — yaw delta (proto field 9)

    // Setters — automatically mark has-bits for each field
    void SetButton(uint64_t btn)
    {
        nHasBits |= MOVESTEP_BITS_BUTTON;
        nButton = btn;
    }
    void SetPressed(bool pressed)
    {
        nHasBits |= MOVESTEP_BITS_PRESSED;
        bPressed = pressed;
    }
    void SetWhen(float when)
    {
        nHasBits |= MOVESTEP_BITS_WHEN;
        flWhen = when;
    }
};
static_assert(offsetof(CSubtickMoveStep, nButton) == 0x18);
static_assert(offsetof(CSubtickMoveStep, bPressed) == 0x20);
static_assert(offsetof(CSubtickMoveStep, flWhen) == 0x24);
static_assert(offsetof(CSubtickMoveStep, flAnalogForwardDelta) == 0x28);
static_assert(offsetof(CSubtickMoveStep, flAnalogLeftDelta) == 0x2C);
static_assert(offsetof(CSubtickMoveStep, flPitchDelta) == 0x30);
static_assert(offsetof(CSubtickMoveStep, flYawDelta) == 0x34);

// CMsgQAngle : CBasePB (protobuf view angles, confirmed via raw dump March 2026)
struct CMsgQAngle : CBasePB
{
    float x; // +0x18 — pitch
    float y; // +0x1C — yaw
    float z; // +0x20 — roll
};
static_assert(offsetof(CMsgQAngle, x) == 0x18);
static_assert(offsetof(CMsgQAngle, y) == 0x1C);

// CBaseUserCmdPB : CBasePB (protobuf, March 2026)
struct CBaseUserCmdPB : CBasePB
{
    RepeatedPtrField subtickMoves; // +0x18 — subtick move steps (0x18 bytes)
    void* pMoveCrc;                // +0x30 — std::string* (protobuf bytes field for CRC)
    void* pInButtonStatePB;        // +0x38 — CInButtonStatePB* (protobuf buttons)
    CMsgQAngle* pViewAngles;       // +0x40 — CMsgQAngle*
    MEM_PAD(0x10);                 // +0x48 — nLegacyCommandNumber + nClientTick + new field
    float flForwardMove;           // +0x58 — forward/back movement
    float flSideMove;              // +0x5C — left/right movement

    // Create a new subtick entry and add it to this command.
    // Returns nullptr if subtick functions aren't resolved.
    inline CSubtickMoveStep* CreateSubtickMove();
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
// Uses a static pool of pre-allocated entries. CreateSubtickMove() returns
// entries from this pool and replaces the command's subtickMoves pRep
// with the pool's own rep (never touching the engine's original Rep_t).
// Restore() must be called after the original hook processes our entries.
//
// Usage:
//   if (auto* pSubtick = pBase->CreateSubtickMove()) {
//       pSubtick->SetButton(IN_JUMP);
//       pSubtick->SetWhen(0.5f);
//       pSubtick->SetPressed(true);
//   }
//   ...
//   Subtick::Restore();  // after original hook returns
// -----------------------------------------------------------------------
namespace Subtick
{
    // Function pointers resolved from engine pattern in interfaces.cpp
    using CreateElementFn = void*(__fastcall*)(void* pArena);
    using AddToFieldFn = void*(__fastcall*)(void* pRepeatedField, void* pElement);
    inline CreateElementFn fnCreateElement = nullptr;
    inline AddToFieldFn fnAddToField = nullptr;

    inline bool IsAvailable()
    {
        return fnCreateElement != nullptr;
    }

    // Static pool — max 12 subtick entries per tick (bhop=2, jumpbug=4, strafe=4, spare=2)
    constexpr int kMaxEntries = 12;

    struct Pool
    {
        bool initialized = false;
        int nUsed = 0;
        CSubtickMoveStep* steps[kMaxEntries] = {};
        struct
        {
            uint64_t allocatedSize;
            void* elements[kMaxEntries];
        } rep = {};

        CBaseUserCmdPB* activePBase = nullptr;
        RepeatedPtrField saved = {};

        bool Init()
        {
            if (initialized)
                return true;
            if (!fnCreateElement)
                return false;

            for (int i = 0; i < kMaxEntries; ++i)
            {
                steps[i] = static_cast<CSubtickMoveStep*>(fnCreateElement(nullptr));
                if (!steps[i])
                    return false;
                rep.elements[i] = steps[i];
            }
            rep.allocatedSize = kMaxEntries;
            initialized = true;
            return true;
        }

        CSubtickMoveStep* Allocate(CBaseUserCmdPB* pBase)
        {
            if (nUsed >= kMaxEntries)
                return nullptr;

            // First allocation this tick — save + replace pRep
            if (nUsed == 0)
            {
                saved = pBase->subtickMoves;
                pBase->subtickMoves.pRep = &rep;
                activePBase = pBase;
            }

            auto* pStep = steps[nUsed];
            ++nUsed;

            pBase->subtickMoves.nCurrentSize = nUsed;
            pBase->subtickMoves.nTotalSize = nUsed;
            return pStep;
        }

        void Reset(CBaseUserCmdPB* pBase)
        {
            if (pBase && pBase == activePBase)
            {
                pBase->subtickMoves = saved;
                activePBase = nullptr;
            }
            nUsed = 0;
        }
    };

    inline Pool pool;

    // Restore original subtick state — call after original hook returns
    inline void Restore()
    {
        if (pool.activePBase)
            pool.Reset(pool.activePBase);
    }
}

// CreateSubtickMove — get next entry from the static pool
inline CSubtickMoveStep* CBaseUserCmdPB::CreateSubtickMove()
{
    if (!Subtick::pool.Init())
        return nullptr;

    return Subtick::pool.Allocate(this);
}

namespace Subtick
{
    inline void AddButton(CUserCmd* pCmd, uint64_t button, float when, bool pressed)
    {
        if (auto* pSubtick = pCmd->pBaseCmd->CreateSubtickMove())
        {
            pSubtick->SetButton(button);
            pSubtick->SetWhen(when);
            pSubtick->SetPressed(pressed);
        }
    }
}
