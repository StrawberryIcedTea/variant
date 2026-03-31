// Menu rendering - ImGui overlay

#include "menu.h"
#include "theme.h"
#include "../variables.h"
#include "../../../dependencies/imgui/imgui.h"
#include "../../utilities/widgets.h"
#include "../hooks/render/render.h"
#include "icons.h"

// ---------------------------------------------------------------------------
// Tab content
// ---------------------------------------------------------------------------

static void RenderVisuals()
{
    Widgets::Checkbox("ESP", &Vars.bESP);
    ImGui::Separator();

    Widgets::Checkbox("Chams", &Vars.bChams);
    if (Vars.bChams)
    {
        static const char* matNames[] = {"Flat", "Glow"};

        Widgets::Combo("Material##chams", &Vars.nChamsMaterial, matNames, IM_ARRAYSIZE(matNames));
        Widgets::Checkbox("XQZ (through-wall)##chams", &Vars.bChamsXQZ);
        Widgets::Checkbox("Double Chams##chams", &Vars.bChamsDouble);
        if (Vars.bChamsDouble)
            Widgets::Combo("Material 2##chams", &Vars.nChamsMaterial2, matNames, IM_ARRAYSIZE(matNames));

        Widgets::Checkbox("Enemies##chams", &Vars.bChamsEnemies);
        ImGui::SameLine();
        Widgets::Checkbox("Teammates##chams", &Vars.bChamsTeammates);
        ImGui::SameLine();
        Widgets::Checkbox("Self##chams", &Vars.bChamsSelf);

        ImGui::Spacing();
        if (Vars.bChamsDouble)
        {
            ImGui::TextDisabled("Pass 1");
            ImGui::ColorEdit4("Enemy##c1", Vars.flChamsEnemyColor, ImGuiColorEditFlags_NoInputs);
            ImGui::SameLine();
            ImGui::ColorEdit4("Team##c1", Vars.flChamsTeamColor, ImGuiColorEditFlags_NoInputs);
            ImGui::SameLine();
            ImGui::ColorEdit4("Self##c1", Vars.flChamsSelfColor, ImGuiColorEditFlags_NoInputs);
            ImGui::TextDisabled("Pass 2");
            ImGui::ColorEdit4("Enemy##c2", Vars.flChamsEnemyColor2, ImGuiColorEditFlags_NoInputs);
            ImGui::SameLine();
            ImGui::ColorEdit4("Team##c2", Vars.flChamsTeamColor2, ImGuiColorEditFlags_NoInputs);
            ImGui::SameLine();
            ImGui::ColorEdit4("Self##c2", Vars.flChamsSelfColor2, ImGuiColorEditFlags_NoInputs);
        }
        else
        {
            ImGui::ColorEdit4("Enemy##c1", Vars.flChamsEnemyColor, ImGuiColorEditFlags_NoInputs);
            ImGui::SameLine();
            ImGui::ColorEdit4("Team##c1", Vars.flChamsTeamColor, ImGuiColorEditFlags_NoInputs);
            ImGui::SameLine();
            ImGui::ColorEdit4("Self##c1", Vars.flChamsSelfColor, ImGuiColorEditFlags_NoInputs);
        }
    }
}

static void RenderMisc()
{
    static const char* modes[] = {"Disabled", "Normal", "Subtick"};
    Widgets::Combo("Bunny Hop", &Vars.nBhopMode, modes, IM_ARRAYSIZE(modes));
    Widgets::Combo("Auto Strafe", &Vars.nAutoStrafeMode, modes, IM_ARRAYSIZE(modes));
    Widgets::Checkbox("Jump Bug", &Vars.bJumpBug);
    Widgets::Checkbox("Edge Bug", &Vars.bEdgeBug);

    ImGui::Separator();

    if (ImGui::ColorEdit4("menu color", Vars.flAccentColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha))
        Menu::ApplyTheme();
}

// ---------------------------------------------------------------------------
// Theme
// ---------------------------------------------------------------------------

