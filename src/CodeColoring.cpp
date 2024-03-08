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
	ImGuiCol_TitleBg,
	ImGuiCol_TitleBgActive,
	ImGuiCol_TitleBgCollapsed,
	ImGuiCol_Tab,                   // TabItem in a TabBar
	ImGuiCol_TabHovered,
	ImGuiCol_TabActive,
	ImGuiCol_TabUnfocused,
	ImGuiCol_TabUnfocusedActive,
	ImGuiCol_CheckMark,
	ImGuiCol_MenuBarBg,
	ImGuiCol_Header,
	ImGuiCol_HeaderHovered,
	ImGuiCol_HeaderActive,
	ImGuiCol_TableHeaderBg,
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
	ImGuiCol_ModalWindowDimBg,
};

static const char* saCodeColoringNames[] = {
	"OpCode Color",
	"Address Color",
	"Bytes Color",
	"Param Color",
	"Source Color",
	"Code Cursor Color",
	"PC Highlight Color",
	"Watch Cell Color",
	"Code Label Color"
};

static const uint32_t snThemeColors = sizeof(saThemeColors) / sizeof(saThemeColors[0]);
static const uint32_t snCodeColors = sizeof(saCodeColoringNames) / sizeof(saCodeColoringNames[0]);

#define IDX_SOURCE_COLOR (snCodeColors-1)

struct CustomColors {
	ImVec4 CodeBytesColor;
	ImVec4 CodeAddrColor;
	ImVec4 OpCodeColor;
	ImVec4 SourceColor;
	ImVec4 CodeParamColor;
	ImVec4 CodeCursorColor;
	ImVec4 CodePCHighlightColor;
	ImVec4 WatchChessColor;
	ImVec4 CodeLabelColor;
	float AvoidHueCenter;
	float AvoidHueRadius;
	float BranchTargetV;
	float BranchTargetS;
	bool CodeColoringOn;
	bool PCHighlightStyle;

	CustomColors() {
		CodeBytesColor = C64_LGRAY;
		CodeAddrColor = C64_WHITE;
		OpCodeColor = C64_WHITE;
		SourceColor = C64_YELLOW;
		CodeParamColor = C64_YELLOW;
		CodeCursorColor = C64_PURPLE;
		CodePCHighlightColor = C64_CYAN;
		WatchChessColor = C64_BLUE;
		CodeLabelColor = C64_LGREEN;
		AvoidHueCenter = -1.0f;
		AvoidHueRadius = 0.1f;
		BranchTargetV = 1.0f;
		BranchTargetS = 0.25f;
		CodeColoringOn = true;
		PCHighlightStyle = true;
	}
};


// remember branch targets for this session
typedef HashTable<uint32_t, ImVec4> BranchTargetColor;

static BranchTargetColor sBranchTargets;
static CustomColors sCurrentCustomColors;
static CustomColors sThemeCustomColors;

static bool sLoadThemeFromMenu = false;
static bool sHasCustomTheme = false;
static bool sCustomThemeActive = false;

static ImVec4 sCustomTheme[ImGuiCol_COUNT];

static ImVec4* saCodeColors[snCodeColors] = {
	&sCurrentCustomColors.OpCodeColor,
	&sCurrentCustomColors.CodeAddrColor,
	&sCurrentCustomColors.CodeBytesColor,
	&sCurrentCustomColors.CodeParamColor,
	&sCurrentCustomColors.SourceColor,
	&sCurrentCustomColors.CodeCursorColor,
	&sCurrentCustomColors.CodePCHighlightColor,
	&sCurrentCustomColors.WatchChessColor,
	&sCurrentCustomColors.CodeLabelColor,
};

static ImVec4* saCodeColorsCT[snCodeColors] = {
	&sThemeCustomColors.OpCodeColor,
	&sThemeCustomColors.CodeAddrColor,
	&sThemeCustomColors.CodeBytesColor,
	&sThemeCustomColors.CodeParamColor,
	&sThemeCustomColors.SourceColor,
	&sThemeCustomColors.CodeCursorColor,
	&sThemeCustomColors.CodePCHighlightColor,
	&sThemeCustomColors.WatchChessColor,
	&sThemeCustomColors.CodeLabelColor,
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
	memcpy(&sCurrentCustomColors, &sThemeCustomColors, sizeof(CustomColors));
	memcpy(ImGui::GetStyle().Colors, sCustomTheme, sizeof(sCustomTheme));
	sCustomThemeActive = true;
}

