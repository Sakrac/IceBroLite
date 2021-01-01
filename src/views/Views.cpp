#include <assert.h>
#include "../imgui/imgui.h"
#include "../struse/struse.h"
#include "ToolBar.h"
#include "RegView.h"
#include "MemView.h"
#include "../6510.h"
#include "../data/C64_Pro_Mono-STYLE.ttf.h"
#include "Views.h"


struct ViewContext {
	enum { sNumFontSizes = 7 };
	enum { MaxMemViews = 4 };
	ToolBar toolBar;
	RegisterView regView;
	MemView memView[MaxMemViews];
	ImFont* aFonts[sNumFontSizes];
	int currFont;

	ViewContext();
	void Draw();
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

ViewContext::ViewContext() : currFont(3)
{
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	for (int f = 0; f < ViewContext::sNumFontSizes; ++f) {
		aFonts[f] = io.Fonts->AddFontFromMemoryCompressedTTF(GetC64FontData(), GetC64FontSize(), sFontSizes[f], NULL, C64CharRanges);
		assert(aFonts[f] != NULL);
	}
	memView[0].open = true;
}

void ViewContext::Draw()
{
	ImGui::PushFont(aFonts[currFont]);
	toolBar.Draw();
	regView.Draw();
	for (int m = 0; m < MaxMemViews; ++m) { memView[m].Draw(m); }
	ImGui::PopFont();
	if (CPU6510* cpu = GetCurrCPU()) {
		cpu->RefreshMemory();
	}
}

void InitViews()
{
	viewContext = new ViewContext;

}

void ShowViews()
{
	if (viewContext) {
		viewContext->Draw();
	}
}

float CurrFontSize()
{
	return viewContext ? sFontSizes[viewContext->currFont] : 10.0f;
}

uint8_t InputHex()
{
	for (int num = 0; num < 9; ++num) { if (ImGui::IsKeyPressed(num + '0')) return num; }
	for (int num = 10; num < 16; ++num) { if (ImGui::IsKeyPressed(num + 'A' - 10)) return num; }
	return 0xff;
}
