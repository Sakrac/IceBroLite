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

	void Draw();

	ScreenView();
};


