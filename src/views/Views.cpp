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
#include "SymbolView.h"
#include "SectionView.h"
#include "GfxView.h"
#include "PreView.h"
#include "../6510.h"
#include "../Config.h"
#include "../data/C64_Pro_Mono-STYLE.ttf.h"
#include "../FileDialog.h"
#include "../SourceDebug.h"
#include "Views.h"
#include "GLFW/glfw3.h"

struct ViewContext {
	enum { sNumFontSizes = 7 };
	enum { MaxMemViews = 4, MaxCodeViews = 4, MaxWatchViews = 4, kMaxGfxViews = 2 };
	ToolBar toolBar;
	RegisterView regView;
	MemView memView[MaxMemViews];
	CodeView codeView[MaxCodeViews];
	WatchView watchView[MaxWatchViews];
	GfxView gfxView[kMaxGfxViews];
	BreakpointView breakView;
	SymbolView symbolView;
	SectionView sectionView;
	IceConsole console;
	ScreenView screenView;
	FVFileView fileView;
	PreView preView;

	ImFont* aFonts[sNumFontSizes];

	int currFont;
	float currFontSize;
	bool setupDocking;
	bool memoryWasChanged;
	bool saveSettingsOnExit;

	ViewContext();
	void SaveState(UserData& conf);
	void LoadState(strref config);
	void Draw();
	void GlobalKeyCheck();
};

void UserSaveLayout();
void UseDefaultFont();
void StyleC64();
void StyleC64_Darker();
void StyleC64_Mid();


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
void ResetWindowLayout()
{
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
	ImGuiID dock_id_symbols = ImGui::DockBuilderSplitNode(dock_id_code, ImGuiDir_Down, 0.25f, NULL, &dock_id_code);

	ImGui::DockBuilderDockWindow("Toolbar", dock_id_toolbar);
	ImGui::DockBuilderDockWindow("Vice Monitor", dock_main_id);
	ImGui::DockBuilderDockWindow("###Code1", dock_id_code);
	ImGui::DockBuilderDockWindow("###Code2", dock_id_code2);
	ImGui::DockBuilderDockWindow("###Code3", dock_id_code3);
	ImGui::DockBuilderDockWindow("###Code4", dock_id_code4);
	ImGui::DockBuilderDockWindow("Mem1", dock_id_mem);
	ImGui::DockBuilderDockWindow("Graphics1", dock_id_mem);
	ImGui::DockBuilderDockWindow("Watch1", dock_id_watch);
	ImGui::DockBuilderDockWindow("Registers", dock_id_regs);
	ImGui::DockBuilderDockWindow("Screen", dock_id_screen);
	ImGui::DockBuilderDockWindow("Breakpoints", dock_id_breaks);
	ImGui::DockBuilderDockWindow("Symbols", dock_id_symbols);
	ImGui::DockBuilderDockWindow("Sections", dock_id_symbols);
	ImGui::DockBuilderFinish(dockspace_id);
}
ViewContext::ViewContext() : currFont(3), setupDocking(true), saveSettingsOnExit(true)
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

void ViewContext::SaveState(UserData& conf)
{
	// ToolBar toolBar;
	conf.BeginStruct("ToolBar"); toolBar.WriteConfig(conf); conf.EndStruct();
	// RegisterView regView;
	conf.BeginStruct("Registers"); regView.WriteConfig(conf); conf.EndStruct();
	// MemView memView[MaxMemViews];
	conf.BeginArray("Memory");
	for (int i = 0; i < MaxMemViews; ++i) {
		conf.BeginStruct(); memView[i].WriteConfig(conf); conf.EndStruct();
	}
	conf.EndArray();
	// CodeView codeView[MaxCodeViews];
	conf.BeginArray("Code");
	for (int i = 0; i < MaxCodeViews; ++i) {
		conf.BeginStruct(); codeView[i].WriteConfig(conf); conf.EndStruct();
	}
	conf.EndArray();
	// WatchView watchView[MaxWatchViews];
	conf.BeginArray("Watch");
	for (int i = 0; i < MaxWatchViews; ++i) {
		conf.BeginStruct(); watchView[i].WriteConfig(conf); conf.EndStruct();
	}
	conf.EndArray();
	// GfxView gfxView[kMaxGfxViews];
	conf.BeginArray("Graphics");
	for (int i = 0; i < kMaxGfxViews; ++i) {
		conf.BeginStruct(); gfxView[i].WriteConfig(conf); conf.EndStruct();
	}
	conf.EndArray();
	// BreakpointView breakView;
	conf.BeginStruct("Breakpoits"); breakView.WriteConfig(conf); conf.EndStruct();
	// SymbolView symbolView;
	conf.BeginStruct("Symbols"); symbolView.WriteConfig(conf); conf.EndStruct();
	// SectionView sectionView;
	conf.BeginStruct("Sections"); sectionView.WriteConfig(conf); conf.EndStruct();
	// ImFont* aFonts[sNumFontSizes];
	// IceConsole console;
	conf.BeginStruct("Console"); console.WriteConfig(conf); conf.EndStruct();
	// ScreenView screenView;
	conf.BeginStruct("Screen"); screenView.WriteConfig(conf); conf.EndStruct();
}

