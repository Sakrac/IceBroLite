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
