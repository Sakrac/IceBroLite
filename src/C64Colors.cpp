#include "imgui/imgui.h"
#include "C64Colors.h"

void StyleC64()
{
	ImVec4* colors = ImGui::GetStyle().Colors;

	colors[ ImGuiCol_Text ] = C64_WHITE;
	colors[ ImGuiCol_TextDisabled ] = C64_BLACK;
	colors[ ImGuiCol_WindowBg ] = C64_LBLUE;
	colors[ ImGuiCol_ChildBg ] = C64_LBLUE;
	colors[ ImGuiCol_PopupBg ] = C64_GREEN;
	colors[ ImGuiCol_Border ] = C64_BLUE;
	colors[ ImGuiCol_BorderShadow ] = C64_BLACK;
	colors[ ImGuiCol_FrameBg ] = C64_BROWN;
	colors[ ImGuiCol_FrameBgHovered ] = C64_ORANGE;
	colors[ ImGuiCol_FrameBgActive ] = C64_PINK;
	colors[ ImGuiCol_TitleBg ] = C64_DGRAY;
	colors[ ImGuiCol_TitleBgActive ] = C64_MGRAY;
	colors[ ImGuiCol_TitleBgCollapsed ] = C64_BLACK;
	colors[ ImGuiCol_MenuBarBg ] = C64_BLUE;
	colors[ ImGuiCol_ScrollbarBg ] = C64_BLACK;
	colors[ ImGuiCol_ScrollbarGrab ] = C64_LGRAY;
	colors[ ImGuiCol_ScrollbarGrabHovered ] = C64_GREEN;
	colors[ ImGuiCol_ScrollbarGrabActive ] = C64_LGREEN;
	colors[ ImGuiCol_CheckMark ] = C64_YELLOW;
	colors[ ImGuiCol_SliderGrab ] = C64_LGRAY;
	colors[ ImGuiCol_SliderGrabActive ] = C64_LGREEN;
	colors[ ImGuiCol_Button ] = C64_BLUE;
	colors[ ImGuiCol_ButtonHovered ] = C64_CYAN;
	colors[ ImGuiCol_ButtonActive ] = C64_LBLUE;
	colors[ ImGuiCol_Header ] = C64_BLUE;
	colors[ ImGuiCol_HeaderHovered ] = C64_CYAN;
	colors[ ImGuiCol_HeaderActive ] = C64_LBLUE;
	colors[ ImGuiCol_Separator ] = C64_BLUE;
	colors[ ImGuiCol_SeparatorHovered ] = C64_BLUE;
	colors[ ImGuiCol_SeparatorActive ] = C64_BLUE;
	colors[ ImGuiCol_ResizeGrip ] = C64_WHITE;
	colors[ ImGuiCol_ResizeGripHovered ] = C64_LGREEN;
	colors[ ImGuiCol_ResizeGripActive ] = C64_GREEN;
	colors[ ImGuiCol_PlotLines ] = C64_LGREEN;
	colors[ ImGuiCol_PlotLinesHovered ] = C64_WHITE;
	colors[ ImGuiCol_PlotHistogram ] = C64_WHITE;
	colors[ ImGuiCol_PlotHistogramHovered ] = C64_WHITE;
	colors[ ImGuiCol_TextSelectedBg ] = C64_BLACK;
	colors[ ImGuiCol_DragDropTarget ] = C64_YELLOW;
	colors[ ImGuiCol_NavHighlight ] = C64_PURPLE;
	colors[ ImGuiCol_NavWindowingHighlight ] = C64_PINK;

	ImGuiStyle& style = ImGui::GetStyle();

	style.FramePadding = ImVec2( 8, 1 );
	style.FrameRounding = 0;

	style.WindowBorderSize = 0;
	style.WindowPadding = ImVec2( 0, 0 );

	style.PopupBorderSize = 0;
	style.PopupRounding = 0;

	style.WindowRounding = 0;
	style.ScrollbarRounding = 0;
	style.TabRounding = 0;

	style.ItemSpacing = ImVec2( 4, 2 );
	style.ItemInnerSpacing = ImVec2( 0, 0 );

	style.GrabMinSize = 8;
	style.GrabRounding = 0;
}

