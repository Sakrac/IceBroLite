#include <math.h>
#include <stdlib.h>
#include "../imgui/imgui.h"
#include "../C64Colors.h"
#include "../HashTable.h"
#include "CodeColoring.h"

// remember branch targets for this session
typedef HashTable<uint32_t, ImVec4> BranchTargetColor;

static BranchTargetColor sBranchTargets;
uint32_t sBranchTargetColorSeed = 0;
static ImVec4 sCodeBytesColor = C64_LGRAY;
static float sAvoidHueCenter = -1.0f;
static float sAvoidHueCenterText = -1.0f;
static float sAvoidHueRadius = 0.1f;
static float sAvoidHueRadiusText = 0.1f;
static float sBranchTargetV = 1.0f;
static float sBranchTargetS = 0.25f;

void InvalidateBranchTargets() {
	sBranchTargets.Clear();
	sBranchTargetColorSeed = 0;
}

void ResetCodeColoring() {
	float s, v, th, ts, tv;
	ImVec4 bg = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
	ImVec4 fg = ImGui::GetStyleColorVec4(ImGuiCol_Text);
	ImGui::ColorConvertRGBtoHSV(bg.x, bg.y, bg.z, sAvoidHueCenter, s, v);
	ImGui::ColorConvertRGBtoHSV(fg.x, fg.y, fg.z, th, ts, tv);
	sBranchTargetV = v > 0.85f ? 0.5f : 1.0f;
	sBranchTargetS = s < 0.25f ? 0.9f : 0.35f;
	if (v < 0.1f || s > 0.9f) {
		sAvoidHueCenter = th;
		if (v < 0.1f || s > 0.9f) {
			sAvoidHueRadiusText = 0.0f;
		}
		else if (sAvoidHueRadius < 0.1f) {
			sAvoidHueRadiusText = 0.2f;
		}
	}
	else if (sAvoidHueRadius < 0.1f) {
		sAvoidHueRadius = 0.2f;
	}

	if (tv > 0.5f) { tv -= 0.4f; }
	else { tv += 0.4f;}

	ImGui::ColorConvertHSVtoRGB(th, ts, tv, sCodeBytesColor.x, sCodeBytesColor.y, sCodeBytesColor.z);

	InvalidateBranchTargets();
}

ImVec4 GetCodeBytesColor() {
	return sCodeBytesColor;
}

static ImVec4 GetBranchColorCode(float h) {
	float h2 = fmodf(h + 1.0f - sAvoidHueCenter, 1.0f);
	h2 = h2 * (1.0f - 2.0f * sAvoidHueRadius) + sAvoidHueRadius;
	float hue = fmodf(h2 + sAvoidHueCenter, 1.0f);

	ImVec4 col = C64_WHITE;
	ImGui::ColorConvertHSVtoRGB(hue, sBranchTargetS, sBranchTargetV, col.x, col.y, col.z);
	return col;
}

ImVec4* GetBranchTargetColor(uint16_t addr) {
	return sBranchTargets.Value(addr);
}

ImVec4* MakeBranchTargetColor(uint16_t addr) {
	ImVec4* color = GetBranchTargetColor(addr);
	if (color) { return color; }
	float hue = (float)rand() / (float)RAND_MAX;
	return sBranchTargets.Insert(addr, GetBranchColorCode(hue));
}

