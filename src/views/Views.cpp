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
#include "TraceView.h"
#include "../6510.h"
#include "../Config.h"
#include "../data/C64_Pro_Mono-STYLE.ttf.h"
#include "../FileDialog.h"
#include "../SourceDebug.h"
#include "../StartVice.h"
#include "Views.h"
#include "GLFW/glfw3.h"
#include "../Image.h"
#include "../CodeColoring.h"

struct ViewContext {
	enum { sNumFontSizes = 7 };
	enum { MaxMemViews = 8, MaxCodeViews = 4, MaxWatchViews = 4, kMaxGfxViews = 2 };
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
	TraceView traceView;

	ImFont* aFonts[sNumFontSizes];

	int currFont, nextFont;
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
void StyleC64_Green();

static int imgui_style = 0;
static ViewContext* viewContext = nullptr;
static strown<PATH_MAX_LEN> sUserFontName;
static int sUserFontSize = 0;
static bool sForceFont = false;
static uint8_t sCodePCHighlight = 1;
static uint8_t sCodePCColor = 13;
static ImFont* sUserFont = nullptr;
static float sFontSizes[ViewContext::sNumFontSizes] = { 8.0f, 10.0f, 12.0, 14.0f, 16.0f, 20.0f, 24.0f };
static const ImWchar C64CharRanges[] =
{
	0x0020, 0x00FF, // Basic Latin + Latin Supplement
	0xee00, 0xefff,
	0
	//	0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
	//	0x2DE0, 0x2DFF, // Cyrillic Extended-A
	//	0xA640, 0xA69F, // Cyrillic Extended-B
	//	0,
};
static const ImWchar UserCharRanges[] =
{
	0x0020, 0x00FF, // Basic Latin + Latin Supplement
	0
};

void ResetWindowLayout()
{
	ImGuiID dockspace_id = ImGui::GetID("IceBroLiteDockSpace");

	ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
	ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace); // Add empty node
	ImGui::DockBuilderSetNodeSize(dockspace_id, ImVec2(ImGui::GetWindowWidth(), ImGui::GetWindowHeight()));

	ImGuiID dock_main_id = dockspace_id; // This variable will track the document node, however we are not using it here as we aren't docking anything into it.


	ImGuiID dock_id_toolbar = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Up, 0.10f, NULL, &dock_main_id);
	ImGuiID dock_id_memory = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.667f, NULL, &dock_main_id);
	ImGuiID dock_id_screen = ImGui::DockBuilderSplitNode(dock_id_memory, ImGuiDir_Right, 0.5f, NULL, &dock_id_memory);

	ImGuiID dock_id_breaks = ImGui::DockBuilderSplitNode(dock_id_screen, ImGuiDir_Down, 0.5f, NULL, &dock_id_screen);
	ImGuiID dock_id_vicemon = ImGui::DockBuilderSplitNode(dock_id_breaks, ImGuiDir_Down, 0.75f, NULL, &dock_id_breaks);
	ImGuiID dock_id_graphics = ImGui::DockBuilderSplitNode(dock_id_screen, ImGuiDir_Down, 0.5f, NULL, &dock_id_screen);

	ImGuiID dock_id_regs = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Up, 0.1f, NULL, &dock_main_id);
	ImGuiID dock_id_symbols = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, NULL, &dock_main_id);

	ImGuiID dock_id_watch = ImGui::DockBuilderSplitNode(dock_id_memory, ImGuiDir_Up, 0.33f, NULL, &dock_id_memory);
	ImGuiID dock_id_watch2 = ImGui::DockBuilderSplitNode(dock_id_watch, ImGuiDir_Down, 0.5f, NULL, &dock_id_watch);

	ImGui::DockBuilderDockWindow("###Code1", dock_main_id);
	ImGui::DockBuilderDockWindow("Toolbar", dock_id_toolbar);
	ImGui::DockBuilderDockWindow("Vice Monitor", dock_id_vicemon);
	ImGui::DockBuilderDockWindow("Mem1", dock_id_memory);
	ImGui::DockBuilderDockWindow("###Code2", dock_main_id);
	ImGui::DockBuilderDockWindow("###Code3", dock_main_id);
	ImGui::DockBuilderDockWindow("###Code4", dock_main_id);
	ImGui::DockBuilderDockWindow("Graphics1", dock_id_graphics);
	ImGui::DockBuilderDockWindow("Watch1", dock_id_watch);
	ImGui::DockBuilderDockWindow("Watch2", dock_id_watch2);
	ImGui::DockBuilderDockWindow("Registers", dock_id_regs);
	ImGui::DockBuilderDockWindow("Screen", dock_id_screen);
	ImGui::DockBuilderDockWindow("Trace", dock_id_screen);
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
	nextFont = currFont;
	currFontSize = sFontSizes[currFont];
	memView[0].open = true;
	codeView[0].open = true;
	watchView[0].open = true;
	console.open = true;
	memoryWasChanged = false;
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
	conf.BeginStruct("Trace"); traceView.WriteConfig(conf); conf.EndStruct();
	conf.AddValue("Style", imgui_style);
	conf.AddValue("FontSize", currFont);
	if (sUserFont && sUserFontSize && sUserFontName.valid()) {
		conf.AddValue("UserFont", sUserFontName.get_strref());
		conf.AddValue("UserFontSize", sUserFontSize);
	}
	conf.AddValue("CodePCHighlight", sCodePCHighlight);
	conf.AddValue("CodePCHighlightColor", sCodePCColor);
}

