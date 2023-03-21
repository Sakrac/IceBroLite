#pragma once
#include <stdint.h>
struct ImVec4;
void ResetCodeColoring();
ImVec4 GetCodeBytesColor();
ImVec4 GetCodeAddrColor();
ImVec4 GetCodeOpCodeColor();
ImVec4 GetWatchChessColor();
ImVec4 GetCodeLabelColor();
ImVec4 GetCodeSourceColor();
ImVec4 GetCodeParamColor();
ImVec4 GetCodeCursorColor();
ImVec4 GetPCHighlightColor();
int GetPCHighlightStyle();
void InvalidateBranchTargets();
ImVec4* GetBranchTargetColor(uint16_t addr);
ImVec4* MakeBranchTargetColor(uint16_t addr);
void LoadCustomTheme(const char* themeFile);
void SaveCustomTheme(const char* themeFile);
void SetCustomTheme();
bool HasCustomTheme();
bool CustomThemeActive();

void ThemeColorMenu();
