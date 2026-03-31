#pragma once
#include <Windows.h>
#include <atomic>

// Low-level keyboard + mouse hooks for input detection.
// CS2 uses SDL3 which steals raw input - GetAsyncKeyState returns 0,
// and WM_LBUTTONDOWN etc. never reach the WndProc.
// We use WH_KEYBOARD_LL + WH_MOUSE_LL on a dedicated thread with its own message pump.
namespace Input
{
    // Key states
    inline std::atomic<bool> bInsertPressed = false; // menu toggle
    inline std::atomic<bool> bEndPressed = false;    // unload trigger
    inline std::atomic<bool> bSpaceHeld = false;     // reserved for features

    // Mouse button states
    inline std::atomic<bool> bMouseLeft = false;
    inline std::atomic<bool> bMouseRight = false;
    inline std::atomic<bool> bMouseMiddle = false;

    // Last VK code pressed via WH_KEYBOARD_LL (0 = none pending).
    // Widgets consume this with exchange(0) so it is one-shot.
    inline std::atomic<int> nLastVK = 0;

    void Start();
    void Stop();
}
