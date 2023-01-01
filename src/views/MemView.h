#pragma once
#include <stdint.h>
#include "../struse/struse.h"
struct UserData;

struct MemView {
	char address[128];
	char span[16];

	uint32_t addrValue;
	uint32_t spanValue;

	int cursor[2];

	MemView();

	float cursorTime;
	float mouseWheelDiff;
	bool showAddress;
	bool showHex;
	bool showText;

	bool forceKeyboardInput;
	bool wasActive;
	bool fixedAddress;

	bool open;
	bool evalAddress;
	bool textLowercase;

	void SetAddr(uint16_t addr);
	void Draw(int index);
	void WriteConfig(UserData& config);
	void ReadConfig(strref config);
};

