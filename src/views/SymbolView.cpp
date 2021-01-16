#include "../imgui/imgui.h"
#include "../struse/struse.h"
#include "../Config.h"
#include "../Image.h"
#include "../Breakpoints.h"
#include "../Sym.h"
#include "../ViceInterface.h"
#include "Views.h"
#include "SymbolView.h"

SymbolView::SymbolView() : open(false), case_sensitive(true), selected_row(-1)
{
    searchField[0] = 0;
    contextLabel[0] = 0;
}

void SymbolView::WriteConfig(UserData& config)
{

}

void SymbolView::ReadConfig(strref config)
{

}

enum MyItemColumnID {
    SymbolColumnID_Address,
    SymbolColumnID_Symbol,
    SymbolColumnID_Section,
};


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
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable |
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
        ImGuiTableFlags_ScrollY;

    ImVec2 cursorScreen = ImGui::GetCursorScreenPos();
    ImVec2 outer_size(-FLT_MIN, 0.0f);

    bool goToSymbol = false;
    float bpHitY = 0.0f;
    ImVec2 winSize = ImGui::GetWindowSize();
    ImVec2 winPos = ImGui::GetWindowPos();
    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        ImVec2 mousePos = ImGui::GetMousePos();
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
        ImGui::TableSetupColumn("Addr", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed, -1.0f, SymbolColumnID_Address);
        ImGui::TableSetupColumn("Symbol", ImGuiTableColumnFlags_WidthStretch, -1.0f, SymbolColumnID_Symbol);
        ImGui::TableSetupColumn("Section ", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch, -1.0f, SymbolColumnID_Section);
        ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
        ImGui::TableHeadersRow();

        if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs()) {
            if (sorts_specs->SpecsDirty) {
                sorts_specs->SpecsDirty = false;
                SortSymbols(sorts_specs->Specs->SortDirection == ImGuiSortDirection_Ascending, sorts_specs->Specs->ColumnUserID == SymbolColumnID_Symbol);
                SearchSymbols(searchField, case_sensitive);
            }
        }

        int hovered_row = -1;
        uint32_t hovered_addr = 0;
        const char* hovName = nullptr;
        const char* hovered_label = 0;
        for (size_t s = 0, n = NumSymbolSearchMatches(); s < n; ++s) {
            const char* section;
            uint32_t address;
            const char* symbol = GetSymbolSearchMatch(s, &address, &section);
            if (symbol) {
                ImGui::TableNextRow();

                ImVec2 ul = ImGui::GetCursorPos();
                ul.x += winPos.x; ul.y += winPos.y;
                ImVec2 br(ul.x + winSize.x, ul.y + fontHgt);
                if (ImGui::IsMouseHoveringRect(ul, br)) {
                }



                ImGui::TableSetColumnIndex(0);

                strown<16> str;
                str.append('$').append_num(address, address < 0x10000 ? 4 : 0, 16);
                ImGui::Text(str.c_str());

                bool hov = ImGui::IsItemHovered();
                ImGui::TableSetColumnIndex(1);
                ImGui::PushID(address);
                ImGui::Text(symbol);

                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    // Set payload to carry the index of our item (could be anything)
                    ImGui::SetDragDropPayload("AddressDragDrop", &address, sizeof(uint32_t));
                    // Display preview (could be anything, e.g. when dragging an image we could decide to display
                    // the filename and a small preview of the image, etc.)
                    ImGui::Text("%s: $%04x", symbol, address);
                    ImGui::EndDragDropSource();
                }
                ImGui::PopID();


                hov = hov || ImGui::IsItemHovered();
                ImGui::TableSetColumnIndex(2);
                ImGui::Text(section);

                if (hov || ImGui::IsItemHovered()) {
                    hovered_row = (int)s;
                    hovered_addr = address;
                    hovName = symbol;
                }
            }
        }

        if (hovered_row >= 0 && ImGui::IsMouseReleased(1)) {
            context_row = hovered_row;
            context_address = hovered_addr;
            strovl ctxSym(contextLabel, kContextSymbolSize);
            ctxSym.copy(hovName);
            ctxSym.c_str();
            ImGui::OpenPopup("SymbolContextMenu", ImGuiPopupFlags_NoOpenOverExistingPopup);
        }

        if (ImGui::BeginPopup("SymbolContextMenu")) {
            ImGui::Text("%s $%04x", contextLabel, context_address);
            if (ImGui::MenuItem("Add Breakpoint")) { ViceAddBreakpoint(context_address); }
            if (ImGui::MenuItem("Set Code 1")) { SetCodeViewAddr(context_address, 0); ImGui::CloseCurrentPopup(); }
            if (ImGui::MenuItem("Set Code 2")) { SetCodeViewAddr(context_address, 1); ImGui::CloseCurrentPopup(); }
            if (ImGui::MenuItem("Set Code 3")) { SetCodeViewAddr(context_address, 2); ImGui::CloseCurrentPopup(); }
            if (ImGui::MenuItem("Set Code 4")) { SetCodeViewAddr(context_address, 3); ImGui::CloseCurrentPopup(); }
            if (ImGui::MenuItem("Set Memory 1")) { SetMemoryViewAddr(context_address, 0); ImGui::CloseCurrentPopup(); }
            if (ImGui::MenuItem("Set Memory 2")) { SetMemoryViewAddr(context_address, 1); ImGui::CloseCurrentPopup(); }
            if (ImGui::MenuItem("Set Memory 3")) { SetMemoryViewAddr(context_address, 2); ImGui::CloseCurrentPopup(); }
            if (ImGui::MenuItem("Set Memory 4")) { SetMemoryViewAddr(context_address, 3); ImGui::CloseCurrentPopup(); }
            if (ImGui::Button("Close")) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }


        ImGui::EndTable();
    }
    ImGui::End();
}