void StyleC64_Darker()
{
	ImVec4* colors = ImGui::GetStyle().Colors;

	colors[ImGuiCol_Text] = C64_LGRAY;
	colors[ImGuiCol_TextDisabled] = C64_ORANGE;
	colors[ImGuiCol_WindowBg] = C64_DGRAY;
	colors[ImGuiCol_ChildBg] = C64_BROWN;
	colors[ImGuiCol_PopupBg] = C64_BROWN;
	colors[ImGuiCol_Border] = C64_YELLOW;
	colors[ImGuiCol_BorderShadow] = C64_BLACK;
	colors[ImGuiCol_FrameBg] = C64_DGRAY;
	colors[ImGuiCol_FrameBgHovered] = C64_YELLOW;
	colors[ImGuiCol_FrameBgActive] = C64_PINK;
	colors[ImGuiCol_TitleBg] = C64_ORANGE;
	colors[ImGuiCol_TitleBgActive] = C64_PINK;
	colors[ImGuiCol_TitleBgCollapsed] = C64_BLACK;
	colors[ImGuiCol_MenuBarBg] = C64_BLUE;
	colors[ImGuiCol_ScrollbarBg] = C64_BLACK;
	colors[ImGuiCol_ScrollbarGrab] = C64_LGRAY;
	colors[ImGuiCol_ScrollbarGrabHovered] = C64_GREEN;
	colors[ImGuiCol_ScrollbarGrabActive] = C64_LGREEN;
	colors[ImGuiCol_CheckMark] = C64_YELLOW;
	colors[ImGuiCol_SliderGrab] = C64_LGRAY;
	colors[ImGuiCol_SliderGrabActive] = C64_LGREEN;
	colors[ImGuiCol_Button] = C64_DGRAY;
	colors[ImGuiCol_ButtonHovered] = C64_GREEN;
	colors[ImGuiCol_ButtonActive] = C64_PURPLE;
	colors[ImGuiCol_Header] = C64_LGRAY;
	colors[ImGuiCol_HeaderHovered] = C64_CYAN;
	colors[ImGuiCol_HeaderActive] = C64_WHITE;
	colors[ImGuiCol_Separator] = C64_BLACK;
	colors[ImGuiCol_SeparatorHovered] = C64_WHITE;
	colors[ImGuiCol_SeparatorActive] = C64_ORANGE;
	colors[ImGuiCol_ResizeGrip] = C64_WHITE;
	colors[ImGuiCol_ResizeGripHovered] = C64_LGREEN;
	colors[ImGuiCol_ResizeGripActive] = C64_GREEN;
	colors[ImGuiCol_PlotLines] = C64_LGREEN;
	colors[ImGuiCol_PlotLinesHovered] = C64_WHITE;
	colors[ImGuiCol_PlotHistogram] = C64_WHITE;
	colors[ImGuiCol_PlotHistogramHovered] = C64_WHITE;
	colors[ImGuiCol_TextSelectedBg] = C64_BLACK;
	colors[ImGuiCol_DragDropTarget] = C64_PURPLE;
	colors[ImGuiCol_NavHighlight] = C64_PINK;
	colors[ImGuiCol_NavWindowingHighlight] = C64_PINK;
	colors[ImGuiCol_Tab] = C64_BLUE;// ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.90f);
	colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
	colors[ImGuiCol_TabActive] = C64_PURPLE;// ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
	colors[ImGuiCol_TabUnfocused] = C64_BLUE;// ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
	colors[ImGuiCol_TabUnfocusedActive] = C64_DGRAY;// ImLerp(colors[ImGuiCol_TabActive], colors[ImGuiCol_TitleBg], 0.40f);

	ImGuiStyle& style = ImGui::GetStyle();

	style.FramePadding = ImVec2(8, 1);
	style.FrameRounding = 0;

	style.WindowBorderSize = 0;
	style.WindowPadding = ImVec2(0, 0);

	style.PopupBorderSize = 0;
	style.PopupRounding = 0;

	style.WindowRounding = 0;
	style.ScrollbarRounding = 0;
	style.TabRounding = 0;

	style.ItemSpacing = ImVec2(4, 2);
	style.ItemInnerSpacing = ImVec2(0, 0);

	style.GrabMinSize = 8;
	style.GrabRounding = 0;
}

