#define GL_SILENCE_DEPRECATION
#include <stdint.h>
#include "imgui/imgui.h"
#include "GLFW/glfw3.h"
#include "Image.h"

extern const int sIcons_Width;
extern const int sIcons_Height;
extern const unsigned char sIcons_Pixels[];

#define ColRGBA( r, g, b, a ) uint32_t((a<<24)|(b<<16)|(g<<8)|(r))
uint32_t c64pal[16] = {
	ColRGBA(0,0,0,255),
	ColRGBA(255,255,255,255),
	ColRGBA(136,57,50,255),
	ColRGBA(103,182,189,255),
	ColRGBA(139,63,150,255),
	ColRGBA(85,160,73,255),
	ColRGBA(64,49,141,255),
	ColRGBA(191,206,114,255),
	ColRGBA(139,84,41,255),
	ColRGBA(87,66,0,255),
	ColRGBA(184,105,98,255),
	ColRGBA(80,80,80,255),
	ColRGBA(120,120,120,255),
	ColRGBA(148,224,137,255),
	ColRGBA(120,105,196,255),
	ColRGBA(159,159,159,255)
};

uint32_t vic20pal_sc[16] = {
	// Form vice : mike pal
	ColRGBA(0,0,0,255),
	ColRGBA(255,255,255,255),
	ColRGBA(182,31,33,255),
	ColRGBA(77,240,255,255),
	ColRGBA(180,63,255,255),
	ColRGBA(68,226,55,255),
	ColRGBA(26,52,255,255),
	ColRGBA(220,215,27,255),
	ColRGBA(202,84,0,255),
	ColRGBA(233,176,114,255),
	ColRGBA(231,146,147,255),
	ColRGBA(154,247,253,255),
	ColRGBA(224,159,255,255),
	ColRGBA(143,228,147,255),
	ColRGBA(130,144,255,255),
	ColRGBA(229,222,133,255),
};