void ViewContext::LoadState(strref config)
{
	ConfigParse conf(config);
	while (!conf.Empty()) {
		strref name, value;
		ConfigParseType type = conf.Next(&name, &value);
		if (type == CPT_Struct) {
			if (name.same_str("ToolBar")) { toolBar.ReadConfig(value); }
			else if (name.same_str("Registers")) { regView.ReadConfig(value); }
			else if (name.same_str("Breakpoints")) { breakView.ReadConfig(value); }
			else if (name.same_str("Symbols")) { symbolView.ReadConfig(value); }
			else if (name.same_str("Sections")) { sectionView.ReadConfig(value); }
			else if (name.same_str("Console")) { console.ReadConfig(value); }
			else if (name.same_str("Screen")) { screenView.ReadConfig(value); }
		} else if (type == CPT_Array) {
			size_t i = 0;
			if (name.same_str("Code")) {
				ConfigParse code(value);
				while(!code.Empty() && i < MaxCodeViews) {
					codeView[i++].ReadConfig(code.ArrayElement());
				}
			} else if (name.same_str("Memory")) {
				ConfigParse mem(value);
				while (!mem.Empty() && i < MaxMemViews) {
					memView[i++].ReadConfig(mem.ArrayElement());
				}
			} else if (name.same_str("Watch")) {
				ConfigParse watch(value);
				while (!watch.Empty() && i < MaxWatchViews) {
					watchView[i++].ReadConfig(watch.ArrayElement());
				}
			} else if (name.same_str("Graphics")) {
				ConfigParse gfx(value);
				while (!gfx.Empty() && i < kMaxGfxViews) {
					gfxView[i++].ReadConfig(gfx.ArrayElement());
				}
			}
		}
	}
}

