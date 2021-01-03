#include <assert.h>
#include "../imgui/imgui.h"
#include "../struse/struse.h"
#include "ToolBar.h"
#include "RegView.h"
#include "MemView.h"
#include "CodeView.h"
#include "ConsoleView.h"
#include "FilesView.h"
#include "../6510.h"
#include "../data/C64_Pro_Mono-STYLE.ttf.h"
#include "../FileDialog.h"
#include "Views.h"
#include "GLFW/glfw3.h"


struct ViewContext {
	enum { sNumFontSizes = 7 };
	enum { MaxMemViews = 4, MaxCodeViews = 4 };
	ToolBar toolBar;
	RegisterView regView;
	MemView memView[MaxMemViews];
	CodeView codeView[MaxCodeViews];
	ImFont* aFonts[sNumFontSizes];
	IceConsole console;
	FVFileView fileView;
	int currFont;

	ViewContext();
	void Draw();
	void GlobalKeyCheck();
};

static ViewContext* viewContext = nullptr;
static float sFontSizes[ViewContext::sNumFontSizes] = { 8.0f, 10.0f, 12.0, 14.0f, 16.0f, 20.0f, 24.0f };
static const ImWchar C64CharRanges[] =
{
	0x0020, 0x00FF, // Basic Latin + Latin Supplement
	0xee00, 0xeeff,
	0
	//	0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
	//	0x2DE0, 0x2DFF, // Cyrillic Extended-A
	//	0xA640, 0xA69F, // Cyrillic Extended-B
	//	0,
};

ViewContext::ViewContext() : currFont(1)
{
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	for (int f = 0; f < ViewContext::sNumFontSizes; ++f) {
		aFonts[f] = io.Fonts->AddFontFromMemoryCompressedTTF(GetC64FontData(), GetC64FontSize(), sFontSizes[f], NULL, C64CharRanges);
		assert(aFonts[f] != NULL);
	}
	memView[0].open = true;
	codeView[0].open = true;
	console.open = true;
}

void ViewContext::Draw()
{
	ImGui::PushFont(aFonts[currFont]);

	{
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				//bool updatingViceSymbls = ViceUpdatingSymbols();
				if (ImGui::MenuItem("Load KickAsm Debug")) { LoadKickDbgDialog(); }
				if (ImGui::MenuItem("Load Listing")) { LoadListingDialog(); }
				if (ImGui::MenuItem("Load Sym File")) { LoadSymbolsDialog(); }
				if (ImGui::MenuItem("Load Vice Command Symbols")) { LoadViceCmdDialog(); }
//				if (ImGui::MenuItem("Update Symbols With Vice Sync", nullptr, updatingViceSymbls)) { ViceSetUpdateSymbols(!updatingViceSymbls); }

				if (ImGui::MenuItem("Quit", "Alt+F4")) {}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Windows")) {
				if (ImGui::MenuItem("Ice Console", NULL, console.open)) { console.open = !console.open; }
				if (ImGui::BeginMenu("Memory")) {
					for (int i = 0; i < MaxMemViews; ++i) {
						strown<64> name("Mem");
						name.append_num(i + 1, 1, 10).append(' ').append(memView[i].address);
						if (ImGui::MenuItem(name.c_str(), NULL, memView[i].open)) { memView[i].open = !memView[i].open; }
					}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Code")) {
					for (int i = 0; i < 4; ++i) {
						strown<64> name("Code");
						name.append_num(i + 1, 1, 10).append(' ').append(codeView[i].address);
						if (ImGui::MenuItem(name.c_str(), NULL, codeView[i].open)) { codeView[i].open = !codeView[i].open; }
					}
					ImGui::EndMenu();
				}
				if (ImGui::MenuItem("Registers", NULL, regView.open)) { regView.open = !regView.open; }
				if (ImGui::MenuItem("Toolbar", NULL, toolBar.open)) { toolBar.open = !toolBar.open; }
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();

		}
	}
	toolBar.Draw();
	regView.Draw();
	for (int m = 0; m < MaxMemViews; ++m) { memView[m].Draw(m); }
	for (int c = 0; c < MaxCodeViews; ++c) { codeView[c].Draw(c); }
	console.Draw();

	fileView.Draw("Select File");
	ImGui::PopFont();
	if (CPU6510* cpu = GetCurrCPU()) {
		cpu->RefreshMemory();
	}
	GlobalKeyCheck();
	ViceTickMessage();
}

void ViewContext::GlobalKeyCheck()
{
//	CheckRegChange();

	bool shift = ImGui::IsKeyDown(GLFW_KEY_LEFT_SHIFT) || ImGui::IsKeyDown(GLFW_KEY_RIGHT_SHIFT);
	bool ctrl = ImGui::IsKeyDown(GLFW_KEY_LEFT_CONTROL) || ImGui::IsKeyDown(GLFW_KEY_RIGHT_CONTROL);

	if (ImGui::IsKeyPressed(GLFW_KEY_F5, false)) {
		ViceGo();
//		if (ctrl) {} else if (shift) { CPUReverse(); } else { CPUGo(); }
	}
	if (ImGui::IsKeyPressed(GLFW_KEY_F10, false)) {
		ViceStepOver();
//		if (ctrl) { StepOverVice(); } else if (shift) { StepOverBack(); } else { StepOver(); }
	}
	if (ImGui::IsKeyPressed(GLFW_KEY_F11, false)) {
		if (ctrl) { /*StepInstructionVice();*/ } else if (shift) { ViceStepOut(); } else { ViceStep(); }
	}
}

void InitViews()
{
	viewContext = new ViewContext;

}

void ShowViews()
{
	if (viewContext) {
		viewContext->Draw();
	}
}

float CurrFontSize()
{
	return viewContext ? sFontSizes[viewContext->currFont] : 10.0f;
}

uint8_t InputHex()
{
	for (int num = 0; num < 9; ++num) { if (ImGui::IsKeyPressed(num + '0')) return num; }
	for (int num = 10; num < 16; ++num) { if (ImGui::IsKeyPressed(num + 'A' - 10)) return num; }
	return 0xff;
}

void SelectFont(int size)
{
	if (viewContext && size >= 0 && size < ViewContext::sNumFontSizes) {
		viewContext->currFont = size;
	}
}

FVFileView* GetFileView()
{
	if (viewContext) {
		return &viewContext->fileView;
	}
	return nullptr;
}
