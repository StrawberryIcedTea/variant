#pragma once
#include <string>
#include <cstdio>
#include <Windows.h>

// Console output (debug builds)
namespace C
{
inline FILE* fStream = nullptr;
inline FILE* fLogFile = nullptr;
inline bool bActive = false;

void SetupConsole(const char* title);
void DestroyConsole();
void Print(const std::string& msg);
}
