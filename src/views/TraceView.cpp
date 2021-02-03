#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"

#include "../struse/struse.h"
#include "../Config.h"
#include "../Sym.h"
#include "../Traces.h"
#include "../ViceInterface.h"
#include "Views.h"
#include "TraceView.h"
#include "GLFW/glfw3.h"

#ifndef _WIN32
#define strncpy_s strncpy
#endif

TraceView::TraceView() : open(false), row(0)
{
	tracePointNum = ~(size_t)0;
}

void TraceView::WriteConfig(UserData& config)
{
	config.AddValue(strref("open"), config.OnOff(open));
}

void TraceView::ReadConfig(strref config)
{
	ConfigParse conf(config);
	while (!conf.Empty()) {
		strref name, value;
		ConfigParseType type = conf.Next(&name, &value);
		if (name.same_str("open") && type == CPT_Value) {
			open = !value.same_str("Off");
		}
	}
}

void TraceView::Draw()
{
	if (!open) { return; }
	ImGui::SetNextWindowPos(ImVec2(400, 150), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(520, 400), ImGuiCond_FirstUseEver);

	if(!ImGui::Begin("Trace", &open)) {
		ImGui::End();
		return;
	}

	size_t numTraceIds = NumTracePointIds();
	if (numTraceIds == 0) {
		ImGui::Text("Create a trace in the Console\nby entering tr <addr> [<addr2>]");
	} else {
		strown<16> idStr;
		if (tracePointNum >= 0 && tracePointNum < (int)numTraceIds) {
			idStr.append_num(GetTracePointId(tracePointNum), 0, 10);
		} else { idStr.copy("?"); }
		if (ImGui::BeginCombo("Trace #", idStr.c_str())) {
			for (size_t i = 0; i < numTraceIds; ++i) {
				const bool is_selected = (tracePointNum == i);
				idStr.clear();
				idStr.append_num(GetTracePointId(i), 0, 10);
				if (ImGui::Selectable(idStr.c_str(), is_selected)) {
					tracePointNum = i;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		ImGui::InputInt("Row", &row);

		if (tracePointNum > numTraceIds) {
			ImGui::End();
			return;
		}

		size_t numHits = NumTraceHits(tracePointNum);

		ImGuiContext* g = ImGui::GetCurrentContext();

		const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
			ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable |
			ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV;

#if 0		
		ImVec2 cursorScreen = ImGui::GetCursorScreenPos();
		ImVec2 outer_size(-FLT_MIN, 0.0f);

		ImVec2 mousePos = ImGui::GetMousePos();
		ImVec2 winPos = ImGui::GetWindowPos();
		ImVec2 winSize = ImGui::GetWindowSize();

		float fontHgt = ImGui::GetFont()->FontSize;
		bool haveSymbols = SymbolsLoaded();
		int numColumns = haveSymbols ? 5 : 4;
//#endif
		ImVec2 winSize = ImGui::GetWindowSize();
		size_t r = row;

		if (ImGui::BeginTable("##tracetable", 5, flags)) {
			ImGui::TableSetupColumn("addr", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("pc", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("time", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("frame", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("regs", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
			ImGui::TableHeadersRow();

			while (r < numHits)
			{
				if (ImGui::GetCursorPosY() > (winSize.y - 15.0f)) { break; }
				ImGui::TableNextRow();
				TraceHit hit = GetTraceHit(tracePointNum, r);
				strown<64> str;
				ImGui::TableSetColumnIndex(0);
				str.append_num(hit.addr, 4, 16);
				ImGui::Text(str.c_str());
				ImGui::TableSetColumnIndex(1);
				str.clear();
				str.append_num(hit.pc, 4, 16);
				ImGui::Text(str.c_str());
				ImGui::TableSetColumnIndex(2);
				str.clear();
				str.append_num(hit.line, 0, 10).append('/').append_num(hit.cycle,0,10);
				ImGui::Text(str.c_str());


			}

			for (size_t bpIdx = 0; bpIdx < numBreakpoints; bpIdx++) {
				Breakpoint bp = GetBreakpoint(bpIdx);
				int col = 0;
				if (bp.number != 0xffffffff) {
					ImGui::TableNextRow(bpIdx == 1 ? ImGuiTableBgTarget_RowBg1 : 0);
					ImGui::TableSetColumnIndex(col++);
					if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
						ImVec2 scrCur = ImGui::GetCursorScreenPos();
						if (mousePos.y > scrCur.y && mousePos.y < (scrCur.y + fontHgt) && mousePos.x > winPos.x && mousePos.x < (winPos.x + winSize.x)) {
							selected_row = bpIdx;
							was_selected = true;
							SetSelected((int)selected_row);
						}
					}

					DrawTexturedIcon((bp.flags & Breakpoint::Enabled) ? VMI_BreakPoint : VMI_DisabledBreakPoint, false, ImGui::GetFont()->FontSize);
					ImGui::TableSetColumnIndex(col++);
					strown<64> num;
					if (bp.flags & Breakpoint::Current) { num.append('*'); }
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

						if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
							SymbolDragDrop drag;
							drag.address = bp.start;
							strovl lblStr(drag.symbol, sizeof(drag.symbol));
							lblStr.copy(num); lblStr.c_str();
							ImGui::SetDragDropPayload("AddressDragDrop", &drag, sizeof(drag));
							ImGui::Text("%s: $%04x", num.c_str(), bp.start);
							ImGui::EndDragDropSource();
						}
					}
					ImGui::TableSetColumnIndex(col++);
					if (bpIdx == selected_row) {
						ImGui::InputText("##bpCondition", conditionEdit, sizeof(conditionEdit));
					} else if (bp.condition) {
						ImGui::Text(bp.condition);
					}
				}
			}
			ImGui::EndTable();
		}
#endif
	}
	ImGui::End();
}

