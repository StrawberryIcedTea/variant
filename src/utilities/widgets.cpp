// Custom ImGui widget extensions — adapted from saura07 (qo0) for this project.
// Key API adaptations vs the original:
//   - FocusableItemRegister removed (defunct since 1.82); click-only activation used instead
//   - io.KeysDown / io.KeyMap replaced with Input::nLastVK (WH_LL hook, bypasses SDL3)
//   - fmt::format replaced with plain std::string
//   - vector<bool> replaced with bool* (avoids bitfield proxy issues)
//   - ActiveIdIsJustActivated guards mouse capture on the activation click frame
//   - Checkbox is fully custom-drawn: small square, no tick — filled accent when checked

#include "widgets.h"
#include "input.h"
#include "../core/menu/theme.h"
#include "../core/hooks/render/render.h"
#include "../../dependencies/imgui/imgui_internal.h"

#include <array>
#include <cstdio>
#include <string>

// Bring in all ImGui symbols so internal functions resolve without prefix.
using namespace ImGui;

// Human-readable names indexed by Windows VK code.
static constexpr std::array<const char*, 166> arrKeyNames = {"",          "mouse 1",
                                                             "mouse 2",   "cancel",
                                                             "mouse 3",   "mouse 4",
                                                             "mouse 5",   "",
                                                             "backspace", "tab",
                                                             "",          "",
                                                             "clear",     "enter",
                                                             "",          "",
                                                             "shift",     "control",
                                                             "alt",       "pause",
                                                             "caps",      "",
                                                             "",          "",
                                                             "",          "",
                                                             "",          "escape",
                                                             "",          "",
                                                             "",          "",
                                                             "space",     "page up",
                                                             "page down", "end",
                                                             "home",      "left",
                                                             "up",        "right",
                                                             "down",      "",
                                                             "",          "",
                                                             "print",     "insert",
                                                             "delete",    "",
                                                             "0",         "1",
                                                             "2",         "3",
                                                             "4",         "5",
                                                             "6",         "7",
                                                             "8",         "9",
                                                             "",          "",
                                                             "",          "",
                                                             "",          "",
                                                             "",          "a",
                                                             "b",         "c",
                                                             "d",         "e",
                                                             "f",         "g",
                                                             "h",         "i",
                                                             "j",         "k",
                                                             "l",         "m",
                                                             "n",         "o",
                                                             "p",         "q",
                                                             "r",         "s",
                                                             "t",         "u",
                                                             "v",         "w",
                                                             "x",         "y",
                                                             "z",         "lwin",
                                                             "rwin",      "",
                                                             "",          "",
                                                             "num0",      "num1",
                                                             "num2",      "num3",
                                                             "num4",      "num5",
                                                             "num6",      "num7",
                                                             "num8",      "num9",
                                                             "*",         "+",
                                                             "",          "-",
                                                             ".",         "/",
                                                             "f1",        "f2",
                                                             "f3",        "f4",
                                                             "f5",        "f6",
                                                             "f7",        "f8",
                                                             "f9",        "f10",
                                                             "f11",       "f12",
                                                             "f13",       "f14",
                                                             "f15",       "f16",
                                                             "f17",       "f18",
                                                             "f19",       "f20",
                                                             "f21",       "f22",
                                                             "f23",       "f24",
                                                             "",          "",
                                                             "",          "",
                                                             "",          "",
                                                             "",          "",
                                                             "num lock",  "scroll lock",
                                                             "",          "",
                                                             "",          "",
                                                             "",          "",
                                                             "",          "",
                                                             "",          "",
                                                             "",          "",
                                                             "",          "",
                                                             "lshift",    "rshift",
                                                             "lctrl",     "rctrl",
                                                             "lmenu",     "rmenu"};

// ---------------------------------------------------------------------------
// Checkbox — small square, no tick mark
// ---------------------------------------------------------------------------

