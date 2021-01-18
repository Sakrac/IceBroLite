#pragma once
#include <stdint.h>
#include "../struse/struse.h"
#include "../imgui/imgui.h"

struct UserData;

struct ScreenView {
	uint8_t* bitmap;
	size_t bitmapSize;

	int width, height;

	ImTextureID texture;
	bool open;
	bool refresh;

	void WriteConfig(UserData& config);
	void ReadConfig(strref config);
	void Draw();
	~ScreenView();
	void Refresh(uint8_t* img, uint16_t w, uint16_t h);

	ScreenView();
};


