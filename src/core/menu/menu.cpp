// Menu rendering - ImGui overlay

#include "menu.h"
#include "../variables.h"
#include "../../../dependencies/imgui/imgui.h"

void Menu::Render()
{
    bool bOpenLocal = bOpen.load();
    if (!bOpenLocal)
        return;

    ImGui::SetNextWindowSize(ImVec2(320, 240), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("variant", &bOpenLocal))
    {
        ImGui::Text("variant - CS2 utility");
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Visuals"))
        {
            ImGui::Checkbox("ESP", &Vars.bESP);
        }

        if (ImGui::CollapsingHeader("Misc"))
        {
            ImGui::Checkbox("Bunny Hop", &Vars.bBunnyHop);
        }

        ImGui::Separator();
        ImGui::TextDisabled("INSERT to toggle | END to unload");
    }
    bOpen.store(bOpenLocal);
    ImGui::End();
}
