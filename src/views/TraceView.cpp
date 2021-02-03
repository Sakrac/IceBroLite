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
#endif
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
				TraceHit hit = GetTraceHit((int)tracePointNum, r++);
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
				ImGui::TableSetColumnIndex(3);
				str.clear();
				str.append_num(hit.frame, 0, 10);
				ImGui::Text(str.c_str());
				ImGui::TableSetColumnIndex(4);
				str.clear();
				str.append("A:").append_num(hit.a, 2, 16).append(" X:").append_num(hit.x, 2, 16);
				str.append(" Y:").append_num(hit.y, 2, 16).append(" SP:").append_num(hit.sp, 2, 16);
				str.append(' ').append_num(hit.fl, 8, 2);
				ImGui::Text(str.c_str());
			}

			ImGui::EndTable();
		}
	}
	ImGui::End();
}

