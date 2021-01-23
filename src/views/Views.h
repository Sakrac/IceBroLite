#pragma once

class FVFileView;

void InitViews();
void ShowViews();
void BeginViews();
void EndViews();
void SetCodeViewAddr(uint16_t addr, int view = -1);
void SetMemoryViewAddr(uint16_t addr, int view = -1);
bool SaveLayoutOnExit();

float CurrFontSize();
uint8_t InputHex();
void SelectFont(int size);
void RefreshScreen(uint8_t* img, uint16_t w, uint16_t h);

FVFileView* GetFileView();
