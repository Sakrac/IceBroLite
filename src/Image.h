#pragma once

enum ViceMonIcons {
	VMI_BreakPoint = 0,
	VMI_Reload,
	VMI_Load,
	VMI_Connected,
	VMI_Reset,
	VMI_NMI,
	VMI_Interrupt,
	VMI_SendToVICE,
	VMI_Play,
	VMI_Disconnected,
	VMI_Step,
	VMI_StebBack,
	VMI_Pause,
	VMI_PauseOff,
	VMI_Sandbox,
	VMI_DontSendToVICE,
	VMI_SandboxOff,

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

