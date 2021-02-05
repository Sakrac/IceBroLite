#include <malloc.h>
#include "GLFW/glfw3.h"
#include "../imgui/imgui.h"
#include "../struse/struse.h"
#include "../ImGui_Helper.h"
#include "../Config.h"
#include "../Image.h"
#include "../6510.h"
#include "Views.h"
#include "RegView.h"

constexpr auto CursorFlashPeriod = 64.0f/50.0f;

RegisterView::RegisterView() : cursorTime(0.0f), open(true), wasActive(false)
{
	cursor = -1;
}

void RegisterView::WriteConfig(UserData& config)
{
	config.AddValue(strref("open"), config.OnOff(open));
}

void RegisterView::ReadConfig(strref config)
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

void RegisterView::Draw()
{
	if (!open) { return; }
	ImGui::SetNextWindowPos(ImVec2(600, 160), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(480, 40), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Registers", &open)) {
		ImGui::End();
		return;
	}

	float fontCharWidth = CurrFontSize();

	ImGui::BeginChild(ImGui::GetID("regEdit"));

	bool active = KeyboardCanvas("RegisterView");// IsItemActive();

	if (active && !wasActive) {
		cursorTime = 0.5f * CursorFlashPeriod;
	}

//	ImVec2 topPos = ImGui::GetCursorPos();

	ImGui::Text("ADDR A  X  Y  SP 00 01 NV-BDIZC LIN CYC");
	cursorTime += ImGui::GetIO().DeltaTime;
	if (cursorTime >= CursorFlashPeriod) { cursorTime -= CursorFlashPeriod; }

	if (ImGui::IsMouseClicked(0)) {
		ImVec2 mousePos = ImGui::GetMousePos();
		ImVec2 winPos = ImGui::GetWindowPos();
		ImVec2 winSize = ImGui::GetWindowSize();
		if (mousePos.x >= winPos.x && mousePos.y >= winPos.y &&
			mousePos.x < (winPos.x + winSize.x) && mousePos.y < (winPos.y + winSize.y)) {
			int clickPos = int((mousePos.x - winPos.x) / fontCharWidth);
			if (clickPos < 31) { cursor = clickPos; }
		}
	}

	CPU6510* cpu = GetCurrCPU();

	CPU6510::Regs& r = cpu->regs;
	strown<64> regs;
	regs.append_num(r.PC, 4, 16).append(' ');
	regs.append_num(r.A, 2, 16).append(' ');
	regs.append_num(r.X, 2, 16).append(' ');
	regs.append_num(r.Y, 2, 16).append(' ');
	regs.append_num(r.SP, 2, 16).append(' ');
	regs.append_num(r.ZP00, 2, 16).append(' ');
	regs.append_num(r.ZP01, 2, 16).append(' ');
	regs.append_num(r.FL, 8, 2).append(' ');
	regs.append_num(r.LIN, 3, 16).append(' ');
	regs.append_num(r.CYC, 3, 16).append(' ');
	ImVec2 curPos = ImGui::GetCursorPos();
	ImGui::Text(regs.c_str());

	if (active && cursor >= 0) {
		int o = cursor;
		uint8_t b = InputHex();
		if (b <= 0xf) {
			if (o < 4) {	// PC
				int bt = 4 * (3 - o);
				r.PC = (r.PC & (~(0xf << bt))) | (b << bt);
				ViceSetRegisters(*cpu, CPU6510::RM_PC);
				++cursor; if (cursor == 4) { ++cursor; }
			} else if (o >= 5 && o < 7) { // A
				int bt = 4 * (6 - o);
				r.A = (r.A & (~(0xf << bt))) | (b << bt);
				ViceSetRegisters(*cpu, CPU6510::RM_A);
				++cursor; if (cursor == 7) { ++cursor; }
			} else if (o >= 8 && o < 10) { // X
				int bt = 4 * (9 - o);
				r.X = (r.X & (~(0xf << bt))) | (b << bt);
				ViceSetRegisters(*cpu, CPU6510::RM_X);
				++cursor; if (cursor == 10) { ++cursor; }
			} else if (o >= 11 && o < 13) { // Y
				int bt = 4 * (12 - o);
				r.Y = (r.Y & (~(0xf << bt))) | (b << bt);
				ViceSetRegisters(*cpu, CPU6510::RM_Y);
				++cursor; if (cursor == 13) { ++cursor; }
			} else if (o >= 14 && o < 16) { // S
				int bt = 4 * (15 - o);
				r.SP = (r.SP & (~(0xf << bt))) | (b << bt);
				ViceSetRegisters(*cpu, CPU6510::RM_SP);
				++cursor; if (cursor == 16) { ++cursor; }
			} else if (o >= 17 && o < 19) { // 0
				int bt = 4 * (18 - o);
				r.ZP00 = (r.ZP00 & (~(0xf << bt))) | (b << bt);
				++cursor; if (cursor == 19) { ++cursor; }
				ViceSetRegisters(*cpu, CPU6510::RM_ZP00);
			} else if (o >= 20 && o < 22) { // 1
				int bt = 4 * (21 - o);
				r.ZP01 = (r.ZP01 & (~(0xf << bt))) | (b << bt);
				++cursor; if (cursor == 22) { ++cursor; }
				ViceSetRegisters(*cpu, CPU6510::RM_ZP01);
			} else if (b < 2 && o >= 23 && o < 31) {
				int bt = 30 - o;
				r.FL = (r.FL & (~(1 << bt))) | (b << bt);
				if (cursor < 30) { ++cursor; }
				ViceSetRegisters(*cpu, CPU6510::RM_FL);
			}
		}

		if (cursor && ImGui::IsKeyPressed(GLFW_KEY_LEFT)) { cursor--; }
		if (ImGui::IsKeyPressed(GLFW_KEY_RIGHT)) { cursor++; }
	}

	if (active && cursor >= 0 && cursorTime > (0.5f * CursorFlashPeriod)) {
		if ((uint32_t)cursor > regs.len()) {
			cursor = regs.len() - 1;
		}

		const ImGuiStyle style = ImGui::GetStyle();
		ImGui::SetCursorPos(ImVec2(curPos.x + fontCharWidth * cursor, curPos.y));
		const ImVec2 p = ImGui::GetCursorScreenPos();
		ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + fontCharWidth, p.y + fontCharWidth),
			ImColor(255, 255, 255));
		strown<16> curChr;
		curChr.append(regs[cursor]);
		ImGui::TextColored(style.Colors[ImGuiCol_ChildBg], curChr.c_str());
	}

	ImGui::EndChild();
	wasActive = active;
	ImGui::End();
}
