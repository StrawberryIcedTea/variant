#pragma once
#include "../../dependencies/minhook/minhook.h"
#include "debug.h"
#include <format>

// MinHook detour wrapper for a single hook target.
// Usage:
//   HookManager hkPresent;
//   hkPresent.Create(vtable[8], &hkPresentDetour);
//   auto oPresent = hkPresent.Original<decltype(&hkPresentDetour)>();
class HookManager
{
  public:
    HookManager() = default;

    // Destructor intentionally does NOT call Remove().
    // MinHook hooks must be removed explicitly before MH_Uninitialize(),
    // and these inline globals outlive the MH_Uninitialize() call in H::Restore().
    ~HookManager() = default;

    // Non-copyable, non-movable (MinHook hooks are not transferable)
    HookManager(const HookManager&) = delete;
    HookManager& operator=(const HookManager&) = delete;
    HookManager(HookManager&&) = delete;
    HookManager& operator=(HookManager&&) = delete;

    // Create and enable a hook in one call
    template <typename Fn> bool Create(void* pFunction, Fn pDetour)
    {
        if (!pFunction || !pDetour)
            return false;

        m_pBase = pFunction;
        m_pReplace = reinterpret_cast<void*>(pDetour);

        MH_STATUS status = MH_CreateHook(m_pBase, m_pReplace, &m_pOriginal);
        if (status != MH_OK)
        {
            C::Print(std::format("[hook] MH_CreateHook failed: {} (base -> {:#x})", MH_StatusToString(status),
                                 reinterpret_cast<uintptr_t>(m_pBase)));
            return false;
        }

        if (!Replace())
        {
            MH_RemoveHook(m_pBase);
            m_pBase = nullptr;
            m_pOriginal = nullptr;
            m_pReplace = nullptr;
            return false;
        }

        return true;
    }

    // Enable the hook (re-enable after Restore)
    bool Replace()
    {
        if (!m_pBase || m_bHooked)
            return false;

        MH_STATUS status = MH_EnableHook(m_pBase);
        if (status != MH_OK)
        {
            C::Print(std::format("[hook] MH_EnableHook failed: {} (base -> {:#x})", MH_StatusToString(status),
                                 reinterpret_cast<uintptr_t>(m_pBase)));
            return false;
        }

        m_bHooked = true;
        return true;
    }

    // Disable the hook but keep it intact for re-enable via Replace()
    bool Restore()
    {
        if (!m_bHooked)
            return false;

        MH_STATUS status = MH_DisableHook(m_pBase);
        if (status != MH_OK)
        {
            C::Print(std::format("[hook] MH_DisableHook failed: {} (base -> {:#x})", MH_StatusToString(status),
                                 reinterpret_cast<uintptr_t>(m_pBase)));
            return false;
        }

        m_bHooked = false;
        return true;
    }

    // Fully remove the hook (restore + remove from MinHook)
    void Remove()
    {
        Restore();

        if (m_pBase)
        {
            MH_RemoveHook(m_pBase);
            m_pBase = nullptr;
        }

        m_pOriginal = nullptr;
        m_pReplace = nullptr;
    }

    template <typename Fn> Fn Original() const { return reinterpret_cast<Fn>(m_pOriginal); }
    bool IsHooked() const { return m_bHooked; }

  private:
    void* m_pBase = nullptr;     // original function address
    void* m_pReplace = nullptr;  // our detour function
    void* m_pOriginal = nullptr; // trampoline back to original
    bool m_bHooked = false;
};
