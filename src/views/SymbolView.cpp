#include "../imgui/imgui.h"
#include "../struse/struse.h"
#include "../Config.h"
#include "../Image.h"
#include "../Breakpoints.h"
#include "../Sym.h"
#include "Views.h"
#include "SymbolView.h"

SymbolView::SymbolView() : open(false), case_sensitive(true), selected_row(-1)
{
    searchField[0] = 0;

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

    if (ImGui::InputText("Search", searchField, kSearchFieldSize)) {
        SearchSymbols(searchField, case_sensitive);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("case", case_sensitive)) {
        case_sensitive = !case_sensitive;
        SearchSymbols(searchField, case_sensitive);
    }

    const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
        ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_ScrollY;

    ImVec2 cursorScreen = ImGui::GetCursorScreenPos();
    ImVec2 outer_size(-FLT_MIN, 0.0f);

    bool goToSymbol = false;
    float bpHitY = 0.0f;
    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        if (mousePos.x > cursorScreen.x && mousePos.y > cursorScreen.y &&
            mousePos.x < (winPos.x + winSize.x) && mousePos.y < (winPos.y + winSize.y)) {
            bpHitY = mousePos.y - cursorScreen.y + ImGui::GetScrollY();
            goToSymbol = true;
        }
    }

    float fontHgt = ImGui::GetFont()->FontSize;
    bool haveSymbols = SymbolsLoaded();
    int numColumns = haveSymbols ? 4 : 3;
    if (ImGui::BeginTable("##symbolstable", 3, flags)) {
        ImGui::TableSetupColumn("Addr", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Symbol", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Section ", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableHeadersRow();

        for (size_t s = 0, n = NumSymbolSearchMatches(); s < n; ++s) {
            const char* section;
            uint32_t address;
            const char* symbol = GetSymbolSearchMatch(s, &address, &section);
            if (symbol) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
                strown<32> selRow("##symbolRow_");
                selRow.append_num(s, 0, 10);
                bool item_is_selected = (size_t)selected_row == s;
                if (ImGui::Selectable(selRow.c_str(), item_is_selected, selectable_flags)) {
                    selected_row = (int)s;
                }
                ImGui::SameLine();

                strown<16> str;
                str.append('$').append_num(address, 0, 16);
                ImGui::Text(str.c_str());

                ImGui::TableSetColumnIndex(1);
                ImGui::Text(symbol);

                ImGui::TableSetColumnIndex(2);
                ImGui::Text(section);
            }
        }
        ImGui::EndTable();
    }
    ImGui::End();
}