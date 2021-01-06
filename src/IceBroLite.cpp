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
#endif
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
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
#include "Files.h"
#include "FileDialog.h"
#include "views/FilesView.h"

#include "C64Colors.h"
#include "Image.h"
#include "6510.h"
#include "views/Views.h"

#include "WindowIcon.inc"

void StyleC64();
void CheckMissingConfig();

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

#define MAX_LOADSTRING _MAX_PATH

// Global Variables:
#ifdef _WIN32
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
					  _In_opt_ HINSTANCE hPrevInstance,
					  _In_ LPWSTR    lpCmdLine,
					  _In_ int       nCmdShow)
#else
int main(int argc, char* argv[])
#endif
{
	GetStartFolder();

	// check if either imgui.ini or icebro.cfg is missing
	CheckMissingConfig();

	// Setup window
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;
	GLFWwindow* window = glfwCreateWindow(1280, 720, "IceBro Lite", NULL, NULL);
	if (window == NULL)
		return 1;
	glfwMakeContextCurrent(window);
	GLFWimage image;
	image.height = 32;
	image.width = 32;
	image.pixels = (unsigned char*)sIcon;

	glfwSetWindowIcon(window, 1, &image);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	LoadIcons();
	InitStartFolder();
	CreateMainCPU();

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoAutoMerge = true;
	//io.ConfigViewportsNoTaskBarIcon = true;

	// Setup Dear ImGui style
	//ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();
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

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);

	InitViews();

	// Our state
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

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
		ImGui::SetNextWindowPos(viewport->GetWorkPos());
		ImGui::SetNextWindowSize(viewport->GetWorkSize());
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		static bool mainWindowOpen = true;
		ImGui::Begin("WitchMap Main", &mainWindowOpen, window_flags);
		ImGui::PopStyleVar(2);

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
	}

	ShutdownMainCPU();
	// Cleanup
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}


static const char sIceBro_cfg[] = {
	"Views{\n"
	"  fontSize = 0\n"
	"  width = 1280\n"
	"  height = 720\n"
	"}\n"
	"ViceMonitor{\n"
	"  open = On\n"
	"}\n"
	"MemView[\n"
	"{\n"
	"  open = On\n"
	"}\n"
	"{\n"
	"  open = Off\n"
	"}\n"
	"{\n"
	"  open = Off\n"
	"}\n"
	"{\n"
	"  open = Off\n"
	"}\n"
	"]\n"
	"CodeView[\n"
	"{\n"
	"  open = On\n"
	"}\n"
	"{\n"
	"  open = Off\n"
	"}\n"
	"{\n"
	"  open = Off\n"
	"}\n"
	"{\n"
	"  open = Off\n"
	"}\n"
	"]\n"
	"RegisterView{\n"
	"  open = On\n"
	"}\n"
	"ScreenView[\n"
	"{\n"
	"  open = On\n"
	"  mode = 9\n"
	"  system = 1\n"
	"  c64Mode = 7\n"
	"}\n"
	"{\n"
	"  open = Off\n"
	"}\n"
	"{\n"
	"  open = Off\n"
	"}\n"
	"{\n"
	"  open = Off\n"
	"}\n"
	"]\n"
	"WatchView[\n"
	"{\n"
	"  open = On\n"
	"}\n"
	"{\n"
	"  open = Off\n"
	"}\n"
	"]\n"
	"Breakpoints{\n"
	"  open = On\n"
	"}\n"
	"Toolbar{\n"
	"  open = On\n"
	"}\n"
};

