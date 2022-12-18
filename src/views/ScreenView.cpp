#include <malloc.h>
#include <stdint.h>
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include "../Image.h"
#include "../Config.h"
#include "../platform.h"
#include "../6510.h"
#include "../C64Colors.h"
#include "GLFW/glfw3.h"
#include "ScreenView.h"


void ScreenView::WriteConfig(UserData& config)
{
	config.AddValue(strref("open"), config.OnOff(open));
	config.AddValue(strref("borders"), borderMode);
	config.AddValue(strref("rasterTime"), config.OnOff(drawRasterTime));
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
		if (name.same_str("borders") && type == ConfigParseType::CPT_Value) {
			borderMode = (int)value.atoi();
			if (borderMode < 0 || borderMode >(int)BorderMode::Screen) { borderMode = 0; }
		}
		if (name.same_str("rasterTime") && type == ConfigParseType::CPT_Value) {
			drawRasterTime = !value.same_str("Off");
		}
	}
}

void ScreenView::Draw()
{
	if (!open) { return; }

	ImGui::SetNextWindowPos(ImVec2(400, 150), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(520, 400), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Screen", &open)) {
		ImGui::End();
		return;
	}

	ImVec2 cursorTop = ImGui::GetCursorPos();

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
		if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_C) &&
			(ImGui::IsKeyDown((ImGuiKey)GLFW_KEY_LEFT_CONTROL) || ImGui::IsKeyDown((ImGuiKey)GLFW_KEY_RIGHT_CONTROL))) {
			CopyBitmapToClipboard(bitmap, width, height);
		}
	}

	if (texture) {
		ImVec2 uv0(0, 0), uv1(1, 1);

//		ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));

//		void ImGui::Image(ImTextureID user_texture_id, const ImVec2 & size, const ImVec2 & uv0, const ImVec2 & uv1, const ImVec4 & tint_col, const ImVec4 & border_col)

		switch ((BorderMode)borderMode) {
			case BorderMode::Full:
//				ImGui::Image(texture, size);
				break;
			case BorderMode::Borders: {
				int x0 = offs_x - 32;
				int x1 = offs_x + scrn_w + 32;
				int y0 = offs_y - 32;
				int y1 = offs_y + scrn_h + 32;
				if (x0 < 0) { x0 = 0; }
				if (x1 > width) { x1 = width; }
				if (y0 < 0) { y0 = 0; }
				if (y1 > height) { y1 = height; }
				uv0 = ImVec2((float)x0 / (float)width, (float)y0 / (float)height);
				uv1 = ImVec2((float)x1 / (float)width, (float)y1 / (float)height);
//				ImGui::Image(texture, size, uv0, uv1);
				break;
			}
			case BorderMode::Screen: {
				int x0 = offs_x;
				int x1 = offs_x + scrn_w;
				int y0 = offs_y;
				int y1 = offs_y + scrn_h;
				uv0 = ImVec2((float)x0 / (float)width, (float)y0 / (float)height);
				uv1 = ImVec2((float)x1 / (float)width, (float)y1 / (float)height);
				break;
			}
		}

		ImVec2 size((float)width * (uv1.x-uv0.x), (float)height * (uv1.y - uv0.y));
		float x = ImGui::GetWindowWidth();
		float y = ImGui::GetWindowHeight();
		float s = 1.0f;
		if ((x * size.y) < (y * size.x)) {
			s = x / size.x;
			size.y *= s; size.x = x;
		}
		else {
			s = y / size.y;
			size.x *= s; size.y = y;
		}
		ImGui::Image(texture, size, uv0, uv1);

		if (drawRasterTime) {
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			ImVec2 winPos = ImGui::GetWindowPos();
			ImVec2 winSize = ImGui::GetWindowSize();

			winPos.x += cursorTop.x;
			winPos.y += cursorTop.y;

			CPU6510* cpu = GetCurrCPU();

			float line = (float)cpu->regs.LIN * s - (height * uv0.y) * s;
			float column = (float)cpu->regs.CYC * 8.0f * s - (width * uv0.x) * s;

			draw_list->AddLine(ImVec2(winPos.x, winPos.y + line), ImVec2(winPos.x + size.x, winPos.y + line),
				ImColor(C64_LGREEN));
			draw_list->AddLine(ImVec2(winPos.x + column, winPos.y), ImVec2(winPos.x + column, winPos.y + size.y),
				ImColor(C64_LGREEN));
		}

		if (ImGui::GetCurrentWindow() == GImGui->HoveredWindow) {
			ImGui::SetCursorPos(cursorTop);
			ImGui::Text("Select Crop Mode");
			ImGui::RadioButton("Full View", &borderMode, 0);
			ImGui::RadioButton("Normal Borders", &borderMode, 1);
			ImGui::RadioButton("Screen Only", &borderMode, 2);
			ImGui::Checkbox("Draw Raster Time", &drawRasterTime);
		}
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

void ScreenView::Refresh(uint8_t* img, uint16_t w, uint16_t h,
	uint16_t sx, uint16_t sy, uint16_t sw, uint16_t sh)
{
	offs_x = sx; offs_y = sy; scrn_w = sw; scrn_h = sh;
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

ScreenView::ScreenView() : bitmap(nullptr), bitmapSize(0), width(0), height(0), open(true), refresh(false), drawRasterTime(false)
{
	offs_x = 0;
	offs_y = 0;
	scrn_w = 320;
	scrn_h = 200;
	texture = 0;
	borderMode = (int)BorderMode::Full;
}
