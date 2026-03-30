#pragma once
#include <string_view>
#include "../../dependencies/imgui/imgui.h"

// Custom ImGui widget extensions.
// Kept in Widgets:: to avoid link conflicts with ImGui's own implementations.
namespace Widgets
{
    // Small square checkbox: empty border = unchecked, filled accent square = checked.
    bool Checkbox(const char* szLabel, bool* bValue);

    // Key-binding input. Displays "[ key ]" / "[ press ]" / "[ none ]".
    // pValue holds a Windows VK code (0 = unbound).
    // Click to activate, press any key or mouse button to bind, ESC to clear.
    bool HotKey(const char* szLabel, int* pValue);

    // Standard combo with tighter popup padding (equal sides, smaller items).
    bool Combo(const char* szLabel, int* pValue, const char* const* arrItems, int nItemsCount);

    // Multi-select combo. bValues[i] tracks the checked state of arrItems[i].
    bool MultiCombo(const char* szLabel, bool* bValues, const std::string_view* arrItems, int nItemsCount);

    // "(?) label" with tooltip on hover.
    void HelpMarker(const char* szDescription);
}
