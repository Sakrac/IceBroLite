#include <malloc.h>
#include "GLFW/glfw3.h"
#include "../imgui/imgui.h"
#include "../Image.h"
#include "../Config.h"
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
		if (name.same_str("open") && type == CPT_Value) {
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
		memcpy(bitmap, img, bitmapSize);
		refresh = true;
	}
}

ScreenView::ScreenView() : bitmap(nullptr), bitmapSize(0), width(0), height(0), open(true), refresh(false)
{
	texture = 0;
}
