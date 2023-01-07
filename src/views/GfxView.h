#pragma once
#include <stdint.h>
#include "../struse/struse.h"
struct UserData;
struct CPU6510;

struct GfxView {
	enum System {
		Generic,
		C64,
		Count
	};

	enum Mode {
		Generic_Modes = 0,
		Planar = Generic_Modes,
		Columns,
		ColumnScreen_MC,
		C64_Modes,
		C64_Current = C64_Modes,
		C64_Text,
		C64_Bitmap,
		C64_Sprites
	};

	enum Zoom {
		Zoom_1x1,
		Zoom_2x2,
		Zoom_4x4,
		Zoom_FitX,
		Zoom_FitY,
		Zoom_FitWindow
	};

	char address_screen[128];
	char address_gfx[128];
	char address_col[128];
	char columns_str[32];
	char rows_str[32];
	char columns_spr_str[32];
	char rows_spr_str[32];

	uint32_t addrScreenValue;
	uint32_t addrGfxValue;
	uint32_t addrColValue;
	uint32_t columns;
	uint32_t rows;
	uint32_t columns_sprite;
	uint32_t rows_sprite;

	int zoom;
	int displaySystem;
	int displayMode;
	int genericMode;
	int c64Mode;

	uint8_t* bitmap;
	size_t bitmapSize;
	int bitmapWidth;
	int bitmapHeight;

	ImTextureID texture;

	uint8_t bg;
	uint8_t spr_col[3];
	uint8_t txt_col[3];

	bool open;
	bool reeval;

	bool color;
	bool multicolor;
	bool ecbm;
	bool vicColors;

	bool useRomFont;

	GfxView();

	void WriteConfig(UserData& config);

	void ReadConfig(strref config);

	void Draw(int index);
	void Create8bppBitmap(CPU6510* cpu);

	void CreatePlanarBitmap(CPU6510* cpu, uint32_t* dst, int lines, uint32_t width, const uint32_t* palette);
	void CreateColumnsBitmap(CPU6510* cpu, uint32_t* dst, int lines, uint32_t width, const uint32_t* palette);

	void PrintCurrentInfo(CPU6510* cpu);
	void PrintHoverInfo(CPU6510* cpu, int* hoverPos);

	void CreateC64BitmapBitmap(CPU6510* cpu, uint32_t* dst, const uint32_t* palette, uint16_t bitmap, size_t cl, uint32_t rw);
	void CreateC64ColorBitmapBitmap(CPU6510* cpu, uint32_t* dst, const uint32_t* palette, uint16_t bitmap, uint16_t screen, size_t cl, uint32_t rw);
	void CreateC64ExtBkgTextBitmap(CPU6510* cpu, uint32_t* dst, const uint32_t* palette, uint16_t bitmap, uint16_t screen, uint16_t cm, size_t cl, uint32_t rw);
	void CreateC64TextBitmap(CPU6510* cpu, uint32_t* dst, const uint32_t* palette, size_t cl, uint32_t rw);
	void CreateC64ColorTextBitmap(CPU6510* cpu, uint32_t* d, const uint32_t* pal, uint16_t bitmap, uint16_t screen, uint16_t cm, size_t cl, uint32_t rw);
	void CreateC64MulticolorTextBitmap(CPU6510* cpu, uint32_t* dst, const uint32_t* palette, uint16_t bitmap, uint16_t screen, uint16_t cm, size_t cl, uint32_t rw);
	void CreateC64MulticolorBitmapBitmap(CPU6510* cpu, uint32_t* dst, const uint32_t* palette, uint16_t bitmap, uint16_t screen, uint16_t cm, size_t cl, uint32_t rw);
	void CreateC64SpritesBitmap(CPU6510* cpu, uint32_t* dst, int lines, uint32_t width, const uint32_t* palette);
	void CreateC64SpritesMCBitmap(CPU6510* cpu, uint32_t* d, int linesHigh, uint32_t w, const uint32_t* pal);
	void CreateC64ColorTextColumns(CPU6510* cpu, uint32_t* d, const uint32_t* pal, uint16_t bitmap, uint16_t screen, uint16_t colorAddr, size_t cl, uint32_t rw);
	void CreateC64CurrentBitmap(CPU6510* cpu, uint32_t* d, const uint32_t* pal);
};

