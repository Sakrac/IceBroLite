// WitchMap by Carl-Henrik Sk�rstedt, Space Moguls 2020
//  uses STB image loader by Sean Barrett and others, see stb/stb_image.h
//  uses ImGui by Omar Cornut, see ImGui/imgui.h

// dear imgui: standalone example application for GLFW + OpenGL2, using legacy fixed pipeline
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the example_glfw_opengl2/ folder**
// See imgui_impl_glfw.cpp for details.

#ifdef _WIN32
#include "framework.h"
#include <shellapi.h>
#else
#define APIENTRY
#endif

// C RunTime Header Files
#include <stdlib.h>
#include <memory.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include <stdio.h>
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <stdint.h>
#include <GLFW/glfw3.h>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>
#include "struse/struse.h"
#include "Config.h"
#include "Files.h"
#include "FileDialog.h"
#include "Breakpoints.h"
#include "Traces.h"
#include "Sym.h"
#include "StartVice.h"
#include "SaveState.h"
#include "views/FilesView.h"

#include "C64Colors.h"
#include "Image.h"
#include "6510.h"
#include "SourceDebug.h"
#include "CodeColoring.h"
#include "views/Views.h"

#include "WindowIcon.inc"

void StyleC64();
static GLFWwindow* sWindow;
static int sWindow_width = 1700, sWindow_height = 900;

void SaveStateWindow(UserData& conf) {
	if (sWindow) {
		conf.BeginStruct("Window");

		conf.AddValue("width", sWindow_width);
		conf.AddValue("height", sWindow_height);

		conf.AddValue("maximized", glfwGetWindowAttrib(sWindow, GLFW_MAXIMIZED));
		conf.EndStruct();
	}
}

struct WindowPreset { int w, h, m; };

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
							ret.w = (int)win_value.atoi(); if (ret.w < 320) { ret.w = 320; }
							sWindow_width = ret.w;
						} else if (win_name.same_str("height")) {
							ret.h = (int)win_value.atoi(); if (ret.h < 200) { ret.h = 200; }
							sWindow_height = ret.h;
						} else if (win_name.same_str("maximized")) {
							ret.m = (int)win_value.atoi()!=0;
						}
					}
				}
				break;
			}
		}
	}
	return ret;
}

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

void window_size_callback(GLFWwindow* window, int width, int height)
{
	if (!glfwGetWindowAttrib(window, GLFW_MAXIMIZED)) {
		sWindow_width = width;
		sWindow_height = height;
	}
}

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

#define MAX_LOADSTRING _MAX_PATH

static char forceLoadProgram[PATH_MAX_LEN];
static char forceLoadSymbols[PATH_MAX_LEN];
static char forceLoadExtraDebug[PATH_MAX_LEN];

// Global Variables:
#ifdef _WIN32
HWND hWnd;
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

HWND GetHWnd() { return hWnd; }

int __stdcall wWinMain(_In_ HINSTANCE hInstance,
					  _In_opt_ HINSTANCE hPrevInstance,
					  _In_ LPWSTR    lpCmdLine,
					  _In_ int       nCmdShow)
#else
int main(int argc, char* argv[])
#endif
{
#ifdef _WIN32
	int argc;
	LPWSTR* szArglist = CommandLineToArgvW(lpCmdLine, &argc);
	char** argv = nullptr;

	argc += 1;
	argv = (char**)calloc(argc+1, sizeof(char*));
	for (int i = 1; i < argc; ++i) {
		size_t len = wcslen(szArglist[i-1]);
		size_t count;
		argv[i] = (char*)LocalAlloc(LMEM_FIXED, len+1);
		wcstombs_s(&count, argv[i], len+1, szArglist[i-1], len);
	}
#endif

	for (int i = 1; i < argc; ++i) {
		strref line = argv[i];
		if (line[0] == '-') {
			++line;
			if(line.same_str("?") || line.same_str("help") || line.same_str("info")) {
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
					, "IceBroLite Info", 0
#endif
				);
				return 0;
			}
		}
	}

	GetStartFolder();

	SaveStateFile state = ReadState();

	// Setup window
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	WindowPreset winset = ReadStateWindow(state);
	if(winset.m) { glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE); }

	GLFWwindow* window = glfwCreateWindow(winset.w, winset.h, "IceBro Lite", NULL, NULL);
	if (window == NULL)
		return 1;
	glfwMakeContextCurrent(window);
	glfwSetWindowSizeCallback(window, window_size_callback);
	sWindow = window;
	GLFWimage image;
	image.height = 42;
	image.width = 42;
	image.pixels = (unsigned char*)sIcon_Pixels;

	glfwSetWindowIcon(window, 1, &image);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

