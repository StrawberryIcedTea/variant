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
inline std::atomic<bool> bSpaceHeld = false;     // reserved for features

// Mouse button states
inline std::atomic<bool> bMouseLeft = false;
inline std::atomic<bool> bMouseRight = false;
inline std::atomic<bool> bMouseMiddle = false;

void Start();
void Stop();
}
