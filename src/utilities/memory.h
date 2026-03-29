#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <Windows.h>

// -----------------------------------------------------------------------
// Struct padding macro — use instead of manual char _padN[size]
// Two-level expansion ensures __COUNTER__ resolves before ##
// -----------------------------------------------------------------------
#define MEM_PAD_CONCAT_INNER(a, b) a##b
#define MEM_PAD_CONCAT(a, b) MEM_PAD_CONCAT_INNER(a, b)
#define MEM_PAD(SIZE) char MEM_PAD_CONCAT(_pad_, __COUNTER__)[SIZE];

// -----------------------------------------------------------------------
// Pointer validation
// -----------------------------------------------------------------------
inline bool IsValidPtr(const void* ptr)
{
    auto addr = reinterpret_cast<uintptr_t>(ptr);
    return addr > 0x10000 && addr < 0x7FFFFFFFFFFF;
}

// -----------------------------------------------------------------------
// Memory utilities: pattern scanning, vtable access
// -----------------------------------------------------------------------
namespace M
{
    // Scan a module for a byte pattern (IDA-style: "48 8B 05 ? ? ? ?")
    uintptr_t FindPattern(const char* moduleName, const char* pattern);

    // Try multiple patterns in order, return the first match.
    // Logs only when ALL patterns fail (individual misses are silent).
    uintptr_t FindPattern(const char* moduleName, const char** patterns, size_t count);

    // Resolve a RIP-relative address: addr + offset + instrLen
    uintptr_t ResolveRelative(uintptr_t addr, int offset, int instrLen);

    // Get a virtual function pointer from a vtable
    template <typename T = void*> T GetVFunc(void* pClass, int index)
    {
        auto vtable = *reinterpret_cast<void***>(pClass);
        return reinterpret_cast<T>(vtable[index]);
    }

    // Call a virtual function by index (combines GetVFunc + invoke)
    template <typename T, typename... Args> T CallVFunc(void* pClass, int index, Args... args)
    {
        using Fn = T(__thiscall*)(void*, Args...);
        return (*static_cast<Fn**>(pClass))[index](pClass, args...);
    }

    // Get an exported function from a loaded module
    inline void* GetExport(const char* moduleName, const char* exportName)
    {
        HMODULE hMod = GetModuleHandleA(moduleName);
        if (!hMod)
            return nullptr;
        return reinterpret_cast<void*>(GetProcAddress(hMod, exportName));
    }

    // Find the next match of a pattern beyond a previous result
    uintptr_t FindNextPattern(const char* moduleName, const char* pattern, uintptr_t prev);

    // Get module base address and size
    bool GetModuleInfo(const char* moduleName, uintptr_t& base, size_t& size);
}