#ifdef _WIN32
	hWnd = glfwGetWin32Window(window);
#endif

	LoadIcons();
	InitStartFolder();
	CreateMainCPU();
	InitSymbols();
	InitBreakpoints();
	InitTraces();

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	io.IniFilename = nullptr; // prevent imgui from auto saving

	StyleC64();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL2_Init();

	io.Fonts->AddFontDefault();
	InitViews();
	InitSourceDebug();
	ParseState(state); ReleaseState(state);

	for (int i = 1; i < argc; ++i) {
		strref line = argv[i];
		if (line[0] == '-') {
			++line;
			strref cmd = line.split_token_trim('=');
			if (line.get_first() == '"') { line.skip(1); }
			if (line.get_last() == '"') { line.clip(1); }
			if (cmd.same_str("load")) { strovl ovl(forceLoadProgram, sizeof(forceLoadProgram)); ovl.copy(line); ovl.c_str(); }
			else if (cmd.same_str("symbols")) { strovl ovl(forceLoadSymbols, sizeof(forceLoadSymbols)); ovl.copy(line); ovl.c_str(); }
			else if (cmd.same_str("connect")) { ViceConnect("127.0.0.1", 6502); }
			else if (cmd.same_str("extradebug")) { strovl ovl(forceLoadExtraDebug, sizeof(forceLoadExtraDebug)); ovl.copy(line); ovl.c_str(); }
			else if (cmd.same_str("start")) { if (line) { SetViceEXEPath(line); } LoadViceEXE(); }
			else if (cmd.same_str("font")) {
				if (line) {
					strref file = line.split_token(',');
					ForceUserFont(file, line.atoi() ? (int)line.atoi() : 13);
				}
			} else if (cmd.same_str("emu")) {
				if(line.same_str("c64")) {
					ViceSetEmuType(VICEEmuType::C64);
				} else if(line.same_str("vic20")) {
					ViceSetEmuType(VICEEmuType::Vic20);
				} else if(line.same_str("plus4")) {
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



#ifdef _WIN32
	// release win32 command line
	for (int i = 1; i < argc; ++i) {
		LocalFree(argv[i]);
	}
	if (argv) { free(argv); }
#endif


	// Our state
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

	// first frame stuff
	bool firstFrame = true;

	// Main loop
	while (!glfwWindowShouldClose(window)) {
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL2_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

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


		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);

		// If you are using this code with non-legacy OpenGL header/contexts (which you should not, prefer using imgui_impl_opengl3.cpp!!),
		// you may need to backup/reset/restore current shader using the commented lines below.
		//GLint last_program;
		//glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
		//glUseProgram(0);
		ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
		//glUseProgram(last_program);

		// Update and Render additional Platform Windows
		// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
		//  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}

		glfwSwapBuffers(window);

		if (forceLoadProgram[0] != 0 && ViceConnected()) {
			ViceStartProgram(forceLoadProgram);
			if (forceLoadSymbols[0] != 0) {
				ReadSymbolsFile(forceLoadSymbols);
			} else {
				ReadSymbolsForBinary(forceLoadProgram);
				if(forceLoadExtraDebug[0]) {
					ReadC64DbgSrcExtra(forceLoadExtraDebug);
				}
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

	if (SaveLayoutOnExit()) {
		SaveState();
	}

	ShutdownTraces();
	ShutdownBreakpoints();
	ShutdownSourceDebug();
	ShutdownSymbols();
	ShutdownMainCPU();
	// Cleanup
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	sWindow = nullptr;
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

