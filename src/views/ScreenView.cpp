#include "ScreenView.h"

void ScreenView::Draw()
{
	ImGui::SetNextWindowPos(ImVec2(400, 150), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(520, 400), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Screen", &open)) {
		ImGui::End();
		return;
	}
	ImGui::Text("Screenshot goes here");
	ImGui::End();
}

ScreenView::ScreenView() : bitmap(nullptr), bitmapSize(0), width(0), height(0), open(true)
{
	texture = 0;
}
