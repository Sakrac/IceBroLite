#include "../imgui/imgui.h"

#include "../struse/struse.h"
#include "../Config.h"
#include "../Image.h"
#include "../Breakpoints.h"
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

    const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;

    if (ImGui::BeginTable("##breakpointstable", 3, flags)) {
        size_t numBreakpoints = NumBreakpoints();
        for(size_t bpIdx = 0; bpIdx < numBreakpoints; bpIdx++) {
            Breakpoint bp = GetBreakpoint(bpIdx);
            if (bp.number != 0xffffffff) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                DrawTexturedIcon(VMI_BreakPoint, false, ImGui::GetFont()->FontSize);
                ImGui::TableSetColumnIndex(1);
                strown<16> num;
                num.append_num(bp.number, 0, 10);
                ImGui::Text(num.c_str());
                ImGui::TableSetColumnIndex(2);
                num.clear();
                num.append('$').append_num(bp.start, 4, 16);
                if (bp.end != bp.start) {
                    num.append("-$").append_num(bp.end, 4, 16);
                }
                ImGui::Text(num.c_str());
            }
        }
        ImGui::EndTable();
    }
    ImGui::End();
}
