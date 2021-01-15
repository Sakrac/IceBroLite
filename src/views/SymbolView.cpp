#include "../imgui/imgui.h"

#include "../struse/struse.h"
#include "../Config.h"
#include "../Image.h"
#include "../Breakpoints.h"
#include "../Sym.h"
#include "Views.h"
#include "SymbolView.h"

SymbolView::SymbolView() : open(false)
{

}

void SymbolView::WriteConfig(UserData& config)
{

}

void SymbolView::ReadConfig(strref config)
{

}

void SymbolView::Draw()
{
    if (!open) { return; }
    ImGui::SetNextWindowPos(ImVec2(400, 150), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(520, 400), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Symbols", &open)) {
        ImGui::End();
        return;
    }


    ImGui::End();
}