#pragma once
#include "../../../dependencies/imgui/imgui.h"

// Central colour palette.
// Edit values here — ApplyTheme() and menu rendering both reference these.
namespace Theme
{
    // ---- Backgrounds -------------------------------------------------------
    // Deep near-black — main window, sidebar
    inline constexpr ImVec4 BgPrimary = {0.04f, 0.04f, 0.04f, 1.00f};
    // Slightly lifted — child windows, hovered controls
    inline constexpr ImVec4 BgSecondary = {0.12f, 0.12f, 0.12f, 1.00f};

    // ---- Foreground --------------------------------------------------------
    // Near-white body text
    inline constexpr ImVec4 FgPrimary = {0.95f, 0.95f, 0.95f, 1.00f};
    // Muted — disabled labels, hints
    inline constexpr ImVec4 FgSecondary = {0.85f, 0.85f, 0.85f, 0.80f};

    // ---- Accent  -----------------------------------------------------------
    // Runtime — updated by Menu::ApplyTheme() from Vars.flAccentColor.
    // Default: saura07 red. Change via the Misc menu color picker.
    inline ImVec4 Accent = {1.000f, 0.000f, 0.404f, 1.00f};
    // Half-brightness of Accent — recomputed each ApplyTheme() call.
    inline ImVec4 AccentDim = {0.500f, 0.000f, 0.202f, 1.00f};

    // ---- Borders -----------------------------------------------------------
    // Default border — matches ImGuiCol_Border set in ApplyTheme
    inline constexpr ImVec4 BorderDefault = {0.28f, 0.28f, 0.28f, 1.00f};
    // Hovered border / icon hover color — slightly lighter gray
    inline constexpr ImVec4 BorderHovered = {0.45f, 0.45f, 0.45f, 1.00f};

    // ---- Title bar ---------------------------------------------------------
    // Bright bar contrasting the dark window body
    inline constexpr ImVec4 TitleBg = {0.95f, 0.95f, 0.95f, 1.00f};
    // Dark text that reads against TitleBg
    inline constexpr ImVec4 TitleText = {0.04f, 0.04f, 0.04f, 1.00f};
}
