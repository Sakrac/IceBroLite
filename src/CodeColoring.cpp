#include <math.h>
#include <stdlib.h>
#include "../imgui/imgui.h"
#include "../C64Colors.h"
#include "../HashTable.h"
#include "CodeColoring.h"
#include "Config.h"
#include "Files.h"
#include "FileDialog.h"
#include "struse/struse.h"

static ImGuiCol saThemeColors[] = {
	ImGuiCol_Text,
	ImGuiCol_TextDisabled,
	ImGuiCol_WindowBg,
	ImGuiCol_ChildBg,
	ImGuiCol_Border,
	ImGuiCol_BorderShadow,
	ImGuiCol_Button,
	ImGuiCol_ButtonHovered,
	ImGuiCol_ButtonActive,
	ImGuiCol_PopupBg,
	ImGuiCol_FrameBg,
	ImGuiCol_FrameBgHovered,
	ImGuiCol_FrameBgActive,
	ImGuiCol_CheckMark,
	ImGuiCol_TitleBg,
	ImGuiCol_TitleBgActive,
	ImGuiCol_TitleBgCollapsed,
	ImGuiCol_MenuBarBg,
	ImGuiCol_Header,
	ImGuiCol_HeaderHovered,
	ImGuiCol_HeaderActive,
	ImGuiCol_NavHighlight,
	ImGuiCol_NavWindowingHighlight,
	ImGuiCol_NavWindowingDimBg,
	ImGuiCol_ScrollbarBg,
	ImGuiCol_ScrollbarGrab,
	ImGuiCol_ScrollbarGrabHovered,
	ImGuiCol_ScrollbarGrabActive,
	ImGuiCol_SliderGrab,
	ImGuiCol_SliderGrabActive,
	ImGuiCol_Separator,
	ImGuiCol_SeparatorHovered,
	ImGuiCol_SeparatorActive,
	ImGuiCol_ResizeGrip,
	ImGuiCol_ResizeGripHovered,
	ImGuiCol_ResizeGripActive,
	ImGuiCol_TextSelectedBg,
	ImGuiCol_DragDropTarget,
	ImGuiCol_ModalWindowDimBg,
};

static const char* saCodeColoringNames[] = {
	"OpCode Color",
	"Address Color",
	"Bytes Color",
	"Param Color",
	"Source Color"
};

static const uint32_t snThemeColors = sizeof(saThemeColors) / sizeof(saThemeColors[0]);
static const uint32_t snCodeColors = sizeof(saCodeColoringNames) / sizeof(saCodeColoringNames[0]);

#define IDX_SOURCE_COLOR (snCodeColors-1)

// remember branch targets for this session
typedef HashTable<uint32_t, ImVec4> BranchTargetColor;

static BranchTargetColor sBranchTargets;
static ImVec4 sCodeBytesColor = C64_LGRAY;
static ImVec4 sCodeAddrColor = C64_WHITE;
static ImVec4 sOpCodeColor = C64_WHITE;
static ImVec4 sSourceColor = C64_YELLOW;
static ImVec4 sCodeParamColor = C64_YELLOW;

static ImVec4 sCodeBytesColorCT = C64_LGRAY;
static ImVec4 sCodeAddrColorCT = C64_WHITE;
static ImVec4 sOpCodeColorCT = C64_WHITE;
static ImVec4 sSourceColorCT = C64_YELLOW;
static ImVec4 sCodeParamColorCT = C64_YELLOW;

static float sAvoidHueCenter = -1.0f;
static float sAvoidHueRadius = 0.1f;
static float sBranchTargetV = 1.0f;
static float sBranchTargetS = 0.25f;

static float sAvoidHueCenterCT = -1.0f;
static float sAvoidHueRadiusCT = 0.1f;
static float sBranchTargetVCT = 1.0f;
static float sBranchTargetSCT = 0.25f;



static bool sCodeColoringOn = true;
static bool sCodeColoringOnCT = true;
static bool sLoadThemeFromMenu = false;
static bool sHasCustomTheme = false;
static bool sCustomThemeActive = false;