void CopyThemeToCustom() {
	memcpy(&sThemeCustomColors, &sCurrentCustomColors, sizeof(CustomColors));
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
			/*ConfigParseType type =*/ config.Next(&name, &value);
			if (name.same_str("CodeColoring")) {
				sThemeCustomColors.CodeColoringOn = (bool)value.atoi();
			} else if(name.same_str("SourceColor")) {
				sThemeCustomColors.SourceColor = ParseCustomColor(value);
			} else if(name.same_str("OpCodeColor")) {
				sThemeCustomColors.OpCodeColor = ParseCustomColor(value);
			} else if(name.same_str("AddrColor")) {
				sThemeCustomColors.CodeAddrColor = ParseCustomColor(value);
			} else if(name.same_str("ByteColor")) {
				sThemeCustomColors.CodeBytesColor = ParseCustomColor(value);
			} else if(name.same_str("CodeCursorColor")) {
				sThemeCustomColors.CodeCursorColor = ParseCustomColor(value);
			} else if(name.same_str("PCHighlightColor")) {
				sThemeCustomColors.CodePCHighlightColor = ParseCustomColor(value);
			} else if(name.same_str("WatchChessColor")) {
				sThemeCustomColors.WatchChessColor = ParseCustomColor(value);
			} else if(name.same_str("CodeLabelColor")) {
				sThemeCustomColors.CodeLabelColor = ParseCustomColor(value);
			} else if(name.same_str("BranchTargetSaturation")) {
				sThemeCustomColors.BranchTargetS = value.atof();
			} else if(name.same_str("BranchTargetBrightness")) {
				sThemeCustomColors.BranchTargetV = value.atof();
			} else if(name.same_str("BranchTargetHue")) {
				sThemeCustomColors.AvoidHueCenter = value.atof();
			} else if(name.same_str("BranchTargetAvoid")) {
				sThemeCustomColors.AvoidHueRadius = value.atof();
			} else if(name.same_str("PCHighlightStyle")) {
				sThemeCustomColors.PCHighlightStyle = !!value.atoi();
			} else {
				printf("color = " STRREF_FMT "\n", STRREF_ARG(value));
				for (uint32_t i = 0; i < snThemeColors; ++i) {
					int c = saThemeColors[i];
					if (name.same_str(ImGui::GetStyleColorName(c))) {
						sCustomTheme[c] = ParseCustomColor(value);
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
	strovl str(col, colSize);
	theme.AddValue(strref("CodeColoring"), sCurrentCustomColors.CodeColoringOn ? 1 : 0);
	theme.AddValue(strref("SourceColor"), strref(col, AppendColorHash(col, colSize, sCurrentCustomColors.SourceColor)));
	theme.AddValue(strref("OpCodeColor"), strref(col, AppendColorHash(col, colSize, sCurrentCustomColors.OpCodeColor)));
	theme.AddValue(strref("AddrColor"), strref(col, AppendColorHash(col, colSize, sCurrentCustomColors.CodeAddrColor)));
	theme.AddValue(strref("ByteColor"), strref(col, AppendColorHash(col, colSize, sCurrentCustomColors.CodeBytesColor)));
	theme.AddValue(strref("CodeCursorColor"), strref(col, AppendColorHash(col, colSize, sCurrentCustomColors.CodeCursorColor)));
	theme.AddValue(strref("PCHighlightColor"), strref(col, AppendColorHash(col, colSize, sCurrentCustomColors.CodePCHighlightColor)));
	theme.AddValue(strref("WatchChessColor"), strref(col, AppendColorHash(col, colSize, sCurrentCustomColors.WatchChessColor)));
	theme.AddValue(strref("CodeLabelColor"), strref(col, AppendColorHash(col, colSize, sCurrentCustomColors.CodeLabelColor)));
	str.sprintf("%.3f", sCurrentCustomColors.BranchTargetS);
	theme.AddValue(strref("BranchTargetSaturation"), str.get_strref());
	str.sprintf("%.3f", sCurrentCustomColors.BranchTargetV);
	theme.AddValue(strref("BranchTargetBrightness"), str.get_strref());
	str.sprintf("%.3f", sCurrentCustomColors.AvoidHueCenter);
	theme.AddValue(strref("BranchTargetHue"), str.get_strref());
	str.sprintf("%.3f", sCurrentCustomColors.AvoidHueRadius);
	theme.AddValue(strref("BranchTargetAvoid"), str.get_strref());
	theme.AddValue(strref("PCHighlightStyle"), sCurrentCustomColors.PCHighlightStyle ? 1 : 0);
	for (uint32_t i = 0; i < snThemeColors; ++i) {
		int c = saThemeColors[i];
		theme.AddValue(ImGui::GetStyleColorName(c),
			strref(col, AppendColorHash(col, colSize, sCustomTheme[c])));
	}

	strown<PATH_MAX_LEN> path(themeFile);
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
	ImGui::ColorConvertRGBtoHSV(bg.x, bg.y, bg.z, sCurrentCustomColors.AvoidHueCenter, s, v);
	ImGui::ColorConvertRGBtoHSV(fg.x, fg.y, fg.z, th, ts, tv);
	sCurrentCustomColors.BranchTargetV = v > 0.85f ? 0.5f : 1.0f;
	sCurrentCustomColors.BranchTargetS = s < 0.25f ? 0.9f : 0.35f;
	if (v < 0.1f || s > 0.9f) {
		sCurrentCustomColors.AvoidHueCenter = th;
		if (v < 0.1f || s > 0.9f) {
			sCurrentCustomColors.AvoidHueRadius = 0.0f;
		}
		else if (sCurrentCustomColors.AvoidHueRadius < 0.1f) {
			sCurrentCustomColors.AvoidHueRadius = 0.2f;
		}
	}
	else if (sCurrentCustomColors.AvoidHueRadius < 0.1f) {
		sCurrentCustomColors.AvoidHueRadius = 0.2f;
	}

	if (tv > 0.5f) { tv -= 0.4f; }
	else { tv += 0.4f;}

	ImGui::ColorConvertHSVtoRGB(th, ts, tv,
		sCurrentCustomColors.CodeBytesColor.x, sCurrentCustomColors.CodeBytesColor.y, sCurrentCustomColors.CodeBytesColor.z);

	sCurrentCustomColors.CodeAddrColor = fg;
	sCurrentCustomColors.OpCodeColor = fg;
	sCurrentCustomColors.CodeParamColor = fg;
	sCustomThemeActive = false;

	InvalidateBranchTargets();
}

ImVec4 GetCodeBytesColor() {
	return sCurrentCustomColors.CodeColoringOn ? sCurrentCustomColors.CodeBytesColor : ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

ImVec4 GetCodeAddrColor() {
	return sCurrentCustomColors.CodeColoringOn ? sCurrentCustomColors.CodeAddrColor : ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

ImVec4 GetCodeOpCodeColor() {
	return sCurrentCustomColors.CodeColoringOn ? sCurrentCustomColors.OpCodeColor : ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

ImVec4 GetCodeParamColor() {
	return sCurrentCustomColors.CodeColoringOn ? sCurrentCustomColors.CodeParamColor : ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

ImVec4 GetCodeCursorColor()
{
	return sCurrentCustomColors.CodeCursorColor;
}

ImVec4 GetWatchChessColor() {
	return sCurrentCustomColors.WatchChessColor;
}

ImVec4 GetCodeLabelColor() {
	return sCurrentCustomColors.CodeLabelColor;
}

ImVec4 GetPCHighlightColor()
{
	return sCurrentCustomColors.CodePCHighlightColor;
}

int GetPCHighlightStyle()
{
	return sCurrentCustomColors.PCHighlightStyle ? 1 : 0;
}

ImVec4 GetCodeSourceColor() {
	return sCurrentCustomColors.SourceColor;
}


static ImVec4 GetBranchColorCode(float h) {
	float h2 = fmodf(h + 1.0f - sCurrentCustomColors.AvoidHueCenter, 1.0f);
	h2 = h2 * (1.0f - 2.0f * sCurrentCustomColors.AvoidHueRadius) + sCurrentCustomColors.AvoidHueRadius;
	float hue = fmodf(h2 + sCurrentCustomColors.AvoidHueCenter, 1.0f);

	ImVec4 col = C64_WHITE;
	ImGui::ColorConvertHSVtoRGB(hue, sCurrentCustomColors.BranchTargetS, sCurrentCustomColors.BranchTargetV, col.x, col.y, col.z);
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
			if (ImGui::MenuItem("Enable Code Coloring", nullptr, &sCurrentCustomColors.CodeColoringOn)) {
			}
			if (ImGui::MenuItem("PC Highlight Rect", nullptr, &sCurrentCustomColors.PCHighlightStyle)) {
				if (sCustomThemeActive) { sThemeCustomColors.PCHighlightStyle = sCurrentCustomColors.PCHighlightStyle; }
			}
			if (sCurrentCustomColors.CodeColoringOn) {
				for (uint32_t i = 0; i < snCodeColors; ++i) {
					if (sCurrentCustomColors.CodeColoringOn || i == IDX_SOURCE_COLOR) {
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
				if (ImGui::SliderFloat("##avoid", &sCurrentCustomColors.AvoidHueRadius, 0.0f, 0.5f, "avoid = %.2f")) {
					if (!sCustomThemeActive) { CopyThemeToCustom(); }
					sThemeCustomColors.AvoidHueRadius = sCurrentCustomColors.AvoidHueRadius;
				}
				if (ImGui::SliderFloat("##sat", &sCurrentCustomColors.BranchTargetS, 0.0f, 1.0f, "saturation = %.2f")) {
					if (!sCustomThemeActive) { CopyThemeToCustom(); }
					sThemeCustomColors.BranchTargetS = sCurrentCustomColors.BranchTargetS;
				}
				if (ImGui::SliderFloat("##bri", &sCurrentCustomColors.BranchTargetV, 0.0f, 1.0f, "brightness = %.2f")) {
					if (!sCustomThemeActive) { CopyThemeToCustom(); }
					sThemeCustomColors.BranchTargetV = sCurrentCustomColors.BranchTargetV;
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

			for (uint32_t i = 0; i < snThemeColors; i++) {
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
						sCurrentCustomColors.AvoidHueCenter = sThemeCustomColors.AvoidHueCenter = h;
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