static ImGuiCol saThemeColors[] = {
	ImGuiCol_Text,
	ImGuiCol_TextDisabled,
	ImGuiCol_WindowBg,
	ImGuiCol_ChildBg,
	ImGuiCol_PopupBg,
	ImGuiCol_Border,
	ImGuiCol_BorderShadow,
	ImGuiCol_FrameBg,
	ImGuiCol_FrameBgHovered,
	ImGuiCol_FrameBgActive,
	ImGuiCol_TitleBg,
	ImGuiCol_TitleBgActive,
	ImGuiCol_TitleBgCollapsed,
	ImGuiCol_MenuBarBg,
	ImGuiCol_ScrollbarBg,
	ImGuiCol_ScrollbarGrab,
	ImGuiCol_ScrollbarGrabHovered,
	ImGuiCol_ScrollbarGrabActive,
	ImGuiCol_CheckMark,
	ImGuiCol_SliderGrab,
	ImGuiCol_SliderGrabActive,
	ImGuiCol_Button,
	ImGuiCol_ButtonHovered,
	ImGuiCol_ButtonActive,
	ImGuiCol_Header,
	ImGuiCol_HeaderHovered,
	ImGuiCol_HeaderActive,
	ImGuiCol_Separator,
	ImGuiCol_SeparatorHovered,
	ImGuiCol_SeparatorActive,
	ImGuiCol_ResizeGrip,
	ImGuiCol_ResizeGripHovered,
	ImGuiCol_ResizeGripActive,
	ImGuiCol_TextSelectedBg,
	ImGuiCol_DragDropTarget,
	ImGuiCol_NavHighlight,
	ImGuiCol_NavWindowingHighlight,
};

static const uint32_t snThemeColors = sizeof(saThemeColors) / sizeof(saThemeColors[0]);

static bool themeMenuClosed = false;
void ThemeColorMenu()
{
	ImGuiStyle* ref = nullptr;
	if (ImGui::BeginMenu("Theme")) {
		if (ImGui::BeginMenu("Code Coloring")) {
			ImGui::SliderFloat("Branch Trg##avoid", &sAvoidHueRadius, 0.0f, 0.5f, "avoid = %.2f");
			ImGui::SliderFloat("Branch Trg##sat", &sBranchTargetS, 0.0f, 1.0f, "saturation = %.2f");
			ImGui::SliderFloat("Branch Trg##bri", &sBranchTargetV, 0.0f, 1.0f, "brightness = %.2f");
			ImVec2 pos = ImGui::GetCursorScreenPos();

			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			float ch = ImGui::GetTextLineHeightWithSpacing();
			float cw = ImGui::GetFont()->GetCharAdvance('D');
			draw_list->AddRectFilled(pos,
				ImVec2(pos.x + cw * (4 * 5 - 1), pos.y + ch * 4),
				ImGui::GetColorU32(ImGuiCol_ChildBg));

			for (int y = 0; y < 4; ++y) {
				for (int x = 0; x < 4; ++x) {
					ImGui::TextColored(GetBranchColorCode(0.25f * y + (0.25f * 0.25f) * x), "%04x", y * 0x400 + x * 0x40);
					if (x != 3) { ImGui::SameLine(); }
				}
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("ImGui Colors")) {
			ImGuiStyle& style = ImGui::GetStyle();
			static ImGuiStyle ref_saved_style;
			static bool init = true;
			if (themeMenuClosed && ref == NULL)
				ref_saved_style = style;
			themeMenuClosed = false;
			if (ref == NULL)
				ref = &ref_saved_style;

			for (int i = 0; i < snThemeColors; i++)
			{
				const char* name = ImGui::GetStyleColorName(saThemeColors[i]);
				ImGui::PushID(i);
				ImGui::ColorEdit3("##color", (float*)&style.Colors[i], ImGuiColorEditFlags_NoInputs);
				if (memcmp(&style.Colors[i], &ref->Colors[i], sizeof(ImVec4)) != 0)
				{
					// Tips: in a real user application, you may want to merge and use an icon font into the main font,
					// so instead of "Save"/"Revert" you'd use icons!
					// Read the FAQ and docs/FONTS.md about using icon fonts. It's really easy and super convenient!
					ImGui::SameLine(0.0f, style.ItemInnerSpacing.x); if (ImGui::Button("Save")) { ref->Colors[i] = style.Colors[i]; }
					ImGui::SameLine(0.0f, style.ItemInnerSpacing.x); if (ImGui::Button("Revert")) { style.Colors[i] = ref->Colors[i]; }
				}
				ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
				ImGui::TextUnformatted(name);
				ImGui::PopID();
			}
			ImGui::EndMenu();
		} else {
			themeMenuClosed = true;
		}
		if (ImGui::MenuItem("Save Theme As..")) {
		}
		if (ImGui::MenuItem("Load Custom Theme")) {
		}
		ImGui::EndMenu();
	}
}