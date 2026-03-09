#pragma once
#include <atomic>

// ImGui menu
namespace Menu
{
inline std::atomic<bool> bOpen = true; // starts visible

void Render();
}
