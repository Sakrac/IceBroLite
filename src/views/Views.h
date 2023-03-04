#pragma once

class FVFileView;

void InitViews();
void ShowViews();
void BeginViews();
void EndViews();
void SetCodeViewAddr(uint16_t addr, int view = -1);
void SetMemoryViewAddr(uint16_t addr, int view = -1);
bool SaveLayoutOnExit();
void ReviewListing();

void AddWatch(int watch, const char* expr);

void SetCodeAddr(int code, uint16_t addr);

float CurrFontSize();
uint8_t InputHex();
void SelectFont(int size);
void RefreshScreen(uint8_t* img, uint16_t w, uint16_t h,
	uint16_t sx, uint16_t sy, uint16_t sw, uint16_t sh);
bool LoadUserFont(const char* file, int size);
void CheckUserFont();
bool UseCustomFont();
void SetInitialFont();

void ForceUserFont(strref file, int size);

int PetsciiFont();
void ViewPushFont(int font);
void ViewPopFont();

FVFileView* GetFileView();
uint8_t DrawPaletteMenu(uint8_t col);
int GetPCHighlightStyle();
uint32_t GetPCHighlightColor();