void ViewContext::LoadState(strref config)
{
	ConfigParse conf(config);
	while (!conf.Empty()) {
		strref name, value;
		ConfigParseType type = conf.Next(&name, &value);
		if (type == ConfigParseType::CPT_Value) {
			if (name.same_str("FontSize")) {
				int fontSize = (int)value.atoi();
				SelectFont(fontSize);
			} else if (name.same_str("UserFont") && !sForceFont) {
				sUserFontName.copy(value);
			} else if (name.same_str("UserFontSize") && !sForceFont) {
				sUserFontSize = (int)value.atoi();
			} else if(name.same_str("Style")) {
				imgui_style = (int)value.atoi();
				switch (imgui_style) {
					case 0: break;// default
					case 1: ImGui::StyleColorsDark(); ResetCodeColoring(); break;
					case 2: ImGui::StyleColorsLight(); ResetCodeColoring(); break;
					case 3: ImGui::StyleColorsClassic(); ResetCodeColoring(); break;
					case 4: StyleC64_Darker(); break;
					case 5: StyleC64_Mid(); break;
					case 6: StyleC64_Green(); break;
					default: imgui_style = 0; break;
				}
				ResetCodeColoring();
			} else if(name.same_str("CodePCHighlight")) {
				sCodePCHighlight = (uint8_t)value.atoi();
			} else if(name.same_str("CodePCHighlightColor")) {
				sCodePCColor = (uint8_t)value.atoi() & 0xf;
			}
		}
		if (type == ConfigParseType::CPT_Struct) {
			if (name.same_str("ToolBar")) { toolBar.ReadConfig(value); }
			else if (name.same_str("Registers")) { regView.ReadConfig(value); }
			else if (name.same_str("Breakpoints")) { breakView.ReadConfig(value); }
			else if (name.same_str("Symbols")) { symbolView.ReadConfig(value); }
			else if (name.same_str("Sections")) { sectionView.ReadConfig(value); }
			else if (name.same_str("Console")) { console.ReadConfig(value); }
			else if (name.same_str("Screen")) { screenView.ReadConfig(value); }
			else if (name.same_str("Trace")) { traceView.ReadConfig(value); }
		} else if (type == ConfigParseType::CPT_Array) {
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


void SendViceMonitorLine(const char* message, int size);
static const char DetachCartridgeCommand[] = "detach 20\n";

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
				if (ImGui::MenuItem("Load VICE")) { SetViceEXEPathDialog(); }
				if (GetViceEXEPath()) {
					if (ImGui::MenuItem("Reload VICE")) { LoadViceEXE(); }
				}
				if (ImGui::MenuItem("Read .prg to RAM")) { ReadPRGDialog(); }
				if (ImGui::MenuItem("Reread .prg to RAM")) { GetCurrCPU()->ReadPRGToRAM(ReadPRGFile()); }
				if (ImGui::MenuItem("Detach Cartridge")) { SendViceMonitorLine(DetachCartridgeCommand, sizeof(DetachCartridgeCommand)); }

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
				if (ImGui::MenuItem("Screen", NULL, screenView.open)) { screenView.open = !screenView.open; }
				if (ImGui::MenuItem("Trace", NULL, traceView.open)) { traceView.open = !traceView.open; }
				if (ImGui::MenuItem("Symbols", NULL, symbolView.open)) { symbolView.open = !symbolView.open; }
				if (ImGui::MenuItem("Sections", NULL, sectionView.open)) { sectionView.open = !sectionView.open; }
				if (ImGui::MenuItem("Toolbar", NULL, toolBar.open)) { toolBar.open = !toolBar.open; }
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Style")) {
				if (ImGui::MenuItem("MonstersGoBoom C64")) { StyleC64(); imgui_style = 0; }
				if (ImGui::MenuItem("Dark")) { ImGui::StyleColorsDark(); ResetCodeColoring(); imgui_style = 1; }
				if (ImGui::MenuItem("Light")) { ImGui::StyleColorsLight(); ResetCodeColoring(); imgui_style = 2; }
				if (ImGui::MenuItem("Classic")) { ImGui::StyleColorsClassic(); ResetCodeColoring(); imgui_style = 3; }
				if (ImGui::MenuItem("High Noon C64")) { StyleC64_Darker(); imgui_style = 4; }
				if (ImGui::MenuItem("Regular C64")) { StyleC64_Mid(); imgui_style = 5; }
				if (ImGui::MenuItem("Matrix C64")) { StyleC64_Green(); imgui_style = 6; }
				if (HasCustomTheme()) {
					if (ImGui::MenuItem("Custom")) { SetCustomTheme(); imgui_style = 6; }
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Font")) {
				for (int i = 0; i < sNumFontSizes; ++i) {
					strown<16> sz("Font Size ");
					sz.append_num(i, 0, 10);
					if (ImGui::MenuItem(sz.c_str())) { SelectFont(i); }
				}
				if (ImGui::MenuItem("ImGui default")) { UseDefaultFont(); }
				if (sUserFont) {
					if (ImGui::MenuItem("Custom Font")) { UseCustomFont(); }
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Layout")) {
				if (ImGui::MenuItem("Reset Layout")) { setupDocking = true; }
				if (ImGui::MenuItem("Save Layout")) { UserSaveLayout(); }
				if (ImGui::MenuItem("Save Layout on Exit", nullptr, &saveSettingsOnExit)) { saveSettingsOnExit = !saveSettingsOnExit; }
				if (ImGui::BeginMenu("Code PC Highlight")) {
					if (ImGui::MenuItem("None", nullptr, sCodePCHighlight == 0)) { sCodePCHighlight = 0; }
					if (ImGui::MenuItem("Outline", nullptr, sCodePCHighlight == 1)) { sCodePCHighlight = 1; }
					if (ImGui::MenuItem("Highlight", nullptr, sCodePCHighlight == 2)) { sCodePCHighlight = 2; }
					if (ImGui::BeginMenu("Color")) {
						sCodePCColor = DrawPaletteMenu(sCodePCColor);
						ImGui::EndMenu();
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}

			ThemeColorMenu();

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
		for (size_t i = 1; i < kMaxGfxViews; ++i) { gfxView[i].open = false; }
		breakView.open = true;
		symbolView.open = true;
		sectionView.open = true;
		console.open = true;
		screenView.open = true;
		traceView.open = false;
	}

	if (const char* prg = ReadPRGToRAMReady()) { GetCurrCPU()->ReadPRGToRAM(prg); }

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
	traceView.Draw();

	fileView.Draw("Select File");
	GlobalKeyCheck();
	ViceTickMessage();

	if (GetCurrCPU()->MemoryChange() && memoryWasChanged) {
		GetCurrCPU()->WemoryChangeRefreshed();
	}

	if (nextFont != currFont) {
		SelectFont(nextFont);
	}

}

void ViewContext::GlobalKeyCheck()
{
//	CheckRegChange();

	bool shift = ImGui::IsKeyDown((ImGuiKey)GLFW_KEY_LEFT_SHIFT) || ImGui::IsKeyDown((ImGuiKey)GLFW_KEY_RIGHT_SHIFT);
	bool ctrl = ImGui::IsKeyDown((ImGuiKey)GLFW_KEY_LEFT_CONTROL) || ImGui::IsKeyDown((ImGuiKey)GLFW_KEY_RIGHT_CONTROL);
	bool alt = ImGui::IsKeyDown((ImGuiKey)GLFW_KEY_LEFT_ALT) || ImGui::IsKeyDown((ImGuiKey)GLFW_KEY_RIGHT_ALT);

	// mimic vice monitor key combo
	if (alt && ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_H)) {
		ViceBreak();
	}

	if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_F5, false)) {
		if (shift) { ViceBreak(); }
		else { ViceGo(); }
//		if (ctrl) {} else if (shift) { CPUReverse(); } else { CPUGo(); }
	}
	if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_F10, false)) {
		ViceStepOver();
//		if (ctrl) { StepOverVice(); } else if (shift) { StepOverBack(); } else { StepOver(); }
	}
	if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_F11, false)) {
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
	for (int num = 0; num < 10; ++num) { if (ImGui::IsKeyPressed((ImGuiKey)(num + '0'))) return num; }
	for (int num = 10; num < 16; ++num) { if (ImGui::IsKeyPressed((ImGuiKey)(num + 'A' - 10))) return num; }
	return 0xff;
}

