#include "ImGui.h"
#include "imgui_internal.h"

void ForceKeyboardCanvas(const char* label)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	const ImGuiIO& io = g.IO;

	const ImGuiID id = window->GetID(label);

	if (g.ActiveId!=id) {
		ImGui::SetActiveID(id, window);
		ImGui::SetFocusID(id, window);
		ImGui::FocusWindow(window);
	}
}

bool KeyboardCanvas( const char* label )
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if( window->SkipItems )
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiIO& io = g.IO;

	const ImGuiID id = window->GetID( label );

	const bool hovered = ImGui::IsWindowHovered();
	const bool focus_requested = ImGui::FocusableItemRegister( window, id );    // Using completion callback disable keyboard tabbing

	const bool user_clicked = hovered && io.MouseClicked[ 0 ];
	const bool user_nav_input_start = (g.ActiveId != id) && ((g.NavInputId == id) || (g.NavActivateId == id && g.NavInputSource == ImGuiInputSource_NavKeyboard));

	if( focus_requested || user_clicked || user_nav_input_start )
	{
		if( g.ActiveId != id )
		{
			ImGui::SetActiveID( id, window );
			ImGui::SetFocusID( id, window );
			ImGui::FocusWindow( window );
		}
	}
	else if( g.ActiveId == id && io.MouseClicked[ 0 ] )
		ImGui::ClearActiveID();

	return g.ActiveId == id;
#if 0

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if( window->SkipItems )
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiID id = window->GetID( label );
	if( ( ImGui::IsWindowHovered() && g.IO.MouseClicked[ 0 ] ) && ( g.NavActivateId == id ) )
	{
		if( g.ActiveId != id )
		{
			ImGui::SetActiveID( id, window );
			ImGui::SetFocusID( id, window );
			ImGui::FocusWindow( window );
		}
	}
	else if( g.ActiveId == id && g.IO.MouseClicked[ 0 ] )
		ImGui::ClearActiveID();

	return g.ActiveId == id;
#endif
}


