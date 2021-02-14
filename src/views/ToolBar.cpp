#include "../imgui/imgui.h"
#include "../struse/struse.h"
#include "../Image.h"
#include "../ViceInterface.h"
#include "../C64Colors.h"
#include "../Config.h"
#include "../FileDialog.h"
#include "../Sym.h"
#include "../StartVice.h"
#include "ToolBar.h"

ToolBar::ToolBar() : open(true) {}

void ToolBar::WriteConfig(UserData& config)
{
	config.AddValue(strref("open"), config.OnOff(open));
}

void ToolBar::ReadConfig(strref config)
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

bool CenterTextInColumn(const char* text)
{
	ImVec2 textSize = ImGui::CalcTextSize(text);
	textSize.x += 12.0f;
	textSize.y += 4.0f;
	ImGui::SetCursorPosX(0.5f * (ImGui::GetColumnWidth() - textSize.x) + ImGui::GetColumnOffset());
//	ImGui::Text( text );
	return ImGui::Button(text, textSize);
}

void ToolBar::Draw()
{
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(720, 64), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowDockID(1, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Toolbar", &open)) {
		ImGui::End();
		return;
	}

	ImGui::Columns(9, 0, false);

	bool connected = ViceConnected();
	bool playing = connected && ViceRunning();
	bool stopGo = DrawTexturedIconCenter(
		playing ? ViceMonIcons::VMI_Pause : ViceMonIcons::VMI_Play,
		false, -1.0f, connected ? (playing ? C64_PINK : C64_WHITE ) : C64_LGRAY);
	stopGo = CenterTextInColumn(playing ? "Pause" : "Go") || stopGo;

	ImGui::NextColumn();

	bool step = DrawTexturedIconCenter(ViceMonIcons::VMI_Step);
	step = CenterTextInColumn("Step") || step;

	ImGui::NextColumn();

	bool stepOver = DrawTexturedIconCenter(ViceMonIcons::VMI_Step);
	stepOver = CenterTextInColumn("Step Over") || stepOver;

	ImGui::NextColumn();

	bool stepOut = DrawTexturedIconCenter(ViceMonIcons::VMI_Step);
	stepOut = CenterTextInColumn("Step Out") || stepOut;

	ImGui::NextColumn();

	bool load = DrawTexturedIconCenter(ViceMonIcons::VMI_Load);
	load = CenterTextInColumn("Load") || load;

	ImGui::NextColumn();

	bool reload = DrawTexturedIconCenter(ViceMonIcons::VMI_Reload);
	reload = CenterTextInColumn("Reload") || reload;

	ImGui::NextColumn();

	bool reset = DrawTexturedIconCenter(ViceMonIcons::VMI_Reset);
	reset = CenterTextInColumn("Reset") || reset;

	ImGui::NextColumn();

	bool connect = DrawTexturedIconCenter(ViceConnected() ? ViceMonIcons::VMI_Connected : ViceMonIcons::VMI_Disconnected);
	connect = CenterTextInColumn("Connect") || connect;

	ImGui::NextColumn();

	bool viceToggle = DrawTexturedIconCenter(ViceConnected() ? ViceMonIcons::VMI_Connected : ViceMonIcons::VMI_Disconnected);
	viceToggle = CenterTextInColumn("Vice") || viceToggle;

	ImGui::Columns(1);
	ImGui::End();

	if (connected && stopGo) {
		if (playing) { ViceBreak(); }
		else { ViceGo(); }
	}

	if (step) { ViceStep(); }
	if (stepOver) { ViceStepOver(); }
	if (stepOut) { ViceStepOut(); }

//	if (stepBack && !ViceRunning()) { CPUStepBack(); }

	if (connect) {
		if (ViceConnected()) {
			ViceDisconnect();
		} else {
			ViceConnect("127.0.0.1", 6502);
		}
	}

	if (viceToggle) {
		if (connected) {
			ViceQuit();
		} else {
			LoadViceEXE();
		}
	}

	if (load) {
		LoadProgramDialog();
	}

	if (const char* loadPrg = LoadProgramReady()) {
		ViceStartProgram(loadPrg);
		ReadSymbolsForBinary(loadPrg);
	}
//
	if (reload) {
		if (const char* loadPrg = ReloadProgramFile()) {
			ViceStartProgram(loadPrg);
			ReadSymbolsForBinary(loadPrg);
		}
	}
//
//	if (NMI) { CPUNMI(); }
//
//	if (Interrupt) { CPUIRQ(); }
//
	if (reset) {
		ViceReset(1);
	}
//
//	if (IsFileLoadReady()) {
//		ImGui::OpenPopup("Load Binary");
//		FileLoadReadyAck();
//	}

	if (ImGui::BeginPopupModal("Load Binary", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		static int loadFileType = 0;
		ImGui::RadioButton("C64 PRG File", &loadFileType, 0);
		ImGui::RadioButton("Apple II Dos 3.3 File", &loadFileType, 1);
		ImGui::RadioButton("Raw Binary File", &loadFileType, 2);

		static bool setPCToLoadAddress = true;
		static bool resetBacktrace = true;
		static bool forceLoadTo = false;
		static char forceLoadAddress[16] = {};
		ImGui::Checkbox("Force Load To", &forceLoadTo); ImGui::SameLine();
		ImGui::InputText("Address", forceLoadAddress, 16, ImGuiInputTextFlags_CharsHexadecimal);
		ImGui::Checkbox("Set PC to Load Addres", &setPCToLoadAddress);
		ImGui::Checkbox("Reset back trace on Load", &resetBacktrace);

		if (ImGui::Button("OK", ImVec2(120, 0))) {
			int addr = (int)strref(forceLoadAddress).ahextoui();
			if (!addr) { addr = 0x1000; }
//			LoadBinary(loadFileType, addr, setPCToLoadAddress, forceLoadTo, resetBacktrace);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}
}
