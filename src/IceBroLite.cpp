// IceBroLite by Carl-Henrik Skårstedt, Space Moguls 2020
//  uses STB image loader by Sean Barrett and others, see stb/stb_image.h
//  uses ImGui by Omar Cornut, see ImGui/imgui.h
//  uses Sokol by Andre Weissflog, see sokol/sokol_app.h

//
// Compile Instructions
//	first import dependencies by running get_dependencies.bat / .sh
//	compile with either MSVC or CMake (Clang)
//

#include "imgui.h"
#include "struse/struse.h"
#include "Config.h"
#include "FileDialog.h"
#include "Breakpoints.h"
#include "Traces.h"
#include "Sym.h"
#include "StartVice.h"
#include "SaveState.h"
#include "Image.h"
#include "6510.h"
#include "SourceDebug.h"
#include "CodeColoring.h"
#include "views/Views.h"
#include "views/FilesView.h"
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/util/sokol_imgui.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"
#include "IceBroLite.h"

void StyleC64();
int sWindow_width = 1700, sWindow_height = 900;
static int sCmdLineArgN = 0;
static const char** sCmdLineArgs = nullptr;
static SaveStateFile state;
// first frame stuff
static bool firstFrame = true;
static sg_pass_action pass_action;

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#define MAX_LOADSTRING _MAX_PATH

static char forceLoadProgram[PATH_MAX_LEN];
static char forceLoadSymbols[PATH_MAX_LEN];
static char forceLoadExtraDebug[PATH_MAX_LEN];

// If extra debug is specified on the command line then reload extra debug info
// when "Reload" button is pressed as well
void CheckForceLoadExtraDebug() {
	if (forceLoadExtraDebug[0]) {
		ReadC64DbgSrcExtra(forceLoadExtraDebug);
	}
}