void SelectFont(int size)
{
	// check if another font change is already pending
	if (size != viewContext->nextFont && viewContext->nextFont != viewContext->currFont) { return; }

	// attempt to set this font
	if (viewContext && size >= 0 && size < ViewContext::sNumFontSizes) {
		if (viewContext->aFonts[size]->IsLoaded()) {
			viewContext->currFont = size;
			viewContext->currFontSize = sFontSizes[size];
			ImGui::SetCurrentFont(viewContext->aFonts[viewContext->currFont]);
			GImGui->IO.FontDefault = viewContext->aFonts[viewContext->currFont];
		}
		viewContext->nextFont = size;
	}
}

void ViewPushFont(int size) {
	if (size < 0 || size >= ViewContext::sNumFontSizes) { size = 1; }
	ImGui::PushFont(viewContext->aFonts[size]);
}

void ViewPopFont() {
	ImGui::PopFont();
}

int PetsciiFont() {
	if (viewContext->currFont < ViewContext::sNumFontSizes) {
		return -1; // already a petscii font
	}

	if (viewContext->currFont == (ViewContext::sNumFontSizes + 1) && sUserFontSize) {
		int best = 1;
		int diff = 1000;
		for (int s = 0; s < (int)(sizeof(sFontSizes) / sizeof(sFontSizes[0])); ++s) {
			int d = abs((int)sFontSizes[s] - sUserFontSize);
			if (d < diff) { best = s; diff = d; }
		}
		return best;
	}
	return 1;
}


