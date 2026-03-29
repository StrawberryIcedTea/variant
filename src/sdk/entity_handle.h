#pragma once
#include <cstdint>

// -----------------------------------------------------------------------
// CBaseHandle -- entity handle (index + serial number)
// -----------------------------------------------------------------------
constexpr uint32_t INVALID_EHANDLE_INDEX = 0xFFFFFFFF;
constexpr uint32_t ENT_ENTRY_MASK = 0x7FFF;
constexpr int NUM_SERIAL_NUM_SHIFT_BITS = 15;

struct CBaseHandle
{
    uint32_t nIndex = INVALID_EHANDLE_INDEX;

    bool IsValid() const { return nIndex != INVALID_EHANDLE_INDEX; }
    int GetEntryIndex() const { return static_cast<int>(nIndex & ENT_ENTRY_MASK); }
    int GetSerialNumber() const { return static_cast<int>(nIndex >> NUM_SERIAL_NUM_SHIFT_BITS); }
};
