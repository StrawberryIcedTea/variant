#pragma once
#include <atomic>

// ImGui menu
namespace Menu
{
    inline std::atomic<bool> bOpen = true; // starts visible
    inline int nCurrentTab = 0;            // 0=Visuals, 1=Misc

    void ApplyTheme();
    void Render();
}