void UseDefaultFont()
{
	if (viewContext) {
		viewContext->nextFont = viewContext->currFont = ViewContext::sNumFontSizes;
		viewContext->currFontSize = 7;
		GImGui->IO.FontDefault = nullptr;
	}
}

bool UseCustomFont() {
	if (viewContext && sUserFont) {
		viewContext->currFontSize = (float)sUserFontSize;
		GImGui->IO.FontDefault = sUserFont;
		viewContext->nextFont = viewContext->currFont = ViewContext::sNumFontSizes + 1;
		return true;
	}
	return false;
}

bool LoadUserFont(const char* file, int size) {
	sUserFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(
		file, (float)size, NULL, UserCharRanges);
	if (sUserFont) {
		sUserFontName.copy(file);
		sUserFontSize = size;
		return true;
	}
	return false;
}

void SetInitialFont() {
	if (viewContext) {
		if (viewContext->currFont == (ViewContext::sNumFontSizes + 1) && sUserFont) {
			UseCustomFont();
		} else if (viewContext->currFont == ViewContext::sNumFontSizes) {
			UseDefaultFont();
		} else {
			SelectFont(viewContext->currFont < ViewContext::sNumFontSizes ?
				viewContext->currFont : 3);
		}
	}
}

void ForceUserFont(strref file, int size) {
	sUserFontName.copy(file);
	sUserFontSize = size;
	sForceFont = true;
}

void CheckUserFont() {
	if (!sUserFont && sUserFontName.valid()) {
		sUserFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(
			sUserFontName.c_str(), (float)sUserFontSize, NULL, UserCharRanges);
	}
	if (sUserFont) {
		if(sUserFontSize == (ViewContext::sNumFontSizes+1) || sForceFont)
			UseCustomFont();
	} else {
		sUserFontName.clear();
		sUserFontSize = 0;
	}
	sForceFont = false;
}



void RefreshScreen(uint8_t* img, uint16_t w, uint16_t h,
	uint16_t sx, uint16_t sy, uint16_t sw, uint16_t sh)
{
	if (viewContext) {
		viewContext->screenView.Refresh(img, w, h, sx, sy, sw, sh);
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

void AddWatch(int watch, const char* expr) {
	if (viewContext && watch < ViewContext::MaxWatchViews) {
		viewContext->watchView[watch].AddWatch(expr);
		viewContext->watchView[watch].open = true;
	}
}

void SetCodeAddr(int code, uint16_t addr) {
	if (viewContext && code < ViewContext::MaxCodeViews) {
		viewContext->codeView[code].addrValue = addr;
		viewContext->codeView[code].addrCursor = addr;
		viewContext->codeView[code].lastShownPC = addr;
		viewContext->codeView[code].open = true;
		viewContext->codeView[code].showPCAddress = false;
		strovl addrOvl(viewContext->codeView[code].address, sizeof(viewContext->codeView[code].address));
		addrOvl.append('$').append_num(addr, 4, 16).c_str();
	}
}


static const char* sColorName[] = {
	"Black", "White", "Red", "Cyan", "Purple", "Green",
	"Blue", "Yellow", "Orange", "Brown", "Pink", "Dark Grey",
	"Mid Grey", "Light Green", "Light Blue", "Light Grey"
};

uint8_t DrawPaletteMenu(uint8_t col) {
	for (uint8_t c = 0; c < 16; ++c) {
		strown<16> name;
		name.append_num(c, 0, 10).append(' ').append(sColorName[c]);
		if (ImGui::MenuItem(name.c_str(), nullptr, col == c)) { col = c; }
	}
	return col;
}

int GetPCHighlightStyle() { return sCodePCHighlight; }
uint32_t GetPCHighlightColor() { return c64pal[sCodePCColor]; }
