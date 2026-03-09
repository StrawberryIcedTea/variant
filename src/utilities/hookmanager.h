#pragma once
#include "../../dependencies/minhook/minhook.h"
#include "debug.h"
#include <format>

// Thin wrapper around MinHook for a single hook target.
// Usage:
//   HookManager hkPresent;
//   hkPresent.pTarget = vtable[8];  // Present slot
//   hkPresent.Create(&hkPresentDetour);
//   auto oPresent = hkPresent.Original<decltype(&hkPresentDetour)>();
class HookManager
{
  public:
    void* pTarget = nullptr;

    HookManager() = default;
    ~HookManager() { Destroy(); }

    // Non-copyable, non-movable (MinHook hooks are not transferable)
    HookManager(const HookManager&) = delete;
    HookManager& operator=(const HookManager&) = delete;
    HookManager(HookManager&&) = delete;
    HookManager& operator=(HookManager&&) = delete;

    bool Create(void* pDetour) noexcept
    {
        if (!pTarget || !pDetour)
            return false;

        MH_STATUS status = MH_CreateHook(pTarget, pDetour, &m_pOriginal);
        if (status != MH_OK)
        {
            C::Print(std::format("[hook] MH_CreateHook failed: {}", MH_StatusToString(status)));
            return false;
        }

        status = MH_EnableHook(pTarget);
        if (status != MH_OK)
        {
            C::Print(std::format("[hook] MH_EnableHook failed: {}", MH_StatusToString(status)));
            return false;
        }

        m_bActive = true;
        return true;
    }

    void Destroy() noexcept
    {
        if (!m_bActive)
            return;

        MH_DisableHook(pTarget);
        MH_RemoveHook(pTarget);
        m_bActive = false;
        m_pOriginal = nullptr;
    }

    template <typename Fn> Fn Original() const noexcept { return reinterpret_cast<Fn>(m_pOriginal); }

  private:
    void* m_pOriginal = nullptr;
    bool m_bActive = false;
};