bool Widgets::Checkbox(const char* szLabel, bool* bValue)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* pWindow = g.CurrentWindow;
    if (pWindow->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = pWindow->GetID(szLabel);

    // Box dimensions — small square, vertically centred in a standard frame row
    static constexpr float BOX = 9.f; // outer box size in pixels
    static constexpr float PAD = 2.f; // inset for the filled square

    const float frameH = GetFrameHeight();
    const ImVec2 pos = pWindow->DC.CursorPos;
    const ImVec2 labelSize = CalcTextSize(szLabel, nullptr, true);

    // Vertically centre the box within the frame row
    const float boxY = pos.y + (frameH - BOX) * 0.5f;
    const ImRect bbBox(ImVec2(pos.x, boxY), ImVec2(pos.x + BOX, boxY + BOX));

    const float totalW = BOX + (labelSize.x > 0.f ? style.ItemInnerSpacing.x + labelSize.x : 0.f);
    const ImRect bbTotal(pos, ImVec2(pos.x + totalW, pos.y + frameH));

    ItemSize(bbTotal, style.FramePadding.y);
    if (!ItemAdd(bbTotal, id))
        return false;

    bool bHovered = false, bHeld = false;
    const bool bPressed = ButtonBehavior(bbTotal, id, &bHovered, &bHeld);
    if (bPressed)
        *bValue = !(*bValue);

    // ---- Draw box ----
    ImDrawList* dl = pWindow->DrawList;

    // Background fill
    dl->AddRectFilled(bbBox.Min, bbBox.Max, GetColorU32(ImGuiCol_FrameBg));

    // Border — lighter gray on hover/held, matches combo + icon standard
    const ImU32 colBorder = (bHovered || bHeld) ? GetColorU32(Theme::BorderHovered) : GetColorU32(ImGuiCol_Border);
    dl->AddRect(bbBox.Min, bbBox.Max, colBorder);

    // Checked: fill inner square with accent colour
    if (*bValue)
    {
        const ImVec4& ac = Theme::Accent;
        dl->AddRectFilled(ImVec2(bbBox.Min.x + PAD, bbBox.Min.y + PAD), ImVec2(bbBox.Max.x - PAD, bbBox.Max.y - PAD),
                          IM_COL32(static_cast<int>(ac.x * 255.f), static_cast<int>(ac.y * 255.f),
                                   static_cast<int>(ac.z * 255.f), 255));
    }

    // ---- Label ----
    if (labelSize.x > 0.f)
        RenderText(ImVec2(bbBox.Max.x + style.ItemInnerSpacing.x, pos.y + style.FramePadding.y), szLabel);

    return bPressed;
}

// ---------------------------------------------------------------------------
// HotKey
// ---------------------------------------------------------------------------

bool Widgets::HotKey(const char* szLabel, int* pValue)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* pWindow = g.CurrentWindow;
    if (pWindow->SkipItems)
        return false;

    ImGuiIO& io = g.IO;
    const ImGuiStyle& style = g.Style;
    const ImGuiID nId = pWindow->GetID(szLabel);

    const float flWidth = CalcItemWidth();
    const ImVec2 vecLabelSize = CalcTextSize(szLabel, nullptr, true);

    const float flLabelOffset = vecLabelSize.x > 0.f ? style.ItemInnerSpacing.x + GetFrameHeight() : 0.f;
    const ImRect rectFrame(
        ImVec2(pWindow->DC.CursorPos.x + flLabelOffset, pWindow->DC.CursorPos.y),
        ImVec2(pWindow->DC.CursorPos.x + flWidth,
               pWindow->DC.CursorPos.y + (vecLabelSize.x > 0.f ? vecLabelSize.y + style.FramePadding.y : 0.f)));
    const ImRect rectTotal(rectFrame.Min, rectFrame.Max);

    ItemSize(rectTotal, style.FramePadding.y);
    if (!ItemAdd(rectTotal, nId, &rectFrame))
        return false;

    const bool bHovered = ItemHoverable(rectFrame, nId, 0);
    if (bHovered && io.MouseClicked[0])
    {
        if (g.ActiveId != nId)
            *pValue = 0;
        SetActiveID(nId, pWindow);
        FocusWindow(pWindow);
    }

    bool bValueChanged = false;

    if (g.ActiveId == nId && !g.ActiveIdIsJustActivated)
    {
        // Mouse buttons
        static constexpr int vkMouse[5] = {VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2};
        for (int n = 0; n < IM_ARRAYSIZE(vkMouse); n++)
        {
            if (IsMouseDown(n))
            {
                *pValue = vkMouse[n];
                bValueChanged = true;
                ClearActiveID();
                break;
            }
        }

        if (!bValueChanged)
        {
            // Keyboard — consumed from WH_LL hook (bypasses SDL3 input stealing)
            const int vk = Input::nLastVK.exchange(0);
            if (vk == VK_ESCAPE)
            {
                *pValue = 0;
                ClearActiveID();
            }
            else if (vk != 0)
            {
                *pValue = vk;
                bValueChanged = true;
                ClearActiveID();
            }
        }
    }

    // Display "[ key ]" / "[ press ]" / "[ none ]"
    char chBuffer[64] = {};
    const bool bActive = (g.ActiveId == nId);
    if (bActive)
    {
        snprintf(chBuffer, sizeof(chBuffer), "[ press ]");
    }
    else if (*pValue > 0 && *pValue < static_cast<int>(arrKeyNames.size()))
    {
        snprintf(chBuffer, sizeof(chBuffer), "[ %s ]", arrKeyNames[static_cast<std::size_t>(*pValue)]);
    }
    else
    {
        snprintf(chBuffer, sizeof(chBuffer), "[ none ]");
    }

    pWindow->DrawList->AddText(
        ImVec2(rectFrame.Max.x - CalcTextSize(chBuffer).x, rectTotal.Min.y + style.FramePadding.y),
        GetColorU32(bActive ? ImGuiCol_Text : ImGuiCol_TextDisabled), chBuffer);

    if (vecLabelSize.x > 0.f)
        RenderText(ImVec2(rectTotal.Min.x, rectTotal.Min.y + style.FramePadding.y), szLabel);

    return bValueChanged;
}