uint32_t plus4pal[128] = {
	// from vice : yape pal
	ColRGBA(0,0,0,255),
	ColRGBA(39,39,39,255),
	ColRGBA(96,15,16,255),
	ColRGBA(0,64,63,255),
	ColRGBA(86,4,102,255),
	ColRGBA(0,75,0,255),
	ColRGBA(26,26,140,255),
	ColRGBA(53,52,0,255),
	ColRGBA(83,30,0,255),
	ColRGBA(71,40,0,255),
	ColRGBA(24,67,0,255),
	ColRGBA(97,8,52,255),
	ColRGBA(0,71,30,255),
	ColRGBA(4,41,122,255),
	ColRGBA(40,19,143,255),
	ColRGBA(8,72,0,255),
	ColRGBA(0,0,0,255),
	ColRGBA(55,55,55,255),
	ColRGBA(111,30,31,255),
	ColRGBA(0,79,78,255),
	ColRGBA(101,19,117,255),
	ColRGBA(4,90,5,255),
	ColRGBA(42,42,156,255),
	ColRGBA(68,68,0,255),
	ColRGBA(99,46,0,255),
	ColRGBA(86,56,0,255),
	ColRGBA(40,82,0,255),
	ColRGBA(112,23,67,255),
	ColRGBA(0,86,46,255),
	ColRGBA(20,56,138,255),
	ColRGBA(56,34,158,255),
	ColRGBA(23,88,0,255),
	ColRGBA(0,0,0,255),
	ColRGBA(67,67,67,255),
	ColRGBA(124,43,44,255),
	ColRGBA(11,92,91,255),
	ColRGBA(114,32,130,255),
	ColRGBA(17,103,17,255),
	ColRGBA(54,55,168,255),
	ColRGBA(81,80,0,255),
	ColRGBA(111,58,0,255),
	ColRGBA(99,68,0,255),
	ColRGBA(52,95,0,255),
	ColRGBA(125,36,80,255),
	ColRGBA(10,99,58,255),
	ColRGBA(33,69,150,255),
	ColRGBA(69,47,171,255),
	ColRGBA(36,101,0,255),
	ColRGBA(0,0,0,255),
	ColRGBA(85,85,85,255),
	ColRGBA(141,60,61,255),
	ColRGBA(28,109,108,255),
	ColRGBA(131,49,147,255),
	ColRGBA(34,120,34,255),
	ColRGBA(71,72,185,255),
	ColRGBA(98,97,0,255),
	ColRGBA(128,75,17,255),
	ColRGBA(116,85,0,255),
	ColRGBA(69,112,0,255),
	ColRGBA(142,53,97,255),
	ColRGBA(27,116,75,255),
	ColRGBA(50,86,167,255),
	ColRGBA(86,64,188,255),
	ColRGBA(53,118,0,255),
	ColRGBA(0,0,0,255),
	ColRGBA(121,121,121,255),
	ColRGBA(178,97,98,255),
	ColRGBA(64,145,144,255),
	ColRGBA(167,85,183,255),
	ColRGBA(70,157,71,255),
	ColRGBA(108,108,222,255),
	ColRGBA(134,134,20,255),
	ColRGBA(165,112,53,255),
	ColRGBA(153,122,34,255),
	ColRGBA(106,148,21,255),
	ColRGBA(179,89,134,255),
	ColRGBA(63,152,112,255),
	ColRGBA(86,123,204,255),
	ColRGBA(122,100,225,255),
	ColRGBA(89,154,34,255),
	ColRGBA(0,0,0,255),
	ColRGBA(169,169,169,255),
	ColRGBA(225,144,145,255),
	ColRGBA(112,193,192,255),
	ColRGBA(215,133,231,255),
	ColRGBA(118,204,118,255),
	ColRGBA(156,156,255,255),
	ColRGBA(182,182,68,255),
	ColRGBA(213,160,101,255),
	ColRGBA(200,169,82,255),
	ColRGBA(154,196,69,255),
	ColRGBA(226,137,181,255),
	ColRGBA(111,200,160,255),
	ColRGBA(134,170,251,255),
	ColRGBA(170,148,255,255),
	ColRGBA(137,202,82,255),
	ColRGBA(0,0,0,255),
	ColRGBA(199,199,199,255),
	ColRGBA(255,175,176,255),
	ColRGBA(143,224,223,255),
	ColRGBA(246,163,255,255),
	ColRGBA(148,235,149,255),
	ColRGBA(186,186,255,255),
	ColRGBA(212,212,98,255),
	ColRGBA(243,190,131,255),
	ColRGBA(231,200,112,255),
	ColRGBA(184,226,99,255),
	ColRGBA(255,167,212,255),
	ColRGBA(141,231,190,255),
	ColRGBA(164,201,255,255),
	ColRGBA(200,179,255,255),
	ColRGBA(168,232,112,255),
	ColRGBA(0,0,0,255),
	ColRGBA(250,250,250,255),
	ColRGBA(255,226,227,255),
	ColRGBA(194,255,255,255),
	ColRGBA(255,214,255,255),
	ColRGBA(199,255,200,255),
	ColRGBA(237,237,255,255),
	ColRGBA(255,255,149,255),
	ColRGBA(255,241,182,255),
	ColRGBA(255,251,163,255),
	ColRGBA(235,255,150,255),
	ColRGBA(255,218,255,255),
	ColRGBA(192,255,241,255),
	ColRGBA(215,252,255,255),
	ColRGBA(251,230,255,255),
	ColRGBA(219,255,163,255),
};

