#include "imgui.h"
#include "imgui_internal.h"
#include "GLFW/glfw3.h"

void ForceKeyboardCanvas(const char* label)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	const ImGuiID id = window->GetID(label);

	if (g.ActiveId!=id) {
		ImGui::SetActiveID(id, window);
		ImGui::SetFocusID(id, window);
		ImGui::FocusWindow(window);
	}
}

static ImGuiID sCanvasID = 0;

bool KeyboardCanvas( const char* label )
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if( window->SkipItems )
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiIO& io = g.IO;

	const ImGuiID id = window->GetID( label );

	const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
	const bool clicked = io.MouseClicked[0];
	if (clicked) {
		if (hovered) {
			sCanvasID = id;
		} else if (sCanvasID == id) {
			sCanvasID = 0;
		}
	}

	const bool user_focused = sCanvasID == id;
	const bool user_clicked = hovered && clicked;
	const bool user_nav_input_start = (g.ActiveId != id) && ((g.NavActivateInputId == id) || (g.NavActivateId == id && g.NavInputSource == ImGuiInputSource_Keyboard));

	if(user_focused || user_clicked || user_nav_input_start)
	{
		if( g.ActiveId != id )
		{
			ImGui::SetActiveID( id, window );
			ImGui::SetFocusID( id, window );
			ImGui::FocusWindow( window );
		}
	} else if (g.ActiveId == id) {
		if ((!hovered && io.MouseClicked[0]) || ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_ESCAPE))
			ImGui::ClearActiveID();
	}

	return g.ActiveId == id;
}


