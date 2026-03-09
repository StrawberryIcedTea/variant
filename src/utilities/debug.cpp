#include "debug.h"
#include <chrono>
#include <format>

void C::SetupConsole(const char* title)
{
#ifdef _DEBUG
    if (bActive)
        return;

    AllocConsole();
    freopen_s(&fStream, "CONOUT$", "w", stdout);
    SetConsoleTitleA(title);

    // Log to file next to the DLL for post-crash inspection
    char dllPath[MAX_PATH]{};
    HMODULE hSelf = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCSTR>(&C::SetupConsole), &hSelf);
    if (hSelf && GetModuleFileNameA(hSelf, dllPath, MAX_PATH))
    {
        std::string logPath(dllPath);
        auto pos = logPath.find_last_of("\\/");
        if (pos != std::string::npos)
            logPath = logPath.substr(0, pos + 1) + "variant_log.txt";
        else
            logPath = "variant_log.txt";
        freopen_s(&fLogFile, logPath.c_str(), "w", stderr);
    }

    bActive = true;
#endif
}

void C::DestroyConsole()
{
#ifdef _DEBUG
    if (!bActive)
        return;

    if (fLogFile)
    {
        fclose(fLogFile);
        fLogFile = nullptr;
    }
    if (fStream)
    {
        fclose(fStream);
        fStream = nullptr;
    }
    FreeConsole();
    bActive = false;
#endif
}

void C::Print(const std::string& msg)
{
#ifdef _DEBUG
    if (!bActive)
        return;

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_s(&tm, &time);

    auto formatted = std::format("[{:02d}:{:02d}:{:02d}] {}\n", tm.tm_hour, tm.tm_min, tm.tm_sec, msg);
    printf("%s", formatted.c_str());
    if (fLogFile)
    {
        fprintf(fLogFile, "%s", formatted.c_str());
        fflush(fLogFile);
    }
    OutputDebugStringA(formatted.c_str());
#endif
}
