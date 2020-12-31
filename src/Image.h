#pragma once

#include "struse/struse.h"
#include <vector>

struct UserData;
struct ConfigParse;

ImTextureID CreateTexture();
void UpdateTextureData(int width, int height, int channels, const void* data);
void DestroyTexture(ImTextureID texID);
