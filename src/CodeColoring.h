#pragma once
#include <stdint.h>
struct ImVec4;
void ResetCodeColoring();
ImVec4 GetCodeBytesColor();
ImVec4 GetCodeAddrColor();
ImVec4 GetCodeOpCodeColor();
ImVec4 GetCodeSourceColor();
ImVec4 GetCodeParamColor();
void InvalidateBranchTargets();
ImVec4* GetBranchTargetColor(uint16_t addr);
ImVec4* MakeBranchTargetColor(uint16_t addr);
void LoadCustomTheme(const char* themeFile);
void SaveCustomTheme(const char* themeFile);
void SetCustomTheme();
bool HasCustomTheme();
bool CustomThemeActive();

void ThemeColorMenu();