static ImVec4 sCustomTheme[ImGuiCol_COUNT];

static ImVec4* saCodeColors[snCodeColors] = {
	&sCodeBytesColor,
	&sCodeAddrColor,
	&sOpCodeColor,
	&sCodeParamColor,
	&sSourceColor,
};

static ImVec4* saCodeColorsCT[snCodeColors] = {
	&sCodeBytesColorCT,
	&sCodeAddrColorCT,
	&sOpCodeColorCT,
	&sCodeParamColorCT,
	&sSourceColorCT,
};

void InvalidateBranchTargets() {
	sBranchTargets.Clear();
}

ImVec4 ParseCustomColor(strref value) {
	ImVec4 col = C64_WHITE;
	if (value[0] == '#') { ++value;}
	col.x = (float)value.get_substr(0, 2).ahextoi() / 255.0f;
	col.y = (float)value.get_substr(2, 2).ahextoi() / 255.0f;
	col.z = (float)value.get_substr(4, 2).ahextoi() / 255.0f;
	col.w = value.get_len()>=8 ? (float)value.get_substr(6, 2).ahextoi() / 255.0f : 1.0f;
	return col;
}

void SetCustomTheme() {
	sCodeBytesColor = sCodeBytesColorCT;
	sCodeAddrColor = sCodeAddrColorCT;
	sOpCodeColor = sOpCodeColorCT;
	sSourceColor = sSourceColorCT;
	sCodeParamColor = sCodeParamColorCT;
	sAvoidHueCenter = sAvoidHueCenterCT;
	sAvoidHueRadius = sAvoidHueRadiusCT;
	sBranchTargetS = sBranchTargetSCT;
	sBranchTargetV = sBranchTargetVCT;
	sCodeColoringOn = sCodeColoringOnCT;

	memcpy(ImGui::GetStyle().Colors, sCustomTheme, sizeof(sCustomTheme));
	sCustomThemeActive = true;
}

void CopyThemeToCustom() {
	sCodeBytesColorCT = sCodeBytesColor;
	sCodeAddrColorCT = sCodeAddrColor;
	sOpCodeColorCT = sOpCodeColor;
	sSourceColorCT = sSourceColor;
	sCodeParamColorCT = sCodeParamColor;
	sAvoidHueCenterCT = sAvoidHueCenter;
	sAvoidHueRadiusCT = sAvoidHueRadius;
	sBranchTargetSCT = sBranchTargetS;
	sBranchTargetVCT = sBranchTargetV;
	sCodeColoringOnCT = sCodeColoringOn;

	memcpy(sCustomTheme, ImGui::GetStyle().Colors, sizeof(ImGui::GetStyle().Colors));
	sCustomThemeActive = true;
	sHasCustomTheme = true;
}

bool HasCustomTheme() {
	return sHasCustomTheme;
}

bool CustomThemeActive() {
	return sCustomThemeActive;
}

void LoadCustomTheme(const char *themeFile) {
	size_t size;
	if (uint8_t* data = LoadBinary(themeFile, size)) {
		ConfigParse config(data, size);
		memcpy(sCustomTheme, ImGui::GetStyle().Colors, sizeof(sCustomTheme));
		while (!config.Empty()) {
			strref name, value;
			ConfigParseType type = config.Next(&name, &value);
			if (name.same_str("CodeColoring")) {
				sCodeColoringOnCT = (bool)value.atoi();
			} else if(name.same_str("SourceColor")) {
				sSourceColorCT = ParseCustomColor(value);
			} else if(name.same_str("OpCodeColor")) {
				sOpCodeColorCT = ParseCustomColor(value);
			} else if(name.same_str("AddrColor")) {
				sCodeAddrColorCT = ParseCustomColor(value);
			} else if(name.same_str("ByteColor")) {
				sCodeBytesColorCT = ParseCustomColor(value);
			} else if(name.same_str("BranchTargetSaturation")) {
				sBranchTargetSCT = value.atof();
			} else if(name.same_str("BranchTargetBrightness")) {
				sBranchTargetVCT = value.atof();
			} else if(name.same_str("BranchTargetHue")) {
				sAvoidHueCenterCT = value.atof();
			} else if(name.same_str("BranchTargetAvoid")) {
				sAvoidHueRadiusCT = value.atof();
			} else {
				for (int i = 0; i < snCodeColors; ++i) {
					if (name.same_str(ImGui::GetStyleColorName(saThemeColors[i]))) {
						sCustomTheme[saThemeColors[i]] = ParseCustomColor(value);
					}
				}
			}
		}
		sHasCustomTheme = true;
		if (sLoadThemeFromMenu) {
			SetCustomTheme();
		}
		free(data);
	}
	sLoadThemeFromMenu = false;
}

