#include "../imgui/imgui.h"

#include "../struse/struse.h"
#include "../Config.h"
#include "../Image.h"
#include "../Breakpoints.h"
#include "../Sym.h"
#include "../ViceInterface.h"
#include "Views.h"
#include "BreakpointView.h"

BreakpointView::BreakpointView() : open(true), selected_row(-1)
{

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

	const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
		ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable |
		ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
		ImGuiTableFlags_ScrollY;

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
		ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
		ImGui::TableHeadersRow();

		for(size_t bpIdx = 0; bpIdx < numBreakpoints; bpIdx++) {
			Breakpoint bp = GetBreakpoint(bpIdx);
			int col = 0;

			if (bp.number != 0xffffffff) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(col++);
		 
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
						ImGui::Text("%s: $%04x", num, bp.start);
						ImGui::EndDragDropSource();
					}
				}
			}
		}
		ImGui::EndTable();
	}
	ImGui::End();
}
