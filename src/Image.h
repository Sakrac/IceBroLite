#pragma once

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
bool DrawTexturedIcon(ViceMonIcons icon, bool flipX = false, float wid = -1.0f, const ImVec4& tint = ImVec4(1, 1, 1, 1), const ImVec4& hover = ImVec4(1, 0.5f, 0.0f, 1.0f));
bool DrawTexturedIconCenter(ViceMonIcons icon, bool flipX = false, float wid = -1.0f, const ImVec4& tint = ImVec4(1, 1, 1, 1), const ImVec4& hover = ImVec4(1, 0.5f, 0.0f, 1.0f));
int GetViceMonIconWidth(ViceMonIcons icon);

ImTextureID CreateTexture();
void SelectTexture(ImTextureID img);
void UpdateTextureData(int width, int height, const void* data);
//ImTextureID LoadTexture( const char* filename, int* width, int* height );

extern uint32_t c64pal[16];
extern uint32_t vic20pal_sc[16];
extern uint32_t plus4pal[128];