strl_t AppendColorHash(char* buf, size_t left, ImVec4 color) {
	strovl str(buf, (strl_t)left);
	str.append('#');
	str.append_num((uint32_t)(color.x * 255.0f), 2, 16);
	str.append_num((uint32_t)(color.y * 255.0f), 2, 16);
	str.append_num((uint32_t)(color.z * 255.0f), 2, 16);
	str.append_num((uint32_t)(color.w * 255.0f), 2, 16);
	return str.len();
}

void SaveCustomTheme(const char* themeFile) {
	UserData theme;
	char col[32];
	const size_t colSize = sizeof(col);
	strl_t colLen = 0;
	strovl str(col, colSize);
	theme.AddValue(strref("CodeColoring"), sCodeColoringOn ? 1 : 0);
	theme.AddValue(strref("SourceColor"), strref(col, AppendColorHash(col, colSize, sSourceColor)));
	theme.AddValue(strref("OpCodeColor"), strref(col, AppendColorHash(col, colSize, sOpCodeColor)));
	theme.AddValue(strref("AddrColor"), strref(col, AppendColorHash(col, colSize, sCodeAddrColor)));
	theme.AddValue(strref("ByteColor"), strref(col, AppendColorHash(col, colSize, sCodeBytesColor)));
	str.sprintf("%.3f", sBranchTargetS);
	theme.AddValue(strref("BranchTargetSaturation"), str.get_strref());
	str.sprintf("%.3f", sBranchTargetV);
	theme.AddValue(strref("BranchTargetBrightness"), str.get_strref());
	str.sprintf("%.3f", sAvoidHueCenter);
	theme.AddValue(strref("BranchTargetHue"), str.get_strref());
	str.sprintf("%.3f", sAvoidHueRadius);
	theme.AddValue(strref("BranchTargetAvoid"), str.get_strref());
	for (int i = 0; i < snThemeColors; ++i) {
		theme.AddValue(ImGui::GetStyleColorName(saThemeColors[i]),
			strref(col, AppendColorHash(col, colSize, sCustomTheme[i])));
	}

	strown<_MAX_PATH> path(themeFile);
	if (!path.has_suffix(".theme.txt")) {
		int period = path.find('.');
		if (period > 1) { path.set_len(period); }
		path.append(".theme.txt");
	}

	SaveFile(path.c_str(), theme.start, theme.curr - theme.start);
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
			sAvoidHueRadius = 0.0f;
		}
		else if (sAvoidHueRadius < 0.1f) {
			sAvoidHueRadius = 0.2f;
		}
	}
	else if (sAvoidHueRadius < 0.1f) {
		sAvoidHueRadius = 0.2f;
	}

	if (tv > 0.5f) { tv -= 0.4f; }
	else { tv += 0.4f;}

	ImGui::ColorConvertHSVtoRGB(th, ts, tv, sCodeBytesColor.x, sCodeBytesColor.y, sCodeBytesColor.z);

	sCodeAddrColor = fg;
	sOpCodeColor = fg;
	sCodeParamColor = fg;
	sCustomThemeActive = false;

	InvalidateBranchTargets();
}

