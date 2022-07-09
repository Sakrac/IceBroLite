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

const int nIcons = sizeof(aIcons) / sizeof(aIcons[0]);

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
