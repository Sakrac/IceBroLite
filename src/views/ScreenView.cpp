#include <malloc.h>
#include <stdint.h>
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include "../Image.h"
#include "../Config.h"
#include "../platform.h"
#include "GLFW/glfw3.h"
#include "ScreenView.h"


void ScreenView::WriteConfig(UserData& config)
{
	config.AddValue(strref("open"), config.OnOff(open));
}

void ScreenView::ReadConfig(strref config)
{
	ConfigParse conf(config);
	while (!conf.Empty()) {
		strref name, value;
		ConfigParseType type = conf.Next(&name, &value);
		if (name.same_str("open") && type == ConfigParseType::CPT_Value) {
			open = !value.same_str("Off");
		}
	}
}

void ScreenView::Draw()
{
	ImGui::SetNextWindowPos(ImVec2(400, 150), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(520, 400), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Screen", &open)) {
		ImGui::End();
		return;
	}

	if (refresh) {
		if (!texture) {
			texture = CreateTexture();
		}
		if (texture) {
			SelectTexture(texture);
			UpdateTextureData(width, height, bitmap);
		}
	}

	ImGuiContext* g = ImGui::GetCurrentContext();
	if (g->CurrentWindow == g->NavWindow) {
		if (ImGui::IsKeyPressed(GLFW_KEY_C) &&
			(ImGui::IsKeyDown(GLFW_KEY_LEFT_CONTROL) || ImGui::IsKeyDown(GLFW_KEY_RIGHT_CONTROL))) {
			CopyBitmapToClipboard(bitmap, width, height);
		}
	}

	if (texture) {
		ImVec2 size((float)width, (float)height);
		float x = ImGui::GetWindowWidth();
		float y = ImGui::GetWindowHeight();
		if ((x * size.y) < (y * size.x)) {
			size.y *= x / size.x; size.x = x;
		} else {
			size.x *= y / size.y; size.y = y;
		}
		ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
		ImGui::Image(texture, size);
	} else {
		ImGui::Text("Screen will draw here");
	}

	ImGui::End();
}

ScreenView::~ScreenView()
{
	if (bitmap) { free(bitmap); }
	bitmap = nullptr;
}

void ScreenView::Refresh(uint8_t* img, uint16_t w, uint16_t h)
{
	if (bitmap && ((width != w) || (height != h))) {
		free(bitmap);
		bitmap = nullptr;
	}
	if (!bitmap) {
		bitmapSize = (size_t)w * (size_t)h * 4;
		bitmap = (uint8_t*)calloc(1, bitmapSize);
	}
	if (bitmap) {
		width = w;
		height = h;
//		memcpy(bitmap, img, bitmapSize);
		uint32_t* bo = (uint32_t*)bitmap;
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				*bo++ = c64pal[(*img++)&0xf];
			}
		}
		refresh = true;
	}
}

ScreenView::ScreenView() : bitmap(nullptr), bitmapSize(0), width(0), height(0), open(true), refresh(false)
{
	texture = 0;
}