ImVec4 GetCodeBytesColor() {
	return sCodeColoringOn ? sCodeBytesColor : ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

ImVec4 GetCodeAddrColor() {
	return sCodeColoringOn ? sCodeAddrColor : ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

ImVec4 GetCodeOpCodeColor() {
	return sCodeColoringOn ? sOpCodeColor : ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

ImVec4 GetCodeParamColor() {
	return sCodeColoringOn ? sCodeParamColor : ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

ImVec4 GetCodeSourceColor() {
	return sSourceColor;
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

void ThemeColorMenu()
{
	if (ImGui::BeginMenu("Theme")) {
		if (ImGui::BeginMenu("Code Coloring")) {
			if (ImGui::MenuItem("Enable Code Coloring", nullptr, &sCodeColoringOn)) {
			}
			if (sCodeColoringOn) {
				for (int i = 0; i < snCodeColors; ++i) {
					if (sCodeColoringOn || i == IDX_SOURCE_COLOR) {
						ImGui::PushID(i);
						ImVec4 currCol = *saCodeColors[i];
						ImGui::ColorEdit4("##color", (float*)saCodeColors[i], ImGuiColorEditFlags_NoInputs);
						if (memcmp(saCodeColors[i], &currCol, sizeof(ImVec4)) != 0) {
							if (!sCustomThemeActive) { CopyThemeToCustom(); }
							saCodeColorsCT[i] = saCodeColors[i];
						}
						ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
						ImGui::TextUnformatted(saCodeColoringNames[i]);
						ImGui::PopID();
					}
				}

				ImGui::Separator();
				ImGui::Text("Branch Target Coloring");
				if (ImGui::MenuItem("Reset Branch Targets")) {
					InvalidateBranchTargets();
				}
				if (ImGui::SliderFloat("##avoid", &sAvoidHueRadius, 0.0f, 0.5f, "avoid = %.2f")) {
					if (!sCustomThemeActive) { CopyThemeToCustom(); }
					sAvoidHueRadiusCT = sAvoidHueRadius;
				}
				if (ImGui::SliderFloat("##sat", &sBranchTargetS, 0.0f, 1.0f, "saturation = %.2f")) {
					if (!sCustomThemeActive) { CopyThemeToCustom(); }
					sBranchTargetSCT = sBranchTargetS;
				}
				if (ImGui::SliderFloat("##bri", &sBranchTargetV, 0.0f, 1.0f, "brightness = %.2f")) {
					if (!sCustomThemeActive) { CopyThemeToCustom(); }
					sBranchTargetVCT = sBranchTargetS;
				}
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
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("ImGui Colors")) {
			ImGuiStyle& style = ImGui::GetStyle();

			for (int i = 0; i < snThemeColors; i++) {
				ImGuiCol c = saThemeColors[i];
				const char* name = ImGui::GetStyleColorName(c);
				ImGui::PushID(i);
				ImVec4 prevCol = style.Colors[c];
				ImGui::ColorEdit4("##color", (float*)&style.Colors[c], ImGuiColorEditFlags_NoInputs);
				if (memcmp(&style.Colors[c], &prevCol, sizeof(ImVec4)) != 0) {
					if (!sCustomThemeActive) {
						CopyThemeToCustom();
					}
					if (c == ImGuiCol_WindowBg || c == ImGuiCol_ChildBg) {
						ImVec4 bg = style.Colors[ImGuiCol_WindowBg];
						ImVec4 ch = style.Colors[ImGuiCol_ChildBg];
						float a = ch.w, b = 1.0f - a;
						float h, s, v;
						ImGui::ColorConvertRGBtoHSV(bg.x * b + ch.x * a, bg.y * b + ch.y * a, bg.z * b + ch.z * a,
							h, s, v);
						sAvoidHueCenter = sAvoidHueCenterCT = h;
					}
					sCustomTheme[c] = style.Colors[c];
				}
				ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
				ImGui::TextUnformatted(name);
				ImGui::PopID();
			}
			ImGui::EndMenu();
		}
		if (ImGui::MenuItem("Save Theme As..")) {
			SaveThemeDialog();
		}
		if (ImGui::MenuItem("Load Custom Theme")) {
			sLoadThemeFromMenu = true;
			LoadThemeDialog();
		}
		ImGui::EndMenu();
	}
}