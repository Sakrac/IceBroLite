#include <assert.h>
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include "../struse/struse.h"
#include "ToolBar.h"
#include "RegView.h"
#include "MemView.h"
#include "BreakpointView.h"
#include "CodeView.h"
#include "ConsoleView.h"
#include "FilesView.h"
#include "ScreenView.h"
#include "WatchView.h"
#include "../6510.h"
#include "../data/C64_Pro_Mono-STYLE.ttf.h"
#include "../FileDialog.h"
#include "../SourceDebug.h"
#include "Views.h"
#include "GLFW/glfw3.h"


struct ViewContext {
	enum { sNumFontSizes = 7 };
	enum { MaxMemViews = 4, MaxCodeViews = 4, MaxWatchViews = 4 };
	ToolBar toolBar;
	RegisterView regView;
	MemView memView[MaxMemViews];
	CodeView codeView[MaxCodeViews];
	WatchView watchView[MaxWatchViews];
	BreakpointView breakView;
	ImFont* aFonts[sNumFontSizes];
	IceConsole console;

	ScreenView screenView;
	FVFileView fileView;
	int currFont;
	float currFontSize;
	bool setupDocking;

	ViewContext();
	void Draw();
	void GlobalKeyCheck();
};

void UseDefaultFont();

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

ViewContext::ViewContext() : currFont(3), setupDocking(true)
{
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	for (int fa = 0; fa < ViewContext::sNumFontSizes; ++fa) {
		int f = (fa + 3) % ViewContext::sNumFontSizes;
		aFonts[f] = io.Fonts->AddFontFromMemoryCompressedTTF(GetC64FontData(), GetC64FontSize(), sFontSizes[f], NULL, C64CharRanges);
		assert(aFonts[f] != NULL);
	}
	currFontSize = sFontSizes[currFont];
	memView[0].open = true;
	codeView[0].open = true;
	watchView[0].open = true;
	console.open = true;
}

