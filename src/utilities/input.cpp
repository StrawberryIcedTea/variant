#include "input.h"
#include "debug.h"
#include <atomic>

static HHOOK s_hKeyboardHook = nullptr;
static HHOOK s_hMouseHook = nullptr;
static std::atomic<HANDLE> s_hThread = nullptr;
static DWORD s_threadId = 0;

static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        auto* kb = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

        if (kb->vkCode == VK_INSERT && wParam == WM_KEYDOWN)
            Input::bInsertPressed.store(true);

        if (kb->vkCode == VK_END && wParam == WM_KEYDOWN)
            Input::bEndPressed.store(true);

        if (kb->vkCode == VK_SPACE)
        {
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
                Input::bSpaceHeld.store(true);
            else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
                Input::bSpaceHeld.store(false);
        }
    }

    return CallNextHookEx(s_hKeyboardHook, nCode, wParam, lParam);
}

static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        switch (wParam)
        {
        case WM_LBUTTONDOWN:
            Input::bMouseLeft.store(true);
            break;
        case WM_LBUTTONUP:
            Input::bMouseLeft.store(false);
            break;
        case WM_RBUTTONDOWN:
            Input::bMouseRight.store(true);
            break;
        case WM_RBUTTONUP:
            Input::bMouseRight.store(false);
            break;
        case WM_MBUTTONDOWN:
            Input::bMouseMiddle.store(true);
            break;
        case WM_MBUTTONUP:
            Input::bMouseMiddle.store(false);
            break;
        default:
            break;
        }
    }

    return CallNextHookEx(s_hMouseHook, nCode, wParam, lParam);
}

static DWORD WINAPI HookThread(LPVOID)
{
    s_hKeyboardHook = SetWindowsHookExA(WH_KEYBOARD_LL, LowLevelKeyboardProc, nullptr, 0);
    s_hMouseHook = SetWindowsHookExA(WH_MOUSE_LL, LowLevelMouseProc, nullptr, 0);

    if (!s_hKeyboardHook || !s_hMouseHook)
    {
        C::Print("[input] SetWindowsHookEx failed");
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (s_hKeyboardHook)
        UnhookWindowsHookEx(s_hKeyboardHook);
    if (s_hMouseHook)
        UnhookWindowsHookEx(s_hMouseHook);
    s_hKeyboardHook = nullptr;
    s_hMouseHook = nullptr;
    return 0;
}

void Input::Start()
{
    if (s_hThread)
        return;

    s_hThread = CreateThread(nullptr, 0, HookThread, nullptr, 0, &s_threadId);
    if (!s_hThread)
        C::Print("[input] failed to create hook thread");
}

void Input::Stop()
{
    if (!s_hThread)
        return;

    if (s_threadId)
        PostThreadMessage(s_threadId, WM_QUIT, 0, 0);

    WaitForSingleObject(s_hThread, 2000);
    CloseHandle(s_hThread);
    s_hThread = nullptr;
    s_threadId = 0;
}
