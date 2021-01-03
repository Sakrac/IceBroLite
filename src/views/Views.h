#pragma once

class FVFileView;

void InitViews();
void ShowViews();

float CurrFontSize();
uint8_t InputHex();
void SelectFont(int size);

FVFileView* GetFileView();