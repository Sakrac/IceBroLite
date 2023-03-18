#pragma once
#include <stdint.h>
struct ImVec4;
void ResetCodeColoring();
ImVec4 GetCodeBytesColor();
void InvalidateBranchTargets();
ImVec4* GetBranchTargetColor(uint16_t addr);
ImVec4* MakeBranchTargetColor(uint16_t addr);

void ThemeColorMenu();