void StyleC64_Green()
{
	ImVec4* colors = ImGui::GetStyle().Colors;

	colors[ImGuiCol_Text] = C64_GREEN;
	colors[ImGuiCol_TextDisabled] = C64_MGRAY;
	colors[ImGuiCol_WindowBg] = C64_BLACK;
	colors[ImGuiCol_ChildBg] = C64_BLACK;
	colors[ImGuiCol_PopupBg] = C64_BLUE;
	colors[ImGuiCol_Border] = C64_LGREEN;
	colors[ImGuiCol_BorderShadow] = C64_DGRAY;
	colors[ImGuiCol_FrameBg] = C64_BLUE;
	colors[ImGuiCol_FrameBgHovered] = C64_YELLOW;
	colors[ImGuiCol_FrameBgActive] = C64_LGREEN;
	colors[ImGuiCol_TitleBg] = C64_BLUE;
	colors[ImGuiCol_TitleBgActive] = C64_GREEN;
	colors[ImGuiCol_TitleBgCollapsed] = C64_BLACK;
	colors[ImGuiCol_MenuBarBg] = C64_BLUE;
	colors[ImGuiCol_ScrollbarBg] = C64_BLACK;
	colors[ImGuiCol_ScrollbarGrab] = C64_LGRAY;
	colors[ImGuiCol_ScrollbarGrabHovered] = C64_GREEN;
	colors[ImGuiCol_ScrollbarGrabActive] = C64_LGREEN;
	colors[ImGuiCol_CheckMark] = C64_WHITE;
	colors[ImGuiCol_SliderGrab] = C64_LGRAY;
	colors[ImGuiCol_SliderGrabActive] = C64_LGREEN;
	colors[ImGuiCol_Button] = C64_BLUE;
	colors[ImGuiCol_ButtonHovered] = C64_PURPLE;
	colors[ImGuiCol_ButtonActive] = C64_BROWN;
	colors[ImGuiCol_Header] = C64_PURPLE;
	colors[ImGuiCol_HeaderHovered] = C64_CYAN;
	colors[ImGuiCol_HeaderActive] = C64_WHITE;
	colors[ImGuiCol_Separator] = C64_BLACK;
	colors[ImGuiCol_SeparatorHovered] = C64_WHITE;
	colors[ImGuiCol_SeparatorActive] = C64_ORANGE;
	colors[ImGuiCol_ResizeGrip] = C64_WHITE;
	colors[ImGuiCol_ResizeGripHovered] = C64_LGREEN;
	colors[ImGuiCol_ResizeGripActive] = C64_GREEN;
	colors[ImGuiCol_PlotLines] = C64_LGREEN;
	colors[ImGuiCol_PlotLinesHovered] = C64_WHITE;
	colors[ImGuiCol_PlotHistogram] = C64_WHITE;
	colors[ImGuiCol_PlotHistogramHovered] = C64_WHITE;
	colors[ImGuiCol_TextSelectedBg] = C64_BLUE;
	colors[ImGuiCol_DragDropTarget] = C64_PURPLE;
	colors[ImGuiCol_NavHighlight] = C64_PINK;
	colors[ImGuiCol_NavWindowingHighlight] = C64_PINK;
	colors[ImGuiCol_Tab] = C64_BLACK;// ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.90f);
	colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
	colors[ImGuiCol_TabActive] = C64_BLUE;// ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
	colors[ImGuiCol_TabUnfocused] = C64_DGRAY;// ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
	colors[ImGuiCol_TabUnfocusedActive] = C64_BLACK;// ImLerp(colors[ImGuiCol_TabActive], colors[ImGuiCol_TitleBg], 0.40f);
}


void StyleC64_Mid()
{
	ImVec4* colors = ImGui::GetStyle().Colors;

	colors[ImGuiCol_Text] = C64_LBLUE;
	colors[ImGuiCol_TextDisabled] = C64_LGRAY;
	colors[ImGuiCol_WindowBg] = C64_BLUE;
	colors[ImGuiCol_ChildBg] = C64_BLUE;
	colors[ImGuiCol_PopupBg] = C64_BLACK;
	colors[ImGuiCol_Border] = C64_LGRAY;
	colors[ImGuiCol_BorderShadow] = C64_BLACK;
	colors[ImGuiCol_FrameBg] = ImVec4(64/384.0f, 49/384.0f, 141/384.0f, 1.0f);
	colors[ImGuiCol_FrameBgHovered] = C64_MGRAY;
	colors[ImGuiCol_FrameBgActive] = C64_BLACK;
	colors[ImGuiCol_TitleBg] = C64_LBLUE;
	colors[ImGuiCol_TitleBgActive] = C64_CYAN;
	colors[ImGuiCol_TitleBgCollapsed] = C64_LBLUE;
	colors[ImGuiCol_MenuBarBg] = C64_BLUE;
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.46f, 0.54f, 0.80f, 0.60f);
	colors[ImGuiCol_Button] = C64_BLACK;
	colors[ImGuiCol_ButtonHovered] = C64_BROWN;
	colors[ImGuiCol_ButtonActive] = C64_MGRAY;
	colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.35f, 0.35f, 0.35f, 0.17f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_Tab] = C64_BLUE;// ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.90f);
	colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
	colors[ImGuiCol_TabActive] = C64_BLACK;// ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
	colors[ImGuiCol_TabUnfocused] = C64_BLUE;// ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
	colors[ImGuiCol_TabUnfocusedActive] = C64_BLUE;// ImLerp(colors[ImGuiCol_TabActive], colors[ImGuiCol_TitleBg], 0.40f);
	colors[ImGuiCol_DockingPreview] = colors[ImGuiCol_Header];
	colors[ImGuiCol_DockingPreview].w = 0.7f;
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.78f, 0.87f, 0.98f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.57f, 0.57f, 0.64f, 1.00f);   // Prefer using Alpha=1.0 here
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.68f, 0.68f, 0.74f, 1.00f);   // Prefer using Alpha=1.0 here
	colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.30f, 0.30f, 0.30f, 0.09f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_NavHighlight] = colors[ImGuiCol_HeaderHovered];
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}