static const char sImGui_ini[] = {
	"[Window] [IceBro DockSpace]\n"
	"Size = 1280,720\n"
	"Collapsed = 0\n"
	"\n"
	"[Window][Debug##Default]\n"
	"Pos = 182,182\n"
	"Size = 400,400\n"
	"Collapsed = 0\n"
	"\n"
	"[Window][Toolbar]\n"
	"Pos = 0,10\n"
	"Size = 1280,81\n"
	"Collapsed = 0\n"
	"DockId = 0x00000001,0\n"
	"\n"
	"[Window][Vice Monitor]\n"
	"Pos = 634,171\n"
	"Size = 646,771\n"
	"Collapsed = 0\n"
	"DockId = 0x0000000C,0\n"
	"\n"
	"[Window][Mem1]\n"
	"Pos = 0,620\n"
	"Size = 632,322\n"
	"Collapsed = 0\n"
	"DockId = 0x0000000A,0\n"
	"\n"
	"[Window][###Code1]\n"
	"Pos = 0,93\n"
	"Size = 632,525\n"
	"Collapsed = 0\n"
	"DockId = 0x00000009,0\n"
	"\n"
	"[Window][Registers]\n"
	"Pos = 634,93\n"
	"Size = 646,76\n"
	"Collapsed = 0\n"
	"DockId = 0x0000000B,0\n"
	"\n"
	"[Window][Screen1]\n"
	"Pos = 0,75\n"
	"Size = 389,199\n"
	"Collapsed = 0\n"
	"DockId = 0x00000007,0\n"
	"\n"
	"[Window][Watch1]\n"
	"Pos = 0,276\n"
	"Size = 389,87\n"
	"Collapsed = 0\n"
	"DockId = 0x00000008,0\n"
	"\n"
	"[Window][Breakpoints]\n"
	"Pos = 782,606\n"
	"Size = 498,114\n"
	"Collapsed = 0\n"
	"DockId = 0x0000000E,0\n"
	"\n"
	"[Window][WitchMap Main]\n"
	"Pos = 0,0\n"
	"Size = 1280,942\n"
	"Collapsed = 0\n"
	"\n"
	"[Docking][Data]\n"
	"DockSpace       ID = 0x22EE0AEA Pos = 604,272 Size = 1280,710 Split = X Selected = 0xE59CF797\n"
	"  DockNode      ID = 0x00000003 Parent = 0x22EE0AEA SizeRef = 780,645 Split = Y Selected = 0x172AD7AD\n"
	"	DockNode    ID = 0x00000007 Parent = 0x00000003 SizeRef = 389,199 Selected = 0x6A67AAE6\n"
	"	DockNode    ID = 0x00000008 Parent = 0x00000003 SizeRef = 389,444 Selected = 0x87AB43E7\n"
	"  DockNode      ID = 0x00000004 Parent = 0x22EE0AEA SizeRef = 498,645 Split = Y Selected = 0xE59CF797\n"
	"	DockNode    ID = 0x0000000D Parent = 0x00000004 SizeRef = 498,488 CentralNode = 1 Selected = 0xE59CF797\n"
	"	DockNode    ID = 0x0000000E Parent = 0x00000004 SizeRef = 498,114 Selected = 0x0263173C\n"
	"DockSpace       ID = 0x6322A3BF Window = 0x36029362 Pos = 1014,299 Size = 1280,932 Split = Y Selected = 0x172AD7AD\n"
	"  DockNode      ID = 0x00000001 Parent = 0x6322A3BF SizeRef = 1280,62 Selected = 0x507852CA\n"
	"  DockNode      ID = 0x00000002 Parent = 0x6322A3BF SizeRef = 1280,646 Split = X Selected = 0x172AD7AD\n"
	"	DockNode    ID = 0x00000005 Parent = 0x00000002 SizeRef = 632,646 Split = Y Selected = 0x172AD7AD\n"
	"	  DockNode  ID = 0x00000009 Parent = 0x00000005 SizeRef = 780,322 CentralNode = 1 Selected = 0x172AD7AD\n"
	"	  DockNode  ID = 0x0000000A Parent = 0x00000005 SizeRef = 780,322 Selected = 0x87AB43E7\n"
	"	DockNode    ID = 0x00000006 Parent = 0x00000002 SizeRef = 646,646 Split = Y Selected = 0xE59CF797\n"
	"	  DockNode  ID = 0x0000000B Parent = 0x00000006 SizeRef = 498,58 Selected = 0x837A6095\n"
	"	  DockNode  ID = 0x0000000C Parent = 0x00000006 SizeRef = 498,586 Selected = 0xE59CF797\n"
};

// check if either imgui.ini or icebro.cfg is missing
void CheckMissingConfig()
{
	bool replace = false;
	FILE* f;
	if (fopen_s(&f, "imgui.ini", "rb") == 0 && f) {
		fclose(f);
	} else { replace = true; }
//	if (fopen_s(&f, "IceBro.cfg", "rb") == 0 && f) {
//		fclose(f);
//	} else { replace = true; }
	if (replace) {
		if (fopen_s(&f, "imgui.ini", "w") == 0 && f) {
			fwrite(sImGui_ini, sizeof(sImGui_ini), 1, f);
			fclose(f);
		}
//		if (fopen_s(&f, "IceBro.cfg", "w") == 0 && f) {
//			fwrite(sIceBro_cfg, sizeof(sIceBro_cfg), 1, f);
//			fclose(f);
//		}
	}
}