ImTextureID CreateTexture()
{
	// Turn the RGBA pixel data into an OpenGL texture:
	GLuint my_opengl_texture = 0;
	glGenTextures(1, &my_opengl_texture);
	glBindTexture(GL_TEXTURE_2D, my_opengl_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	return (ImTextureID)(size_t)my_opengl_texture;
}

void SelectTexture(ImTextureID img)
{
	glBindTexture(GL_TEXTURE_2D, (GLuint)(size_t)img);
}

void UpdateTextureData(int width, int height, const void* data)
{
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

// data for icons
struct sTexArea {
	int x, y, w, h;
} aIcons[] = {
	{  0 * 2,  0 * 2, 12 * 2, 11 * 2 },
	{ 12 * 2,  0 * 2, 12 * 2, 12 * 2 },
	{ 24 * 2,  0 * 2, 12 * 2, 12 * 2 },
	{ 36 * 2,  0 * 2, 12 * 2, 12 * 2 },
	{ 48 * 2,  0 * 2, 12 * 2, 12 * 2 },

	{  0 * 2, 12 * 2, 12 * 2, 12 * 2 },
	{ 12 * 2, 12 * 2, 12 * 2, 12 * 2 },
	{ 24 * 2, 12 * 2, 12 * 2, 12 * 2 },
	{ 36 * 2, 12 * 2, 12 * 2, 12 * 2 },
	{ 48 * 2, 12 * 2, 12 * 2, 12 * 2 },

	{  0 * 2, 24 * 2, 12 * 2, 12 * 2 },
	{ 12 * 2, 24 * 2, 12 * 2, 12 * 2 },
	{ 24 * 2, 24 * 2, 12 * 2, 12 * 2 },
	{ 36 * 2, 24 * 2, 12 * 2, 12 * 2 },
	{ 48 * 2, 24 * 2, 12 * 2, 16 * 2 },

	{  0 * 2, 36 * 2, 12 * 2, 12 * 2 },
	{ 12 * 2, 36 * 2, 12 * 2, 12 * 2 },
	{ 24 * 2, 36 * 2, 12 * 2, 12 * 2 },
	{ 36 * 2, 36 * 2, 12 * 2, 12 * 2 },
	{ 48 * 2, 36 * 2, 12 * 2, 16 * 2 },

	{  0 * 2, 48 * 2, 12 * 2, 12 * 2 },
	{ 12 * 2, 48 * 2, 12 * 2, 12 * 2 },
	{ 24 * 2, 48 * 2, 12 * 2, 12 * 2 },
	{ 36 * 2, 48 * 2, 12 * 2, 12 * 2 },
	{ 48 * 2, 48 * 2, 12 * 2, 16 * 2 },
};

static ImTextureID aIconID = {};
static int sIconTexWidth = 0;
static int sIconTexHeight = 0;

void LoadIcons()
{
	if (ImTextureID id = CreateTexture()) {
		UpdateTextureData(sIcons_Width, sIcons_Height, sIcons_Pixels);
		sIconTexWidth = sIcons_Width;
		sIconTexHeight = sIcons_Height;
		aIconID = id;
	}
}

int GetViceMonIconWidth(ViceMonIcons icon)
{
	return aIcons[(size_t)icon].w;
}

bool DrawTexturedIcon(ViceMonIcons icon, bool flipX, float width, const ImVec4& tint, const ImVec4& hover)
{
	bool ret = false;
	if (aIconID) {
		float iW = 1.0f / (float)sIconTexWidth;
		float iH = 1.0f / (float)sIconTexHeight;
		float du = iW * aIcons[(size_t)icon].w;
		float u0 = iW * aIcons[(size_t)icon].x, u1 = u0;
		float s = width > 0.0f ? width / (float)aIcons[(size_t)icon].w : 1.0f;
		if (flipX) { u0 += du; } else { u1 += du; }

		// hover or ting?
		const ImVec4* color = &tint;
		{
			ImVec2 mousePos = ImGui::GetMousePos();
			ImVec2 winPos = ImGui::GetWindowPos();
			ImVec2 curPos = ImGui::GetCursorPos();
			float w = s * aIcons[(size_t)icon].w;
			float h = s * aIcons[(size_t)icon].h;
			float x = mousePos.x - (winPos.x + curPos.x);
			float y = mousePos.y - (winPos.y + curPos.y);
			if (x > 0 && x < w && y > 0 && y < h) {
				color = &hover;
				ret = ImGui::IsMouseReleased(0);
			}
		}
		ImGui::Image(aIconID,
					 ImVec2(s * aIcons[(size_t)icon].w, s * aIcons[(size_t)icon].h),
					 ImVec2(u0, iH * aIcons[(size_t)icon].y),
					 ImVec2(u1, iH * (aIcons[(size_t)icon].y + aIcons[(size_t)icon].h)),
					 *color);
	}
	return ret;
}

bool DrawTexturedIconCenter(ViceMonIcons icon, bool flipX, float width, const ImVec4& tint, const ImVec4& hover)
{
	ImGui::SetCursorPosX(0.5f * (ImGui::GetColumnWidth() - GetViceMonIconWidth(icon)) + ImGui::GetColumnOffset());
	return DrawTexturedIcon(icon, flipX, width, tint, hover);
}
