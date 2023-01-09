#pragma once
#include <stdint.h>
#include "../struse/struse.h"

struct UserData;
struct CPU6510;

struct CodeView
{
	char address[64];
	char editAsmStr[64];

	uint16_t addrValue;
	uint16_t addrCursor;
	uint16_t lastShownPC;
	uint16_t lastShownAddress;

	CodeView();

	void SetAddr( uint16_t addr );

	void WriteConfig( UserData & config );

	void ReadConfig( strref config );

	int editAsmAddr;
	int cursor[ 2 ];
	int contextAddr;
	int lastShownPCRow;
	int srcColDif, srcColDif0;
	float srcColDrag;
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
	bool trackPC;
	bool open;
	bool evalAddress;
	bool showPCAddress;
	bool focusPC;
	bool editAsmFocusRequested;
	bool dragging;

	void Draw( int index );

protected:
	bool EditAssembly();
	void CodeContextMenu(CPU6510* cpu, int index);
	void UpdateTrackPC(CPU6510* cpu, int& dY, int lines);
};

