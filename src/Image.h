#pragma once

#include "imgui/imgui.h"
#include "sokol/sokol_gfx.h"

enum class ViceMonIcons {
  VMI_BreakPoint = 0,
  VMI_Reload,
  VMI_Load,
  VMI_ViceLoaded,
  VMI_Reset,
  VMI_NMI,
  VMI_Interrupt,
  VMI_SendToVICE,
  VMI_Play,
  VMI_ViceNotLoaded,
  VMI_Step,
  VMI_DisabledBreakPoint,
  VMI_Pause,
  VMI_Disconnected,
  VMI_Sandbox,
  VMI_DontSendToVICE,
  VMI_WatchPoint,
  VMI_WatchPointOf,
  VMI_Connected,
  VMI_SandboxOff,
  VMI_StepOver,
  VMI_StepOut,
  VMI_TracePoint,
  VMI_TracePointOf,
  VMI_Count
};

void LoadIcons();
bool DrawTexturedIcon(ViceMonIcons icon, bool flipX = false, float wid = -1.0f,
                      const ImVec4 &tint = ImVec4(1, 1, 1, 1),
                      const ImVec4 &hover = ImVec4(1, 0.5f, 0.0f, 1.0f));
bool DrawTexturedIconCenter(ViceMonIcons icon, bool flipX = false,
                            float wid = -1.0f,
                            const ImVec4 &tint = ImVec4(1, 1, 1, 1),
                            const ImVec4 &hover = ImVec4(1, 0.5f, 0.0f, 1.0f));
int GetViceMonIconWidth(ViceMonIcons icon);

struct IBLImage {
  sg_image image;
  sg_view view;
  sg_pixel_format format;
  uint16_t width;
  uint16_t height;
};

IBLImage CreateImage(int w, int h, const char *label = nullptr,
                     const void *image_data = nullptr, size_t image_size = 0,
                     int format = SG_PIXELFORMAT_RGBA8);
void DestroyImage(IBLImage image);
void UpdateTextureData(IBLImage image, const void *data, size_t size);
IBLImage UpdateImageSize(IBLImage image, int width, int height, int format,
                         const char *label);
ImTextureID GetImageID(IBLImage image);
static inline bool IBLImageValid(IBLImage image) {
  return image.image.id != SG_INVALID_ID && image.view.id != SG_INVALID_ID;
}
IBLImage InvalidImage();

extern uint32_t c64pal[16];
extern uint32_t vic20pal_sc[16];
extern uint32_t plus4pal[128];
