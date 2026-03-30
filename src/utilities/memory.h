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

    // MSVC x64 RTTI vtable scan — stable across updates (mangled class name is a literal in the binary).
    // Walks: TypeDescriptor name → CompleteObjectLocator → vtable[0].
    // Returns vtable[0] pointer, or nullptr on failure.
    inline void** FindVTable(const char* moduleName, const char* className)
    {
        uintptr_t base; size_t size;
        if (!GetModuleInfo(moduleName, base, size))
            return nullptr;

        char needle[256];
        snprintf(needle, sizeof(needle), ".?AV%s@@", className);
        const size_t    nlen = strlen(needle);
        const uintptr_t end  = base + size;

        // 1. TypeDescriptor: mangled name sits at td+0x10
        uintptr_t td = 0;
        for (uintptr_t p = base; p + nlen < end; ++p)
            if (memcmp(reinterpret_cast<const void*>(p), needle, nlen) == 0)
                { td = p - 0x10; break; }
        if (!td) return nullptr;

        // 2. Primary CompleteObjectLocator: sig=1, offset=0, typeDescRVA at +0x0C
        const uint32_t tdRVA = static_cast<uint32_t>(td - base);
        uintptr_t col = 0;
        for (uintptr_t p = base + 0x0C; p + 0x10 < end; p += 4)
            if (*reinterpret_cast<const uint32_t*>(p) == tdRVA)
            {
                uintptr_t c = p - 0x0C;
                if (*reinterpret_cast<const uint32_t*>(c)     == 1u &&
                    *reinterpret_cast<const uint32_t*>(c + 4) == 0u)
                    { col = c; break; }
            }
        if (!col) return nullptr;

        // 3. vtable[0]: the 8 bytes before it hold a pointer to the COL
        for (uintptr_t p = base + 8; p + 8 < end; p += 8)
            if (*reinterpret_cast<const uintptr_t*>(p - 8) == col)
                return reinterpret_cast<void**>(p);
        return nullptr;
    }
}