// ---------------------------------------------------------------------------
// Combo — tighter popup padding so top/bottom match sides
// ---------------------------------------------------------------------------

bool Widgets::Combo(const char* szLabel, int* pValue, const char* const* arrItems, int nItemsCount)
{
    // Render label above the box — display text is everything before "##"
    {
        const char* pHashHash = strstr(szLabel, "##");
        const char* pEnd = pHashHash ? pHashHash : szLabel + strlen(szLabel);
        if (pEnd > szLabel)
            ImGui::TextUnformatted(szLabel, pEnd);
    }

    // WindowPadding must be pushed before BeginCombo so the popup window inherits it
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.f, 6.f));

    // Build a hidden-label ID ("##original_label") so the inline label doesn't render
    char szComboId[256];
    snprintf(szComboId, sizeof(szComboId), "##%s", szLabel);

    bool bChanged = false;
    if (ImGui::BeginCombo(szComboId, arrItems[*pValue]))
    {
        // Popup is now open — push item-only styles inside the dropdown
        if (Render::pFontInterMedium)
            ImGui::PushFont(Render::pFontInterMedium);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.f, 0.f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.f, 3.f));

        for (int i = 0; i < nItemsCount; i++)
        {
            if (i > 0)
                ImGui::Dummy(ImVec2(0.f, 3.f));
            const bool bSelected = (*pValue == i);
            if (ImGui::Selectable(arrItems[i], bSelected, 0, ImVec2(0.f, 14.f)))
            {
                *pValue = i;
                bChanged = true;
            }
            if (bSelected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::PopStyleVar(2);
        if (Render::pFontInterMedium)
            ImGui::PopFont();

        ImGui::EndCombo();
    }

    ImGui::PopStyleVar(); // WindowPadding

    // Hover border — lighter gray, matches checkbox standard
    if (ImGui::IsItemHovered())
        ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                            ImGui::GetColorU32(Theme::BorderHovered));

    return bChanged;
}

// ---------------------------------------------------------------------------
// MultiCombo
// ---------------------------------------------------------------------------

bool Widgets::MultiCombo(const char* szLabel, bool* bValues, const std::string_view* arrItems, int nItemsCount)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* pWindow = g.CurrentWindow;
    if (pWindow->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const float flActiveWidth = CalcItemWidth() - (style.ItemInnerSpacing.x + GetFrameHeight()) - 40.f;

    // Build preview string from active items
    std::string szPreview;
    for (int i = 0; i < nItemsCount; i++)
    {
        if (!bValues[i])
            continue;
        if (!szPreview.empty())
            szPreview += ", ";
        szPreview += arrItems[i];
    }

    if (szPreview.empty())
    {
        szPreview = "none";
    }
    else if (CalcTextSize(szPreview.c_str()).x > flActiveWidth)
    {
        szPreview.resize(static_cast<std::size_t>(flActiveWidth * 0.26f));
        szPreview += "...";
    }

    bool bValueChanged = false;
    if (BeginCombo(szLabel, szPreview.c_str()))
    {
        for (int i = 0; i < nItemsCount; i++)
        {
            if (Selectable(arrItems[i].data(), bValues[i], ImGuiSelectableFlags_DontClosePopups))
            {
                bValues[i] = !bValues[i];
                bValueChanged = true;
            }
        }
        EndCombo();
    }

    return bValueChanged;
}

// ---------------------------------------------------------------------------
// HelpMarker
// ---------------------------------------------------------------------------

void Widgets::HelpMarker(const char* szDescription)
{
    TextDisabled("(?)");
    if (IsItemHovered())
    {
        BeginTooltip();
        PushTextWrapPos(450.f);
        TextUnformatted(szDescription);
        PopTextWrapPos();
        EndTooltip();
    }
}