void ViewContext::Draw()
{
	memoryWasChanged = GetCurrCPU()->MemoryChange();
	{
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Load KickAsm Debug")) { LoadKickDbgDialog(); }
				if (ImGui::MenuItem("Load Listing")) { LoadListingDialog(); }
				if (ImGui::MenuItem("Load Sym File")) { LoadSymbolsDialog(); }
				if (ImGui::MenuItem("Load Vice Command Symbols")) { LoadViceCmdDialog(); }

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
				if (ImGui::BeginMenu("Graphics")) {
					for (int i = 0; i < kMaxGfxViews; ++i) {
						strown<64> name("Graphics");
						name.append_num(i + 1, 1, 10);
						if (ImGui::MenuItem(name.c_str(), NULL, gfxView[i].open)) { gfxView[i].open = !gfxView[i].open; }
					}
					ImGui::EndMenu();
				}
				if (ImGui::MenuItem("Registers", NULL, regView.open)) { regView.open = !regView.open; }
				if (ImGui::MenuItem("Breakpoints", NULL, breakView.open)) { breakView.open = !breakView.open; }
				if (ImGui::MenuItem("Symbols", NULL, symbolView.open)) { symbolView.open = !symbolView.open; }
				if (ImGui::MenuItem("Sections", NULL, sectionView.open)) { sectionView.open = !sectionView.open; }
				if (ImGui::MenuItem("Toolbar", NULL, toolBar.open)) { toolBar.open = !toolBar.open; }
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Style")) {
				if (ImGui::MenuItem("MonstersGoBoom C64")) { StyleC64(); }
				if (ImGui::MenuItem("Dark")) { ImGui::StyleColorsDark(); }
				if (ImGui::MenuItem("Light")) { ImGui::StyleColorsLight(); }
				if (ImGui::MenuItem("Classic")) { ImGui::StyleColorsClassic(); }
				if (ImGui::MenuItem("High Noon C64")) { StyleC64_Darker(); }
				if (ImGui::MenuItem("Regular C64")) { StyleC64_Mid(); }
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

			if (ImGui::BeginMenu("Layout")) {
				if (ImGui::MenuItem("Reset Layout")) { setupDocking = true; }
				if (ImGui::MenuItem("Save Layout")) { UserSaveLayout(); }
				if (ImGui::MenuItem("Save Layout on Exit", nullptr, &saveSettingsOnExit)) { saveSettingsOnExit = !saveSettingsOnExit; }
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();

		}
	}

	if (setupDocking) {
		setupDocking = false;
		ResetWindowLayout();
		toolBar.open = true;
		regView.open = true;
		memView[0].open = true;
		for (size_t i = 1; i < MaxMemViews; ++i) { memView[i].open = false; }
		codeView[0].open = true;
		for (size_t i = 1; i < MaxCodeViews; ++i) { codeView[i].open = false; }
		watchView[0].open = true;
		for (size_t i = 1; i < MaxWatchViews; ++i) { watchView[i].open = false; }
		gfxView[0].open = true;
		for (size_t i = 0; i < kMaxGfxViews; ++i) { gfxView[i].open = false; }
		breakView.open = true;
		symbolView.open = true;
		sectionView.open = true;
		console.open = true;
		screenView.open = true;
	}

	toolBar.Draw();
	regView.Draw();
	for (int m = 0; m < MaxMemViews; ++m) { memView[m].Draw(m); }
	for (int c = 0; c < MaxCodeViews; ++c) { codeView[c].Draw(c); }
	for (int w = 0; w < MaxWatchViews; ++w) { watchView[w].Draw(w); }
	for (int g = 0; g < kMaxGfxViews; ++g) { gfxView[g].Draw(g); }
	console.Draw();
	screenView.Draw();
	breakView.Draw();
	symbolView.Draw();
	sectionView.Draw();
	preView.Draw();

	fileView.Draw("Select File");
	GlobalKeyCheck();
	ViceTickMessage();

	if (GetCurrCPU()->MemoryChange() && memoryWasChanged) {
		GetCurrCPU()->WemoryChangeRefreshed();
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

void SetCodeViewAddr(uint16_t addr, int index)
{
	if (viewContext) {
		if (index < 0) {
			for (size_t i = 0; i < ViewContext::MaxCodeViews; ++i) {
				if (viewContext->codeView[i].open) {
					viewContext->codeView[i].SetAddr(addr);
					break;
				}
			}
		} else if (index < ViewContext::MaxCodeViews) {
			viewContext->codeView[index].SetAddr(addr);
		}
	}
}

void SetMemoryViewAddr(uint16_t addr, int index)
{
	if (viewContext) {
		if (index < 0) {
			for (size_t i = 0; i < ViewContext::MaxMemViews; ++i) {
				if (viewContext->memView[i].open) {
					viewContext->memView[i].SetAddr(addr);
					break;
				}
			}
		} else if (index < ViewContext::MaxCodeViews) {
			viewContext->memView[index].SetAddr(addr);
		}
	}
}

uint8_t InputHex()
{
	for (int num = 0; num < 10; ++num) { if (ImGui::IsKeyPressed(num + '0')) return num; }
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

void StateLoadViews(strref conf)
{
	if (viewContext) {
		viewContext->LoadState(conf);
	}
}
void StateSaveViews(UserData& conf)
{
	if (viewContext) {
		viewContext->SaveState(conf);
	}
}

void ImGuiStateLoaded()
{
	if (viewContext) {
		viewContext->setupDocking = false;
	}
}

bool SaveLayoutOnExit()
{
	if (viewContext) {
		return viewContext->saveSettingsOnExit;
	}
	return false;
}

void ReviewListing()
{
	if (viewContext && GetListingFile()) {
		viewContext->preView.ShowListing(GetListingFile());
	}
}