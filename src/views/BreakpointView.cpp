#include "../imgui/imgui.h"

#include "../struse/struse.h"
#include "../Config.h"
#include "../Image.h"
#include "../Breakpoints.h"
#include "../Sym.h"
#include "Views.h"
#include "BreakpointView.h"

BreakpointView::BreakpointView() : open(true)
{

}

void BreakpointView::WriteConfig(UserData& config)
{

}

void BreakpointView::ReadConfig(strref config)
{

}

void BreakpointView::Draw()
{
    ImGui::SetNextWindowPos(ImVec2(400, 150), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(520, 400), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Breakpoints", &open)) {
        ImGui::End();
        return;
    }

    const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | 
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
        ImGuiTableFlags_ContextMenuInBody;

    ImVec2 cursorScreen = ImGui::GetCursorScreenPos();
    ImVec2 outer_size(-FLT_MIN, 0.0f);

    bool goToBP = false;
    float bpHitY = 0.0f;
    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        if (mousePos.x > cursorScreen.x && mousePos.y > cursorScreen.y &&
            mousePos.x < (winPos.x + winSize.x) && mousePos.y < (winPos.y + winSize.y)) {
            bpHitY = mousePos.y - cursorScreen.y + ImGui::GetScrollY();
            goToBP = true;
        }
    }

    float fontHgt = ImGui::GetFont()->FontSize;
    bool haveSymbols = SymbolsLoaded();
    int numColumns = haveSymbols ? 4 : 3;
    if (ImGui::BeginTable("##breakpointstable", 4, flags)) {
        size_t numBreakpoints = NumBreakpoints();

        ImGui::TableSetupColumn("B", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Addr ", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);

        for(size_t bpIdx = 0; bpIdx < numBreakpoints; bpIdx++) {
            Breakpoint bp = GetBreakpoint(bpIdx);
            int col = 0;
            if (goToBP) {
                float y = ImGui::GetCursorPosY() - bpHitY;
                if (y > 0 && y < (fontHgt + 3.0f)) {
                    SetCodeViewAddr(bp.start);
                }
            }
            if (bp.number != 0xffffffff) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(col++);
                DrawTexturedIcon(VMI_BreakPoint, false, ImGui::GetFont()->FontSize);
                ImGui::TableSetColumnIndex(col++);
                strown<64> num;
                num.append_num(bp.number, 0, 10);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(num.c_str()).x
                                     - ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x);
                ImGui::Text(num.c_str());
                ImGui::TableSetColumnIndex(col++);
                num.clear();
                num.append('$').append_num(bp.start, 4, 16);
                if (bp.end != bp.start) {
                    num.append("-$").append_num(bp.end, 4, 16);
                }
                ImGui::Text(num.c_str());
                ImGui::TableSetColumnIndex(col++);
                uint16_t offs;
                if (const char* label = NearestLabel(bp.start, offs)) {
                    num.clear();
                    num.append(label);
                    if (offs) { num.append("+$").append_num(offs, 4, 16); }
                    ImGui::Text(num.c_str());
                }
            }
        }
        ImGui::EndTable();
    }
    ImGui::End();
}
