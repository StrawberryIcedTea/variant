#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <Windows.h>

// Memory utilities: pattern scanning, vtable access
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

// Get module base address and size
bool GetModuleInfo(const char* moduleName, uintptr_t& base, size_t& size);
}
