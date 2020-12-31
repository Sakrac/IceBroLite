#include <assert.h>
#include "../imgui/imgui.h"
#include "../struse/struse.h"
#include "ToolBar.h"
#include "../data/C64_Pro_Mono-STYLE.ttf.h"


struct ViewContext {
	enum { sNumFontSizes = 7 };
	ToolBar toolBar;
	ImFont* aFonts[sNumFontSizes];
};

static ViewContext* viewContext = nullptr;
static float sFontSizes[ViewContext::sNumFontSizes] = { 8.0f, 10.0f, 12.0, 14.0f, 16.0f, 20.0f, 24.0f };
static const ImWchar C64CharRanges[] =
{
	0x0020, 0x00FF, // Basic Latin + Latin Supplement
	0xee00, 0xeeff,
	0
	//	0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
	//	0x2DE0, 0x2DFF, // Cyrillic Extended-A
	//	0xA640, 0xA69F, // Cyrillic Extended-B
	//	0,
};

void InitViews()
{
	viewContext = new ViewContext;
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	for (int f = 0; f < ViewContext::sNumFontSizes; ++f) {
		viewContext->aFonts[f] = io.Fonts->AddFontFromMemoryCompressedTTF(GetC64FontData(), GetC64FontSize(), sFontSizes[f], NULL, C64CharRanges);
		assert(viewContext->aFonts[f] != NULL);
	}
}

void ShowViews()
{
	if (viewContext) {
		ImGui::PushFont(viewContext->aFonts[3]);
		viewContext->toolBar.Draw();
		ImGui::PopFont();
	}
}