void ViewContext::Draw()
{
//	ImGui::PushFont(aFonts[currFont]);
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
					for (int i = 0; i < MaxCodeViews; ++i) {
						strown<64> name("Code");
						name.append_num(i + 1, 1, 10).append(' ').append(codeView[i].address);
						if (ImGui::MenuItem(name.c_str(), NULL, codeView[i].open)) { codeView[i].open = !codeView[i].open; }
					}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Watch")) {
					for (int i = 0; i < MaxWatchViews; ++i) {
						strown<64> name("Watch");
						name.append_num(i + 1, 1, 10);
						if (ImGui::MenuItem(name.c_str(), NULL, watchView[i].open)) { watchView[i].open = !watchView[i].open; }
					}
					ImGui::EndMenu();
				}
				if (ImGui::MenuItem("Registers", NULL, regView.open)) { regView.open = !regView.open; }
				if (ImGui::MenuItem("Breakpoints", NULL, breakView.open)) { breakView.open = !breakView.open; }
				if (ImGui::MenuItem("Toolbar", NULL, toolBar.open)) { toolBar.open = !toolBar.open; }
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Font")) {
				for (int i = 0; i < sNumFontSizes; ++i) {
					strown<16> sz("Font Size ");
					sz.append_num(i, 0, 10);
					if (ImGui::MenuItem(sz.c_str())) { SelectFont(i); }
				}
				if (ImGui::MenuItem("ImGui default")) { UseDefaultFont(); }
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();

		}
	}

	if (setupDocking) {
		setupDocking = false;
		ImGuiID dockspace_id = ImGui::GetID("IceBroLiteDockSpace");

		ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace); // Add empty node
		ImGui::DockBuilderSetNodeSize(dockspace_id, ImVec2(ImGui::GetWindowWidth(), ImGui::GetWindowHeight()));

		ImGuiID dock_main_id = dockspace_id; // This variable will track the document node, however we are not using it here as we aren't docking anything into it.
		ImGuiID dock_id_toolbar = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Up, 0.10f, NULL, &dock_main_id);
		ImGuiID dock_id_code = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.33f, NULL, &dock_main_id);
		ImGuiID dock_id_code2 = ImGui::DockBuilderSplitNode(dock_id_code, ImGuiDir_Down, 0.25f, NULL, &dock_id_code);
		ImGuiID dock_id_code3 = ImGui::DockBuilderSplitNode(dock_id_code, ImGuiDir_Down, 0.33f, NULL, &dock_id_code);
		ImGuiID dock_id_code4 = ImGui::DockBuilderSplitNode(dock_id_code, ImGuiDir_Down, 0.5f, NULL, &dock_id_code);
		ImGuiID dock_id_regs = ImGui::DockBuilderSplitNode(dock_id_code, ImGuiDir_Up, 0.1f, NULL, &dock_id_code);
		ImGuiID dock_id_mem = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.5f, NULL, &dock_main_id);
		ImGuiID dock_id_watch = ImGui::DockBuilderSplitNode(dock_id_mem, ImGuiDir_Up, 0.33f, NULL, &dock_id_mem);
		ImGuiID dock_id_screen = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Up, 0.2f, NULL, &dock_main_id);
		ImGuiID dock_id_breaks = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Up, 0.2f, NULL, &dock_main_id);

		ImGui::DockBuilderDockWindow("Toolbar", dock_id_toolbar);
		ImGui::DockBuilderDockWindow("Vice Monitor", dock_main_id);
		ImGui::DockBuilderDockWindow("###Code1", dock_id_code);
		ImGui::DockBuilderDockWindow("###Code2", dock_id_code2);
		ImGui::DockBuilderDockWindow("###Code3", dock_id_code3);
		ImGui::DockBuilderDockWindow("###Code4", dock_id_code4);
		ImGui::DockBuilderDockWindow("Mem1", dock_id_mem);
		ImGui::DockBuilderDockWindow("Watch1", dock_id_watch);
		ImGui::DockBuilderDockWindow("Registers", dock_id_regs);
		ImGui::DockBuilderDockWindow("Screen", dock_id_screen);
		ImGui::DockBuilderDockWindow("Breakpoints", dock_id_breaks);
		ImGui::DockBuilderFinish(dockspace_id);
	}

	toolBar.Draw();
	regView.Draw();
	for (int m = 0; m < MaxMemViews; ++m) { memView[m].Draw(m); }
	for (int c = 0; c < MaxCodeViews; ++c) { codeView[c].Draw(c); }
	for (int w = 0; w < MaxWatchViews; ++w) { watchView[w].Draw(w); }
	console.Draw();
	screenView.Draw();
	breakView.Draw();

	fileView.Draw("Select File");
//	ImGui::PopFont();
	GlobalKeyCheck();
	ViceTickMessage();

	if (const char* kickDbgFile = LoadKickDbgReady()) {
		ReadC64DbgSrc(kickDbgFile);
	}
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
	return viewContext ? viewContext->currFontSize : 10.0f;
}

void SetCodeViewAddr(uint16_t addr)
{
	if (viewContext) {
		for (size_t i = 0; i < ViewContext::MaxCodeViews; ++i) {
			if (viewContext->codeView[i].open) {
				viewContext->codeView[i].SetAddr(addr);
				break;
			}
		}
	}
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
		viewContext->currFontSize = sFontSizes[size];
		ImGui::SetCurrentFont(viewContext->aFonts[viewContext->currFont]);
		GImGui->IO.FontDefault = viewContext->aFonts[viewContext->currFont];
	}
}

void UseDefaultFont()
{
	if (viewContext) {
		viewContext->currFontSize = 7;
		GImGui->IO.FontDefault = nullptr;
	}
}

void RefreshScreen(uint8_t* img, uint16_t w, uint16_t h)
{
	if (viewContext) {
		viewContext->screenView.Refresh(img, w, h);
	}
}

FVFileView* GetFileView()
{
	if (viewContext) {
		return &viewContext->fileView;
	}
	return nullptr;
}
