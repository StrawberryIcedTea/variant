// Menu rendering - ImGui overlay

#include "menu.h"
#include "../variables.h"
#include "../../../dependencies/imgui/imgui.h"

void Menu::Render()
{
    bool bOpenLocal = bOpen.load();
    if (!bOpenLocal)
        return;

    ImGui::SetNextWindowSize(ImVec2(360, 300), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("variant", &bOpenLocal))
    {
        ImGui::Text("variant - CS2 utility");
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Visuals"))
        {
            ImGui::Checkbox("ESP", &Vars.bESP);
            ImGui::Separator();

            ImGui::Checkbox("Chams", &Vars.bChams);
            if (Vars.bChams)
            {
                static const char* matNames[] = {"Flat", "Glow"};

                ImGui::Combo("Material##chams", &Vars.nChamsMaterial, matNames, IM_ARRAYSIZE(matNames));
                ImGui::Checkbox("XQZ (through-wall)##chams", &Vars.bChamsXQZ);
                ImGui::Checkbox("Double Chams##chams", &Vars.bChamsDouble);
                if (Vars.bChamsDouble)
                    ImGui::Combo("Material 2##chams", &Vars.nChamsMaterial2, matNames, IM_ARRAYSIZE(matNames));

                ImGui::Checkbox("Enemies##chams", &Vars.bChamsEnemies);
                ImGui::SameLine();
                ImGui::Checkbox("Teammates##chams", &Vars.bChamsTeammates);
                ImGui::SameLine();
                ImGui::Checkbox("Self##chams", &Vars.bChamsSelf);

                ImGui::Spacing();
                if (Vars.bChamsDouble)
                {
                    // Unique IDs per pass so ImGui doesn't alias the two color rows
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

        if (ImGui::CollapsingHeader("Misc"))
        {
            static const char* modes[] = {"Disabled", "Normal", "Subtick"};
            ImGui::Combo("Bunny Hop", &Vars.nBhopMode, modes, IM_ARRAYSIZE(modes));
            ImGui::Combo("Auto Strafe", &Vars.nAutoStrafeMode, modes, IM_ARRAYSIZE(modes));
            ImGui::Checkbox("Jump Bug", &Vars.bJumpBug);
            ImGui::Checkbox("Edge Bug", &Vars.bEdgeBug);
        }

        ImGui::Separator();
        ImGui::TextDisabled("INSERT to toggle | END to unload");
    }
    bOpen.store(bOpenLocal);
    ImGui::End();
}
