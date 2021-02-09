#include "../imgui/imgui.h"
#include "../struse/struse.h"
#include "../Config.h"
#include "../Image.h"
#include "../Breakpoints.h"
#include "../Sym.h"
#include "../ViceInterface.h"
#include "Views.h"
#include "SymbolView.h"

SymbolView::SymbolView() : open(false), case_sensitive(true), start(0), end(0xffffffff)
{
    searchField[0] = 0;
    contextLabel[0] = 0;
    startStr[0] = 0;
    endStr[0] = 0;
}

void SymbolView::WriteConfig(UserData& config)
{
    config.AddValue(strref("open"), config.OnOff(open));
}

void SymbolView::ReadConfig(strref config)
{
    ConfigParse conf(config);
    while (!conf.Empty()) {
        strref name, value;
        ConfigParseType type = conf.Next(&name, &value);
        if (name.same_str("open") && type == ConfigParseType::CPT_Value) {
            open = !value.same_str("Off");
        }
    }
}

enum MyItemColumnID {
    SymbolColumnID_Address,
    SymbolColumnID_Symbol,
    SymbolColumnID_Section,
};

static void LimitHexStr(char* buf, size_t len)
{
    char* chk = buf, * fix = chk;
    while (*chk && len>1) {
        char c = *chk++;
        if (c == '$' || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
            *fix++ = c;
        }
        --len;
    }
    *fix = 0;
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
    ImGui::Columns(2);
    if (ImGui::InputText("start", startStr, sizeof(startStr))) {
        LimitHexStr(startStr, sizeof(startStr));
        if (*startStr == '$') { start = (uint32_t)strref(startStr + 1).ahextoui(); }
        else { start = (uint32_t)strref(startStr + 1).atoui();  }
    }
    ImGui::NextColumn();
    if (ImGui::InputText("end", endStr, sizeof(endStr))) {
        LimitHexStr(endStr, sizeof(endStr));
        if (*endStr == '$') { end = (uint32_t)strref(endStr + 1).ahextoui(); }
        else if (*endStr == 0) { end = 0xffffffff; }
        else{ end = (uint32_t)strref(startStr + 1).atoui(); }
    }
    ImGui::Columns(1);

    const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable |
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
        ImGuiTableFlags_ScrollY;

    ImVec2 outer_size(-FLT_MIN, 0.0f);
    if (ImGui::BeginTable("##symbolstable", 3, flags)) {
        ImGui::TableSetupColumn("Addr", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch, -1.0f, SymbolColumnID_Address);
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

        for (size_t s = 0, n = NumSymbolSearchMatches(); s < n; ++s) {
            const char* section;
            uint32_t address;
            const char* symbol = GetSymbolSearchMatch(s, &address, &section);
            if (address < start || address > end) { continue; }
            if (symbol) {
                ImGui::TableNextRow(s==1 ? ImGuiTableBgTarget_RowBg1 : 0);
                ImGui::TableSetColumnIndex(0);

                strown<16> str;
                str.append('$').append_num(address, address < 0x10000 ? 4 : 0, 16);
                ImGui::Text(str.c_str());

                ImGui::TableSetColumnIndex(1);
                ImGui::Text(symbol);

                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    SymbolDragDrop drag;
                    drag.address = address;
                    strovl lblStr(drag.symbol, sizeof(drag.symbol));
                    lblStr.copy(symbol); lblStr.c_str();
                    ImGui::SetDragDropPayload("AddressDragDrop", &drag, sizeof(drag));
                    ImGui::Text("%s: $%04x", symbol, address);
                    ImGui::EndDragDropSource();
                }

                ImGui::TableSetColumnIndex(2);
                ImGui::Text(section);
            }
        }

        ImGui::EndTable();
    }
    ImGui::End();
}