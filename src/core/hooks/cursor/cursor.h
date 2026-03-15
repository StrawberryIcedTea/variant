#pragma once
#include <atomic>

// Cursor control — forces relative mouse mode off when menu is open.
// Two layers: InputSystem vtable hook + SDL3 direct hook.

// Shared type aliases for cursor-related function signatures
using fnIsRelativeMouseMode = void*(__fastcall*)(void*, bool);
using fnSDL_SetWindowRelativeMouseMode = int (*)(void*, int);

namespace Cursor
{
    // Hook handlers (wired into DTR:: by H::Setup)
    void* __fastcall hkIsRelativeMouseMode(void* pThisptr, bool bActive);
    int hkSDL_SetWindowRelativeMouseMode(void* pWindow, int bEnabled);

    // Called from hkPresent on menu open/close transitions.
    // Handles both InputSystem vtable restore and SDL relative mouse mode.
    void SetMenuMode(bool bMenuOpen);

    // Lightweight per-frame SDL-only suppression while menu is open.
    // The hooks already intercept game calls, but SDL may re-enable between frames.
    void SuppressSDLRelativeMode();

    // Last known game-requested relative mode state.
    // Defaults false — don't hide cursor unless game explicitly requests it.
    // Only updated when menu is closed (open-state calls are echoes of our override).
    // Atomic: written by game thread, read by Present thread.
    inline std::atomic<bool> bGameRelativeMode = false;

    // SDL_Window captured from the game's own calls — always the correct window.
    // Atomic: written by game thread (hook callback), read by Present thread.
    inline std::atomic<void*> pGameSDLWindow = nullptr;
}