void StartupCommandLine() {
	const char** argv = sCmdLineArgs;
	const int argc = sCmdLineArgN;
	// Check if -? / -help or -info is on the command line, then stop and print command line info
	for (int i = 1; i < argc; ++i) {
		strref line = argv[i];
		if (line[0] == '-') {
			++line;
			if (line.same_str("?") || line.same_str("help") || line.same_str("info")) {
#ifdef WIN32
				MessageBoxA(0,
#else
				printf(
#endif
					"IceBroLite optional parameters:\n"
					"* -load=<file>: automatically load a prg/d64/crt etc. and associated debug info when connected to VICE\n"
					" * -symbols=<file>: autoload symbols from file\n"
					" * -connect: connect to an existing instance of VICE\n"
					" * -extradebug=<file>: attempt to load an additional debug XML file (kickasm) in addition to the program debug\n"
					" * -start: start the previously loaded VICE executable on startup\n"
					" * -start:<file>: start a VICE executable specified as the file\n"
					" * -emu=<type>: start a VICE emulator where type is one of: c64, vic20 or plus4\n"
#ifdef WIN32
					,
					"IceBroLite Info", 0
#endif
				);
			}
		}
	}
}

void SaveStateWindow(UserData& conf) {
	conf.BeginStruct("Window");

	conf.AddValue("width", sapp_width());
	conf.AddValue("height", sapp_height());

	conf.AddValue("maximized", sapp_is_fullscreen() ? 1 : 0);
	conf.EndStruct();
}

struct WindowPreset {
	int w, h, m;
};

WindowPreset ReadStateWindow(SaveStateFile file) {
	WindowPreset ret = { sWindow_width, sWindow_height, 0 };
	if (file.data && file.size) {
		ConfigParse config(file.data, file.size);
		while (!config.Empty()) {
			strref name, value;
			ConfigParseType type = config.Next(&name, &value);
			if (name.same_str("Window") && type == ConfigParseType::CPT_Struct) {
				ConfigParse win_config(value);
				while (!win_config.Empty()) {
					strref win_name, win_value;
					ConfigParseType win_type = win_config.Next(&win_name, &win_value);
					if (win_type == ConfigParseType::CPT_Value) {
						if (win_name.same_str("width")) {
							ret.w = (int)win_value.atoi();
							if (ret.w < 320) {
								ret.w = 320;
							}
							sWindow_width = ret.w;
						} else if (win_name.same_str("height")) {
							ret.h = (int)win_value.atoi();
							if (ret.h < 200) {
								ret.h = 200;
							}
							sWindow_height = ret.h;
						} else if (win_name.same_str("maximized")) {
							ret.m = (int)win_value.atoi() != 0;
						}
					}
				}
				break;
			}
		}
	}
	return ret;
}

void ReadCurrentState() {
	GetStartFolder();
	StartupCommandLine();
	state = ReadState();
	WindowPreset winset = ReadStateWindow(state);
	if ((bool)winset.m != sapp_is_fullscreen()) { sapp_toggle_fullscreen(); }
}

void IBLCommandLine(int argc, char* argv[]) {
	sCmdLineArgN = argc;
	sCmdLineArgs = (const char**)argv;
}

void IBLPreSokolSetup() {
	ReadCurrentState();
}

void IBLInit() {
	sg_desc sokol_desc = {};
	sokol_desc.environment = sglue_environment();
	sokol_desc.logger.func = slog_func;
	sg_setup(&sokol_desc);

	simgui_desc_t imgui_desc = {};
	imgui_desc.no_default_font = true;
	simgui_setup(&imgui_desc);

	// initial clear color
	pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
	pass_action.colors[0].clear_value = { 0.0f, 0.5f, 0.7f, 1.0f };

	LoadIcons();
	InitStartFolder();
	CreateMainCPU();
	InitSymbols();
	InitBreakpoints();
	InitTraces();

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;	  // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;	  // Enable Multi-Viewport / Platform Windows
	io.IniFilename = nullptr;							  // prevent imgui from auto saving

	StyleC64();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	io.Fonts->AddFontDefault();
	InitViews();
	InitSourceDebug();
	ParseState(state);
	ReleaseState(state);

	for (int i = 1; i < sCmdLineArgN; ++i) {
		strref line = sCmdLineArgs[i];
		if (line[0] == '-') {
			++line;
			strref cmd = line.split_token_trim('=');
			if (line.get_first() == '"') {
				line.skip(1);
			}
			if (line.get_last() == '"') {
				line.clip(1);
			}
			if (cmd.same_str("load")) {
				strovl ovl(forceLoadProgram, sizeof(forceLoadProgram));
				ovl.copy(line);
				ovl.c_str();
			} else if (cmd.same_str("symbols")) {
				strovl ovl(forceLoadSymbols, sizeof(forceLoadSymbols));
				ovl.copy(line);
				ovl.c_str();
			} else if (cmd.same_str("connect")) {
				ViceConnect("127.0.0.1", 6502);
			} else if (cmd.same_str("extradebug")) {
				strovl ovl(forceLoadExtraDebug, sizeof(forceLoadExtraDebug));
				ovl.copy(line);
				ovl.c_str();
			} else if (cmd.same_str("start")) {
				if (line) {
					SetViceEXEPath(line);
				}
				LoadViceEXE();
			} else if (cmd.same_str("font")) {
				if (line) {
					strref file = line.split_token(',');
					ForceUserFont(file, line.atoi() ? (int)line.atoi() : 13);
				}
			} else if (cmd.same_str("emu")) {
				if (line.same_str("c64")) {
					ViceSetEmuType(VICEEmuType::C64);
				} else if (line.same_str("vic20")) {
					ViceSetEmuType(VICEEmuType::Vic20);
				} else if (line.same_str("plus4")) {
					ViceSetEmuType(VICEEmuType::Plus4);
				}
			}
		}
	}

	CheckUserFont();
	CheckCustomThemeAfterStateLoad();

	// if only symbols provided just read those in immediately
	if (forceLoadProgram[0] == 0 && forceLoadSymbols[0] != 0) {
		ReadSymbolsFile(forceLoadSymbols);
		forceLoadSymbols[0] = 0;
	}

	// Our state
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
}

void IBLFrame() {
	simgui_frame_desc_t frame_desc = {};
	frame_desc.width = sapp_width();
	frame_desc.height = sapp_height();
	frame_desc.delta_time = sapp_frame_duration();
	frame_desc.dpi_scale = sapp_dpi_scale();
	simgui_new_frame(&frame_desc);

	SetViewFont();

	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
	// because it would be confusing to have two docking targets within each others.
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	static bool mainWindowOpen = true;
	ImGui::Begin("IceBroLite Main", &mainWindowOpen, window_flags);
	ImGui::PopStyleVar(2);

	if (firstFrame) {
		SetInitialFont();
	}

	ImGuiID dockspace_id = ImGui::GetID("IceBroLiteDockSpace");
	ImGui::DockSpace(dockspace_id);

	ShowViews();

	ImGui::End();

	// Update and Render additional Platform Windows
	// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
	//  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	EndViewFont();

	// the sokol_gfx draw pass
	sg_pass pass = {};
	pass.action = pass_action;
	pass.swapchain = sglue_swapchain();
	sg_begin_pass(&pass);
	simgui_render();
	sg_end_pass();
	sg_commit();

	if (forceLoadProgram[0] != 0 && ViceConnected()) {
		ViceStartProgram(forceLoadProgram);
		if (forceLoadSymbols[0] != 0) {
			ReadSymbolsFile(forceLoadSymbols);
		} else {
			ReadSymbolsForBinary(forceLoadProgram);
			CheckForceLoadExtraDebug();
		}
		forceLoadProgram[0] = 0;
		forceLoadSymbols[0] = 0;
	}

	if (const char* kickDbgFile = LoadKickDbgReady()) {
		ReadC64DbgSrc(kickDbgFile);
	}
	if (const char* kickDbgExtraFile = LoadKickDbgExtraReady()) {
		ReadC64DbgSrcExtra(kickDbgExtraFile);
	}
	if (const char* viceMonCmdFile = LoadViceCMDReady()) {
		ReadViceCommandFile(viceMonCmdFile);
	}
	if (const char* symFile = LoadSymbolsReady()) {
		ReadSymbols(symFile);
	}
	if (const char* listFile = LoadListingReady()) {
		if (ReadListingFile(listFile)) {
			ReviewListing();
		}
	}
	if (const char* themeFile = LoadThemeReady()) {
		LoadCustomTheme(themeFile);
	}
	if (const char* themeFile = SaveThemeReady()) {
		SaveCustomTheme(themeFile);
	}
	WaitForViceEXEPath();

	UserSaveLayoutUpdate();
	firstFrame = false;
}

void IBLCleanup() {
	if (SaveLayoutOnExit()) {
		SaveState();
	}

	ShutdownTraces();
	ShutdownBreakpoints();
	ShutdownSourceDebug();
	ShutdownSymbols();
	ShutdownMainCPU();
	ShutdownViews();
	ShutdownImage();
	// Cleanup
	simgui_shutdown();
}

bool CapturedKeyboard = false;

void IBLEvent(const sapp_event* sokol_event) {
	CapturedKeyboard = simgui_handle_event(sokol_event);
}