void Menu::ApplyTheme()
{
    // Sync runtime accent from Vars before applying
    Theme::Accent = {Vars.flAccentColor[0], Vars.flAccentColor[1], Vars.flAccentColor[2], 1.00f};
    Theme::AccentDim = {Theme::Accent.x * 0.5f, Theme::Accent.y * 0.5f, Theme::Accent.z * 0.5f, 1.00f};

    // Aliases for readability — all values sourced from Theme::
    const auto& bg = Theme::BgPrimary;
    const auto& bg2 = Theme::BgSecondary;
    const auto& fg = Theme::FgPrimary;
    const auto& ac = Theme::Accent;
    const auto& acd = Theme::AccentDim;

    ImGuiStyle& s = ImGui::GetStyle();

    // ---- Layout ------------------------------------------------------------
    s.Alpha = 1.0f;
    s.WindowPadding = {16.f, 16.f};
    s.WindowRounding = 0.f;
    s.WindowBorderSize = 1.0f;
    s.WindowTitleAlign = {0.033f, 0.5f};
    s.ChildRounding = 0.f;
    s.ChildBorderSize = 1.0f;
    s.PopupRounding = 0.f;
    s.PopupBorderSize = 1.0f;
    s.FramePadding = {8.f, 3.f};
    s.FrameRounding = 0.f;
    s.FrameBorderSize = 1.0f;
    s.ItemSpacing = {8.f, 8.f};
    s.ItemInnerSpacing = {8.f, 4.f};
    s.IndentSpacing = 6.0f;
    s.ColumnsMinSpacing = 6.0f;
    s.ScrollbarSize = 6.0f;
    s.ScrollbarRounding = 0.f;
    s.GrabMinSize = 0.f;
    s.GrabRounding = 0.f;
    s.TabRounding = 0.f;
    s.TabBorderSize = 1.0f;
    s.ButtonTextAlign = {0.5f, 0.5f};
    s.SelectableTextAlign = {0.0f, 0.5f};

    // ---- Colours -----------------------------------------------------------
    auto& c = s.Colors;

    // text
    c[ImGuiCol_Text] = fg;
    c[ImGuiCol_TextDisabled] = {fg.x, fg.y, fg.z, 0.40f};
    c[ImGuiCol_TextSelectedBg] = {ac.x, ac.y, ac.z, 0.35f};

    // windows
    c[ImGuiCol_WindowBg] = {bg.x, bg.y, bg.z, 0.80f};
    c[ImGuiCol_ChildBg] = {bg.x, bg.y, bg.z, 0.60f};
    c[ImGuiCol_PopupBg] = {bg.x, bg.y, bg.z, 0.96f};

    // borders — bright enough to read against the near-black background
    c[ImGuiCol_Border] = {0.28f, 0.28f, 0.28f, 1.00f};
    c[ImGuiCol_BorderShadow] = {0.00f, 0.00f, 0.00f, 0.00f};

    // frames — checkboxes, combos, sliders, inputs
    c[ImGuiCol_FrameBg] = {bg.x, bg.y, bg.z, 0.60f};
    c[ImGuiCol_FrameBgHovered] = {bg2.x, bg2.y, bg2.z, 0.80f};
    c[ImGuiCol_FrameBgActive] = {ac.x, ac.y, ac.z, 0.30f};

    // title bar — bright, contrasts the dark body
    c[ImGuiCol_TitleBg] = Theme::TitleBg;
    c[ImGuiCol_TitleBgActive] = Theme::TitleBg;
    c[ImGuiCol_TitleBgCollapsed] = Theme::TitleBg;

    c[ImGuiCol_MenuBarBg] = bg;

    // scrollbar
    c[ImGuiCol_ScrollbarBg] = {bg.x, bg.y, bg.z, 0.20f};
    c[ImGuiCol_ScrollbarGrab] = acd;
    c[ImGuiCol_ScrollbarGrabHovered] = ac;
    c[ImGuiCol_ScrollbarGrabActive] = ac;

    // checkmark / slider
    c[ImGuiCol_CheckMark] = ac;
    c[ImGuiCol_SliderGrab] = ac;
    c[ImGuiCol_SliderGrabActive] = fg;

    // buttons
    c[ImGuiCol_Button] = {bg.x, bg.y, bg.z, 0.60f};
    c[ImGuiCol_ButtonHovered] = {bg2.x, bg2.y, bg2.z, 0.80f};
    c[ImGuiCol_ButtonActive] = {ac.x, ac.y, ac.z, 0.40f};

    // headers (CollapsingHeader, Selectable, TreeNode)
    c[ImGuiCol_Header] = {ac.x, ac.y, ac.z, 0.25f};
    c[ImGuiCol_HeaderHovered] = {ac.x, ac.y, ac.z, 0.40f};
    c[ImGuiCol_HeaderActive] = {ac.x, ac.y, ac.z, 0.55f};

    // separators
    c[ImGuiCol_Separator] = {bg2.x, bg2.y, bg2.z, 1.00f};
    c[ImGuiCol_SeparatorHovered] = acd;
    c[ImGuiCol_SeparatorActive] = ac;

    // resize grip
    c[ImGuiCol_ResizeGrip] = {ac.x, ac.y, ac.z, 0.20f};
    c[ImGuiCol_ResizeGripHovered] = {ac.x, ac.y, ac.z, 0.60f};
    c[ImGuiCol_ResizeGripActive] = ac;

    // tabs
    c[ImGuiCol_Tab] = {bg.x, bg.y, bg.z, 0.80f};
    c[ImGuiCol_TabHovered] = {ac.x, ac.y, ac.z, 0.40f};
    c[ImGuiCol_TabActive] = {acd.x, acd.y, acd.z, 1.00f};
    c[ImGuiCol_TabUnfocused] = {bg2.x, bg2.y, bg2.z, 0.80f};
    c[ImGuiCol_TabUnfocusedActive] = {acd.x, acd.y, acd.z, 0.80f};

    // plots
    c[ImGuiCol_PlotLines] = ac;
    c[ImGuiCol_PlotLinesHovered] = fg;
    c[ImGuiCol_PlotHistogram] = ac;
    c[ImGuiCol_PlotHistogramHovered] = fg;

    // misc
    c[ImGuiCol_DragDropTarget] = ac;
    c[ImGuiCol_ModalWindowDimBg] = {bg.x, bg.y, bg.z, 0.60f};

    (void)acd; // used above
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void Menu::Render()
{
    bool bOpenLocal = bOpen.load();
    if (!bOpenLocal)
        return;

    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::SetNextWindowPos({io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f}, ImGuiCond_Once, {0.5f, 0.5f});
    ImGui::SetNextWindowSize({560.f, 440.f}, ImGuiCond_Always);

    // Title bar is TitleBg (bright); push TitleText so "variant" reads against it
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TitleText);
    ImGui::Begin("variant", &bOpenLocal,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoCollapse);
    ImGui::PopStyleColor();

    // Outer container — fills window body, provides the inner border
    ImGui::BeginChild("menu", ImVec2(), true, ImGuiWindowFlags_None);

    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, 100.f);

    // ----- Left sidebar -----
    // Square buttons, no frame/bg — active tab text turns accent colour
    const float flBtnSize = ImGui::GetColumnWidth(0) - style.FramePadding.x * 2.f - style.WindowPadding.x * 2.f - 1.f;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.f));

    ImGui::BeginChild("menu.sidebar", ImVec2(), true, ImGuiWindowFlags_NoScrollbar);

    // "VARIANT" logo above the tabs
    if (Render::pFontInterExtraBold)
    {
        ImGui::PushFont(Render::pFontInterExtraBold);
        const float textW = ImGui::CalcTextSize("VARIANT").x;
        const float avail = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - textW) * 0.5f);
        ImGui::TextUnformatted("VARIANT");
        ImGui::PopFont();
        ImGui::Dummy(ImVec2(0.f, 12.f));
    }

    const ImVec4 colAccent = Theme::Accent;

    if (Render::pFontIcons)
        ImGui::PushFont(Render::pFontIcons);

    static constexpr const char* tabIcons[] = {
        ICON_ARROWS_MINIMIZE, // tab 0 — ragebot
        ICON_ARROWS_MAXIMIZE, // tab 1 — anti aim
        ICON_BACKGROUND,      // tab 2 — visuals
        ICON_MENU,            // tab 3 — misc
    };

    // Space-evenly: distribute 4 buttons across remaining sidebar height
    // No leading gap — first icon starts immediately after the 24px title gap
    static constexpr int nTabs = 4;
    static constexpr float btnH = 36.f;
    const float availH = ImGui::GetContentRegionAvail().y;
    const float gap = (availH - nTabs * btnH) / nTabs;

    for (int i = 0; i < nTabs; i++)
    {
        if (i > 0)
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + gap);

        // Text color: accent for active, hover gray for hovered, default gray otherwise
        const ImVec2 btnPos = ImGui::GetCursorScreenPos();
        const bool bThisHovered = ImGui::IsMouseHoveringRect(btnPos, {btnPos.x + flBtnSize, btnPos.y + btnH});
        const ImVec4 colIcon = (nCurrentTab == i) ? colAccent
                               : bThisHovered     ? Theme::BorderHovered
                                                  : Theme::BorderDefault;
        ImGui::PushStyleColor(ImGuiCol_Text, colIcon);
        if (ImGui::Button(tabIcons[i], {flBtnSize, btnH}) && nCurrentTab != i)
            nCurrentTab = i;
        ImGui::PopStyleColor();
    }

    if (Render::pFontIcons)
        ImGui::PopFont();

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
    ImGui::EndChild(); // menu.sidebar

    // ----- Right content -----
    ImGui::NextColumn();

    constexpr float headingPadY = 8.f;
    const float headingH = ImGui::GetTextLineHeight() + headingPadY * 2.f + 2.f; // 2px for borders

    // Heading box — no WindowPadding; we position the text manually so it's pixel-perfect centered
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::BeginChild("menu.heading", ImVec2(0.f, headingH), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar();
    static constexpr const char* tabHeadings[] = {"RAGEBOT", "ANTI AIM", "VISUALS", "MISCELLANEOUS"};
    {
        const char* heading = tabHeadings[nCurrentTab];
        const float textH = ImGui::GetTextLineHeight();
        const float avail = ImGui::GetContentRegionAvail().y;
        ImGui::SetCursorPos(ImVec2(style.WindowPadding.x, (avail - textH) * 0.5f));
        ImGui::TextUnformatted(heading);
    }
    ImGui::EndChild(); // menu.heading

    ImGui::Spacing();

    // Content box
    ImGui::BeginChild("menu.content", ImVec2(), true, ImGuiWindowFlags_None);

    switch (nCurrentTab)
    {
    case 0:
        break;
    case 1:
        break;
    case 2:
        RenderVisuals();
        break;
    case 3:
        RenderMisc();
        break;
    }

    ImGui::EndChild(); // menu.content

    ImGui::EndChild(); // menu
    ImGui::Columns(1);

    bOpen.store(bOpenLocal);
    ImGui::End();
}
