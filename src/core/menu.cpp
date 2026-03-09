// Menu rendering - ImGui overlay

#include "menu.h"
#include "../../dependencies/imgui/imgui.h"

void Menu::Render()
{
    if (!bOpen.load())
        return;

    ImGui::SetNextWindowSize(ImVec2(320, 200), ImGuiCond_FirstUseEver);

    bool bOpenLocal = bOpen.load();
    if (ImGui::Begin("variant", &bOpenLocal))
    {
        ImGui::Text("variant - CS2 utility");
        ImGui::Separator();
        ImGui::Text("INSERT to toggle menu");
        ImGui::Text("END to unload");
    }
    bOpen.store(bOpenLocal);
    ImGui::End();
}
