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
#define MEM_PAD(SIZE)                                                                                                  \
  private:                                                                                                             \
    char MEM_PAD_CONCAT(_pad_, __COUNTER__)[SIZE];                                                                     \
                                                                                                                       \
  public:

// -----------------------------------------------------------------------
// Memory utilities: pattern scanning, vtable access
// -----------------------------------------------------------------------
namespace M
{
    // Scan a module for a byte pattern (IDA-style: "48 8B 05 ? ? ? ?")
    uintptr_t FindPattern(const char* moduleName, const char* pattern);

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

    // Get module base address and size
    bool GetModuleInfo(const char* moduleName, uintptr_t& base, size_t& size);
}
