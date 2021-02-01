#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"

#include "../struse/struse.h"
#include "../Config.h"
#include "../Image.h"
#include "../Breakpoints.h"
#include "../Sym.h"
#include "../ViceInterface.h"
#include "../C64Colors.h"
#include "../ImGui_Helper.h"
#include "Views.h"
#include "BreakpointView.h"
#include "GLFW/glfw3.h"

#ifndef _WIN32
#define strncpy_s strncpy
#endif

BreakpointView::BreakpointView() : open(true), selected_row(-1)
{
	conditionEdit[0] = 0;
}

void BreakpointView::WriteConfig(UserData& config)
{
	config.AddValue(strref("open"), config.OnOff(open));
}

void BreakpointView::ReadConfig(strref config)
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

void BreakpointView::Draw()
{
	ImGui::SetNextWindowPos(ImVec2(400, 150), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(520, 400), ImGuiCond_FirstUseEver);

	bool active = ImGui::Begin("Breakpoints", &open);
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AddressDragDrop")) {
			IM_ASSERT(payload->DataSize == sizeof(SymbolDragDrop));
			SymbolDragDrop* drop = (SymbolDragDrop*)payload->Data;
			if (drop->address < 0x10000) {
				ViceAddBreakpoint((uint16_t)drop->address);
				open = true;
			}
		}
		ImGui::EndDragDropTarget();
	}
	if( !active) {
		ImGui::End();
		return;
	}

	ImGuiContext* g = ImGui::GetCurrentContext();

	const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
		ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable |
		ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
		ImGuiTableFlags_ScrollY;

	ImVec2 cursorScreen = ImGui::GetCursorScreenPos();
	ImVec2 outer_size(-FLT_MIN, 0.0f);

	ImVec2 mousePos = ImGui::GetMousePos();
	ImVec2 winPos = ImGui::GetWindowPos();
	ImVec2 winSize = ImGui::GetWindowSize();

	float fontHgt = ImGui::GetFont()->FontSize;
	bool haveSymbols = SymbolsLoaded();
	int numColumns = haveSymbols ? 5 : 4;
	if (ImGui::BeginTable("##breakpointstable", 5, flags)) {
		size_t numBreakpoints = NumBreakpoints();

		ImGui::TableSetupColumn("B", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Addr ", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Condition", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
		ImGui::TableHeadersRow();

		size_t prior_valid = 0xffffffff;
		size_t next_valid = 0xffffffff;
		bool prev_selected = false;
		bool was_selected = false;

		for(size_t bpIdx = 0; bpIdx < numBreakpoints; bpIdx++) {
			Breakpoint bp = GetBreakpoint(bpIdx);
			int col = 0;
			if (bp.number != 0xffffffff) {
				if (bpIdx == selected_row) {
					prev_selected = true; was_selected = true;
				} else if (!was_selected) {
					prior_valid = bpIdx;
				} else if (prev_selected) {
					next_valid = bpIdx;
					prev_selected = false;
				}

				ImGui::TableNextRow(bpIdx == 1 ? ImGuiTableBgTarget_RowBg1 : 0);

				if (g->CurrentWindow == g->NavWindow && bpIdx == selected_row) {
					ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImColor(C64_PURPLE));
				}

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
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
					SymbolDragDrop drag;
					drag.address = bp.start;
					strovl lblStr(drag.symbol, sizeof(drag.symbol));
					lblStr.copy(num); lblStr.c_str();
					ImGui::SetDragDropPayload("AddressDragDrop", &drag, sizeof(drag));
					ImGui::Text(num.c_str());
					ImGui::EndDragDropSource();
				}
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
		if (!was_selected) {
			selected_row = prior_valid;
			SetSelected((int)selected_row);
		}
		if (selected_row != 0xffffffff && g->CurrentWindow == g->NavWindow) {
			if (prior_valid != 0xffffffff && ImGui::IsKeyPressed(GLFW_KEY_UP)) {
				selected_row = prior_valid; 
				SetSelected((int)selected_row);
			} else if (next_valid != 0xffffffff && ImGui::IsKeyPressed(GLFW_KEY_DOWN)) {
				selected_row = next_valid;
				SetSelected((int)selected_row);
			}
			if (ImGui::IsKeyPressed(GLFW_KEY_DELETE)) {
				Breakpoint bp = GetBreakpoint(selected_row);
				ViceRemoveBreakpoint(bp.number);
			}
		}
		ImGui::EndTable();
	}
	ImGui::End();
}

void BreakpointView::SetSelected(int index)
{
	conditionEdit[0] = 0;
	if (index > 0 && index < NumBreakpoints()) {
		Breakpoint bp = GetBreakpoint((size_t)index);
		if (bp.condition) {
			strncpy_s(conditionEdit, bp.condition, sizeof(conditionEdit));
		}
	}
}
