#include "memory.h"
#include "debug.h"
#include <Psapi.h>
#include <format>

#pragma comment(lib, "psapi.lib")

static std::vector<int> PatternToBytes(const char* pattern)
{
    std::vector<int> bytes;
    const char* cur = pattern;

    while (*cur)
    {
        if (*cur == ' ')
        {
            ++cur;
            continue;
        }
        if (*cur == '?')
        {
            bytes.push_back(-1);
            ++cur;
            if (*cur == '?')
                ++cur;
        }
        else
        {
            bytes.push_back(static_cast<int>(strtoul(cur, const_cast<char**>(&cur), 16)));
        }
    }

    return bytes;
}

bool M::GetModuleInfo(const char* moduleName, uintptr_t& base, size_t& size)
{
    HMODULE hMod = GetModuleHandleA(moduleName);
    if (!hMod)
        return false;

    MODULEINFO mi{};
    if (!GetModuleInformation(GetCurrentProcess(), hMod, &mi, sizeof(mi)))
        return false;

    base = reinterpret_cast<uintptr_t>(mi.lpBaseOfDll);
    size = mi.SizeOfImage;
    return true;
}

// Core scan — no logging, used by both overloads
static uintptr_t ScanModule(uintptr_t base, size_t size, const char* pattern)
{
    auto patternBytes = PatternToBytes(pattern);
    const auto scanBytes = reinterpret_cast<uint8_t*>(base);
    const auto patSize = patternBytes.size();
    const auto patData = patternBytes.data();

    if (patSize == 0 || patSize > size)
        return 0;

    for (size_t i = 0; i < size - patSize; ++i)
    {
        bool found = true;
        for (size_t j = 0; j < patSize; ++j)
        {
            if (patData[j] != -1 && patData[j] != scanBytes[i + j])
            {
                found = false;
                break;
            }
        }

        if (found)
            return base + i;
    }

    return 0;
}

uintptr_t M::FindPattern(const char* moduleName, const char* pattern)
{
    uintptr_t base = 0;
    size_t size = 0;

    if (!GetModuleInfo(moduleName, base, size))
    {
        C::Print(std::format("[memory] module not found: {}", moduleName));
        return 0;
    }

    uintptr_t result = ScanModule(base, size, pattern);
    if (!result)
        C::Print(std::format("[memory] pattern not found in {}: {}", moduleName, pattern));

    return result;
}

uintptr_t M::FindPattern(const char* moduleName, const char** patterns, size_t count)
{
    uintptr_t base = 0;
    size_t size = 0;

    if (!GetModuleInfo(moduleName, base, size))
    {
        C::Print(std::format("[memory] module not found: {}", moduleName));
        return 0;
    }

    for (size_t i = 0; i < count; ++i)
    {
        uintptr_t result = ScanModule(base, size, patterns[i]);
        if (result)
            return result;
    }

    C::Print(std::format("[memory] all {} patterns failed in {}", count, moduleName));
    return 0;
}

uintptr_t M::FindNextPattern(const char* moduleName, const char* pattern, uintptr_t after)
{
    uintptr_t base = 0;
    size_t size = 0;
    if (!GetModuleInfo(moduleName, base, size))
        return 0;

    uintptr_t start = after + 1;
    if (start < base || start >= base + size)
        return 0;

    size_t offset = start - base;
    return ScanModule(start, size - offset, pattern);
}

uintptr_t M::ResolveRelative(uintptr_t addr, int offset, int instrLen)
{
    if (!addr)
        return 0;

    auto rel = *reinterpret_cast<int32_t*>(addr + offset);
    return addr + instrLen + rel;
}
