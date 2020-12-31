#include "imgui/imgui.h"
#include "GLFW/glfw3.h"
#include "Image.h"

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

void UpdateTextureData(int width, int height, int channels, const void* data)
{
	glTexImage2D(GL_TEXTURE_2D, 0, channels == 3 ? GL_RGB : GL_RGBA, width, height, 0, channels == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, data);
}

void DestroyTexture(ImTextureID texID)
{
	GLuint tid = (GLuint)texID;
	glDeleteTextures(1, &tid);
}

