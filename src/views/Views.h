#pragma once

class FVFileView;

void InitViews();
void ShowViews();

float CurrFontSize();
uint8_t InputHex();
void SelectFont(int size);
void RefreshScreen(uint8_t* img, uint16_t w, uint16_t h);

FVFileView* GetFileView();