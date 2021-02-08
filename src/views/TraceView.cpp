#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"

#include "../struse/struse.h"
#include "../Config.h"
#include "../Sym.h"
#include "../6510.h"
#include "../Mnemonics.h"
#include "../Traces.h"
#include "../ViceInterface.h"
#include "Views.h"
#include "TraceView.h"
#include "GLFW/glfw3.h"

#ifndef _WIN32
#define strncpy_s strncpy
#endif

TraceView::TraceView() : lastDrawnRows(1), row(0), open(false), mouseDrag(false)
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
		if (name.same_str("open") && type == ConfigParseType::CPT_Value) {
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

	const ImVec2 topLeft = ImGui::GetCursorScreenPos();

	size_t numTraceIds = NumTracePointIds();
	if (numTraceIds == 0) {
		ImGui::Text("Create a trace in the Console\nby entering tr <addr> [<addr2>]");
	} else {
		strown<16> idStr;
		if (tracePointNum >= 0 && tracePointNum < numTraceIds) {
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

		const ImGuiTableFlags flags = /*ImGuiTableFlags_Borders |*/ ImGuiTableFlags_RowBg |
			ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable |
			ImGuiTableFlags_BordersOuter;// | ImGuiTableFlags_BordersV;

		ImVec2 winPos = ImGui::GetWindowPos();
		ImVec2 winSize = ImGui::GetWindowSize();
		ImVec2 mousePos = ImGui::GetMousePos();
		size_t numRows = 0;

		if (mousePos.x >= winPos.x && mousePos.x < (winPos.x + winSize.x) &&
			mousePos.y >= ImGui::GetCursorPos().y && mousePos.y < (winPos.y + winSize.y)) {
			mouseWheelDiff += ImGui::GetIO().MouseWheel;
			if (mouseWheelDiff < -0.5f) {
				row++;
				mouseWheelDiff += 1.0f;
			}
			else if (mouseWheelDiff > 0.5) {
				if (row) { row--; };
				mouseWheelDiff -= 1.0f;
			}
		} else {
			mouseWheelDiff = 0.0f;
		}

		if (ImGui::IsKeyPressed(GLFW_KEY_PAGE_UP)) {
			row = row > (lastDrawnRows / 2) ? (row - lastDrawnRows / 2) : 0;
		}
		if (ImGui::IsKeyPressed(GLFW_KEY_PAGE_DOWN)) {
			row += lastDrawnRows / 2;
		}

		if (((size_t)row + lastDrawnRows) > numHits) {
			row = (size_t)lastDrawnRows < numHits ? (int)(numHits - lastDrawnRows) : 0;
		}

		size_t r = row;

		if (ImGui::BeginTable("##tracetable", 4, flags)) {
			ImGui::TableSetupColumn("addr", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("pc", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("time", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("frame", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
			ImGui::TableHeadersRow();

			while (r < numHits)
			{
				if (ImGui::GetCursorPosY() > (winSize.y - 15.0f)) { break; }
				ImGui::TableNextRow();
				ImVec2 curPos = ImGui::GetCursorScreenPos();
				TraceHit hit = GetTraceHit((int)tracePointNum, r++);
				strown<64> str;
				if( mousePos.x > winPos.x && mousePos.x < (winPos.x+winSize.x) &&
					mousePos.y > curPos.y && mousePos.y < (curPos.y + ImGui::GetFontSize())) {
					int chars = 0, branchTrg = 0;
					Disassemble(GetCurrCPU(), hit.pc, str.charstr(), str.cap(),
								chars, branchTrg, false, true, true, true);
					str.set_len(chars);
					str.append('\n');
					str.append("A:").append_num(hit.a, 2, 16).append(" X:").append_num(hit.x, 2, 16);
					str.append(" Y:").append_num(hit.y, 2, 16).append(" SP:").append_num(hit.sp, 2, 16);
					str.append(' ');
					str.append(hit.fl & 0x80 ? 'N' : '.');
					str.append(hit.fl & 0x40 ? 'V' : '.');
					str.append('-');
					str.append(hit.fl & 0x10 ? 'B' : '.');
					str.append(hit.fl & 0x08 ? 'D' : '.');
					str.append(hit.fl & 0x04 ? 'I' : '.');
					str.append(hit.fl & 0x02 ? 'Z' : '.');
					str.append(hit.fl & 0x01 ? 'C' : '.');
					ImGui::SetTooltip(str.c_str());
					str.clear();
				}
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
				++numRows;
			}
			lastDrawnRows = (int)(numRows + (size_t)(winSize.y - ImGui::GetCursorPosY()) / ImGui::GetFontSize());
			ImGui::EndTable();
		}

		if (numHits > 0 && lastDrawnRows > 0 && numHits > (size_t)lastDrawnRows) {
			size_t scrollBarHeight = (lastDrawnRows * size_t(winSize.y)) / numHits;
			size_t scrollBarTop = (size_t(row) * size_t(winSize.y)) / numHits;
			ImDrawList* draw_list = ImGui::GetWindowDrawList();

			float x = topLeft.x + winSize.x - 24.0f;
			float y = topLeft.y + (float)scrollBarTop;
			float h = (float)scrollBarHeight;
			bool hover = mousePos.x >= x && mousePos.x < (x + 14.0f) && mousePos.y >= y && mousePos.y < (y + h);
			const ImU32 col = (hover||mouseDrag) ? ImColor(192, 128, 0, 255) : ImColor(0, 0, 0, 96);

			if (hover && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				mouseYLast = mousePos.y;
				mouseDrag = true;
			}
			if (mouseDrag && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
				float delta = mousePos.y - mouseYLast;
				if (delta < -0.5f || delta > 0.5f) {
					mouseYLast = mousePos.y;
					row += (delta * (numHits - lastDrawnRows)) / winSize.y;
					if (row < 0) { row = 0; } else if (((size_t)row + lastDrawnRows) > numHits) {
						row = (size_t)lastDrawnRows < numHits ? (int)(numHits - lastDrawnRows) : 0;
					}
				}
			} else {
				mouseDrag = false;
			}
			draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + 14, y + h), col);
		} else {
			mouseDrag = false;
		}
		//winSize.y;

	}
	ImGui::End();
}

