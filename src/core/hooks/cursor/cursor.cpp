// Cursor control — InputSystem + SDL3 relative mouse mode hooks

#include "cursor.h"
#include "../hooks.h"
#include "../../interfaces.h"
#include "../../menu/menu.h"

void* __fastcall Cursor::hkIsRelativeMouseMode(void* pThisptr, bool bActive)
{
    auto oFunc = DTR::IsRelativeMouseMode.Original<fnIsRelativeMouseMode>();
    // Only track game state when menu is closed — while open, game calls
    // are responses to our forced state, not the game's true intent
    if (!Menu::bOpen)
        bGameRelativeMode.store(bActive);
    else
        return oFunc(pThisptr, false);
    return oFunc(pThisptr, bActive);
}

int Cursor::hkSDL_SetWindowRelativeMouseMode(void* pWindow, int bEnabled)
{
    auto oFunc = DTR::SDLSetRelMouseMode.Original<fnSDL_SetWindowRelativeMouseMode>();

    // Capture the game's own SDL_Window — always the correct pointer
    void* expected = nullptr;
    if (pWindow)
        pGameSDLWindow.compare_exchange_strong(expected, pWindow);

    // Only track game state when menu is closed — while open, game calls
    // are responses to our forced state, not the game's true intent
    if (!Menu::bOpen)
        bGameRelativeMode.store(bEnabled != 0);
    else
        return oFunc(pWindow, 0);
    return oFunc(pWindow, bEnabled);
}

// Internal: force SDL relative mouse mode directly
static void ForceSDLRelativeMouseMode(bool bRelative)
{
    auto oFunc = DTR::SDLSetRelMouseMode.Original<fnSDL_SetWindowRelativeMouseMode>();
    void* pWnd = Cursor::pGameSDLWindow.load();
    if (!oFunc || !pWnd)
        return;
    oFunc(pWnd, bRelative ? 1 : 0);
}

void Cursor::SetMenuMode(bool bMenuOpen)
{
    bool target = bMenuOpen ? false : bGameRelativeMode.load();

    // Layer 1: InputSystem — restore/override game's relative mouse mode
    auto oIsRMM = DTR::IsRelativeMouseMode.Original<fnIsRelativeMouseMode>();
    if (oIsRMM && I::pInputSystem)
        oIsRMM(I::pInputSystem, target);

    // Layer 2: SDL direct — must match Layer 1
    ForceSDLRelativeMouseMode(target);
}

void Cursor::SuppressSDLRelativeMode()
{
    ForceSDLRelativeMouseMode(false);
}
