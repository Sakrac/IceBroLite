#include "imgui/imgui.h"
#include "GLFW/glfw3.h"
#include "Image.h"

extern const int sIcons_Width;
extern const int sIcons_Height;
extern const unsigned char sIcons_Pixels[];

ImTextureID CreateTexture()
{
	// Turn the RGBA pixel data into an OpenGL texture:
	GLuint my_opengl_texture;
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
	return aIcons[icon].w;
}

bool DrawTexturedIcon(ViceMonIcons icon, bool flipX, float width, const ImVec4& tint, const ImVec4& hover)
{
	bool ret = false;
	if (aIconID) {
		float iW = 1.0f / (float)sIconTexWidth;
		float iH = 1.0f / (float)sIconTexHeight;
		float du = iW * aIcons[icon].w;
		float u0 = iW * aIcons[icon].x, u1 = u0;
		float s = width > 0.0f ? width / (float)aIcons[icon].w : 1.0f;
		if (flipX) { u0 += du; } else { u1 += du; }

		// hover or ting?
		const ImVec4* color = &tint;
		{
			ImVec2 mousePos = ImGui::GetMousePos();
			ImVec2 winPos = ImGui::GetWindowPos();
			ImVec2 curPos = ImGui::GetCursorPos();
			float w = s * aIcons[icon].w;
			float h = s * aIcons[icon].h;
			float x = mousePos.x - (winPos.x + curPos.x);
			float y = mousePos.y - (winPos.y + curPos.y);
			if (x > 0 && x < w && y > 0 && y < h) {
				color = &hover;
				ret = ImGui::IsMouseReleased(0);
			}
		}
		ImGui::Image(aIconID,
					 ImVec2(s * aIcons[icon].w, s * aIcons[icon].h),
					 ImVec2(u0, iH * aIcons[icon].y),
					 ImVec2(u1, iH * (aIcons[icon].y + aIcons[icon].h)),
					 *color);
	}
	return ret;
}

bool DrawTexturedIconCenter(ViceMonIcons icon, bool flipX, float width, const ImVec4& tint, const ImVec4& hover)
{
	ImGui::SetCursorPosX(0.5f * (ImGui::GetColumnWidth() - GetViceMonIconWidth(icon)) + ImGui::GetColumnOffset());
	return DrawTexturedIcon(icon, flipX, width, tint, hover);
}
