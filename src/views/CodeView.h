#pragma once
#include <stdint.h>
#include "../struse/struse.h"

struct UserData;

struct CodeView
{
	char address[64];
	char editAsmStr[64];

	uint16_t addrValue;
	uint16_t addrCursor;
	uint16_t lastShownPC;

	CodeView();

	void SetAddr( uint16_t addr );

	void WriteConfig( UserData & config );

	void ReadConfig( strref config );

	int editAsmAddr;
	int cursor[ 2 ];
	int contextAddr;
	float cursorTime;
	float mouseWheelDiff;
	float mouseDragY, dragDiff;

	bool showAddress;
	bool showBytes;
	bool showDisAsm;
	bool showRefs;
	bool showSrc;
	bool showLabels;
	bool fixedAddress;
	bool open;
	bool evalAddress;
	bool showPCAddress;
	bool focusPC;
	bool editAsmFocusRequested;
	bool dragging;

	void Draw( int index );
};

