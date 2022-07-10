#pragma once
#include <stdint.h>
#include "../struse/struse.h"
#include "../imgui/imgui.h"

struct UserData;

struct ScreenView {
	uint8_t* bitmap;
	size_t bitmapSize;

	enum class BorderMode {
		Full,
		Borders,
		Screen
	};

	int width, height;
	int offs_x, offs_y, scrn_w, scrn_h;
	int borderMode;

	ImTextureID texture;
	bool open;
	bool refresh;
	bool drawRasterTime;

	void WriteConfig(UserData& config);
	void ReadConfig(strref config);
	void Draw();
	~ScreenView();
	void Refresh(uint8_t* img, uint16_t w, uint16_t h,
		uint16_t sx, uint16_t sy, uint16_t sw, uint16_t sh);

	ScreenView();
};


